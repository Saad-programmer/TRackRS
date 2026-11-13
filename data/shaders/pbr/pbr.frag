#version 330 core
// ============================================================================
// pbr.frag — TrackRS Cook-Torrance PBR fragment stage (GLSL 330 core)
//
// BRDF  : Lambert diffuse + Cook-Torrance specular  f = D * G * F / (4 NdotL NdotV)
//           D = GGX / Trowbridge-Reitz        (D_GGX)
//           G = Smith, Schlick-GGX (direct k = (r+1)^2/8)   (G_SchlickGGX)
//           F = Schlick Fresnel               (F_Schlick)
// IBL   : split-sum  — irradiance cubemap + prefiltered radiance cubemap (mips = roughness)
//         + 2D BRDF LUT (scale, bias). Sharp chrome/clear-coat reflections come from the
//         prefiltered cubemap sampled with textureLod().
// EXTRAS: clear-coat lobe (car paint), wetness (asphalt/paint), specular anti-aliasing,
//         alpha-test, ACES filmic tone-map, gamma 2.2.
// TEXTURE CONTRACT (bind once per material, 4 sampler units for the surface set):
//   uAlbedoMap     sRGB8_A8  base colour (+alpha)   -> hardware sRGB->linear, NO pow() in shader
//   uNormalMap     RG/RGB    tangent space, LINEAR  -> Z reconstructed (BC5-ready)
//   uMetalRoughMap RGB       R=AO (optional)  G=roughness  B=metallic   (glTF 2.0 packing), LINEAR
//   IBL: uIrradianceMap (cube) / uPrefilterMap (cube, full mip chain) / uBrdfLUT (RG16F)
// ============================================================================

in VS_OUT {
    vec3 worldPos;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
    vec4 color;
#ifdef VEGETATION
    flat float fade;          // LOD dither code, see engine/vegetation/vegetation.h
#endif
#ifdef CAR
    vec3 modelPos;            // body/wheel model space (lights, dirt masks)
    vec3 modelNormal;
#endif
} fs;

out vec4 FragColor;
#include "../world/scene.glsl"

// ---- material -------------------------------------------------------------
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetalRoughMap;
uniform vec4  uBaseColorFactor = vec4(1.0);
uniform float uMetallicFactor  = 1.0;
uniform float uRoughnessFactor = 1.0;
uniform float uNormalScale     = 1.0;
uniform float uAOStrength      = 1.0;   // 0 disables the R channel of the MR map
uniform float uAlphaCutoff     = 0.0;   // >0 => alpha test (foliage, signs, fences)
uniform float uClearcoat       = 0.0;   // 0..1  (car paint = 1.0)
uniform float uClearcoatRough  = 0.03;
uniform float uWetness         = 0.0;   // 0..1  (rain / wet tarmac)
#ifdef CAR
// Phase 4b car material (see engine/car/car_renderer.cpp).
uniform sampler2D uRoleMap;     // R paint (clear-coat), G glass, B rubber, A metal
uniform sampler2D uLightMap;    // R tail/brake light texels, G headlight texels
uniform int   uCarPart;         // 0 body, 1 wheel, 2 interior
uniform bool  uGlassPass;       // second pass: glass only, blended
uniform vec4  uCarLights;       // head on, tail on, brake 0..1, reverse
uniform vec4  uDirt;            // dust, mud, scene wetness, unused
uniform vec4  uBodyBounds;      // model-space minZ, maxZ, frontY, wheel radius
uniform float uBrakeGlow;       // 0..1 for this wheel
uniform vec4  uDamage;          // Phase 4c: corner damage FL, FR, RL, RR (0..1)
uniform vec4  uBodyExtent;      // body model space: minX, maxX, minY (rear), maxY (front)
uniform vec4  uLampsBroken;     // head left, head right, tail left, tail right (0/1)
// Mirrors trackrs::car::damageAt and car.vert: keep in step.
float carDamageAt(vec2 xy) {
    vec2 size = max(uBodyExtent.yw - uBodyExtent.xz, vec2(1e-4));
    vec2 corners[4] = vec2[4](vec2(uBodyExtent.x, uBodyExtent.w), vec2(uBodyExtent.y, uBodyExtent.w),
                              vec2(uBodyExtent.x, uBodyExtent.z), vec2(uBodyExtent.y, uBodyExtent.z));
    float strongest = 0.0;
    for (int i = 0; i < 4; ++i)
        strongest = max(strongest, uDamage[i] * (1.0 - smoothstep(0.12, 0.7, length((xy - corners[i]) / size))));
    return strongest;
}
float carHash(vec3 p) { return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453); }
float carNoise(vec3 p) {
    vec3 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(carHash(i), carHash(i + vec3(1, 0, 0)), f.x), mix(carHash(i + vec3(0, 1, 0)), carHash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(carHash(i + vec3(0, 0, 1)), carHash(i + vec3(1, 0, 1)), f.x), mix(carHash(i + vec3(0, 1, 1)), carHash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}
#endif
#ifdef VEGETATION
uniform bool  uAlbedoGamma     = false; // legacy UNORM sprite cards: decode sRGB here
uniform float uTranslucency    = 0.0;   // back-lit leaf transmission weight
uniform int   uTemporalFrame   = 0;     // TAA frame (0..63) rotating the LOD dither
#endif

// ---- lighting -------------------------------------------------------------
uniform vec3  uCamPos;
uniform vec3  uSunDir;                  // direction TOWARDS the sun (normalised)
uniform vec3  uSunColor;                // linear radiance * intensity
uniform vec3  uAmbientColor;            // fallback when no IBL is bound
#define MAX_POINT_LIGHTS 8
uniform int   uPointLightCount;
uniform vec3  uPointLightPos[MAX_POINT_LIGHTS];
uniform vec4  uPointLightColorRange[MAX_POINT_LIGHTS];   // rgb = radiance, a = range (m)

// ---- image based lighting -------------------------------------------------
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;
uniform sampler2D   uBrdfLUT;
uniform float uPrefilterMaxLod = 7.0;   // mip count - 1 of uPrefilterMap
uniform float uEnvIntensity    = 1.0;
uniform bool  uHasIBL          = false;
uniform mat3 uEnvironmentRotation;
uniform mat3 uProbeRotation;
uniform samplerCube uLocalProbe;
uniform bool uUseLocalProbe=false;
uniform float uProbeMaxLod=0;
vec3 environmentSpecular(vec3 direction,float rough){vec3 value=textureLod(uPrefilterMap,uEnvironmentRotation*direction,rough*uPrefilterMaxLod).rgb;if(uUseLocalProbe)value=textureLod(uLocalProbe,uProbeRotation*direction,rough*uProbeMaxLod).rgb;return value;}

// CSM interface only. Phase 2c does not allocate or bake shadow cascades.
#ifdef USE_CSM
uniform sampler2DArrayShadow uShadowMap;
uniform mat4 uLightViewProj[4];
uniform vec4 uCascadeSplits;
uniform mat4 uView;
uniform int uCascadeCount;
uniform bool uShadowEnabled;
float sunVisibility() {
    if(uSceneEnabled)return sceneVisibility((uSceneFromLocal*vec4(fs.worldPos,1)).xyz,normalize(mat3(uSceneFromLocal)*fs.N));
    if (!uShadowEnabled || uCascadeCount == 0) return 1.0;
    float depth = -(uView * vec4(fs.worldPos, 1.0)).z;
    int layer = 0;
    for (int i=0; i<3; ++i) if (i < uCascadeCount-1 && depth > uCascadeSplits[i]) layer=i+1;
    vec4 light = uLightViewProj[layer] * vec4(fs.worldPos, 1.0);
    vec3 coord = light.xyz / light.w * 0.5 + 0.5;
    if (any(lessThan(coord,vec3(0))) || any(greaterThan(coord,vec3(1)))) return 1.0;
    return texture(uShadowMap, vec4(coord.xy, float(layer), coord.z-0.001));
}
#else
float sunVisibility() { return uSceneEnabled?sceneVisibility((uSceneFromLocal*vec4(fs.worldPos,1)).xyz,normalize(mat3(uSceneFromLocal)*fs.N)):1.0; }
#endif
uniform int uDebugView = 0;
// ---- output ---------------------------------------------------------------
uniform float uExposure = 1.0;
uniform bool  uToneMap  = true;          // false when a later HDR post pass tone-maps
const float GAMMA = 2.2;
const float PI    = 3.14159265359;

// ============================================================================
// BRDF building blocks
// ============================================================================
#include "brdf.glsl"
// ============================================================================
#ifdef TERRAIN
#include "terrain_material.glsl"
#endif
void main()
{
#ifdef VEGETATION
    // Screen-door LOD cross-fade (interleaved gradient noise): the outgoing and
    // incoming LOD of one plant keep complementary pixel sets, so nothing pops
    // and nothing is blended (no sorting, early-Z stays valid).
    // With TAA the pattern rotates by the golden ratio every frame (both LODs
    // share it, so they stay complementary) and the resolve averages it into
    // a smooth cross-fade; without TAA uTemporalFrame is 0 (static pattern).
    float lodNoise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715)))
                           + 0.61803399 * float(uTemporalFrame));
    if (fs.fade >= 2.0 ? lodNoise < fs.fade - 2.0 : lodNoise >= fs.fade) discard;
#endif
    // ---- fetch material (3 texture reads for the whole surface) -----------
#ifdef TERRAIN
    TerrainSurface terrain = terrainMaterial(fs.worldPos, normalize(fs.N));
    vec4 baseSample = vec4(terrain.albedo,1.0);
#else
    vec4 baseSample = texture(uAlbedoMap, fs.uv) * uBaseColorFactor;
#endif
#ifdef VEGETATION
    if (uAlbedoGamma) baseSample.rgb = pow(baseSample.rgb, vec3(2.2));
#endif
    #ifdef ALPHA_TO_COVERAGE
    if (uAlphaCutoff > 0.0) baseSample.a = clamp((baseSample.a-uAlphaCutoff)/max(fwidth(baseSample.a),0.001)+0.5,0.0,1.0);
#else
    if (uAlphaCutoff > 0.0 && baseSample.a < uAlphaCutoff) discard;
#endif

    if(uDepthOnly){FragColor=vec4(0);return;}
    vec3 albedo = baseSample.rgb * fs.color.rgb;            // vertex colour = baked wear/AO tint (white if unused)
#ifdef TERRAIN
    vec3 mr = terrain.orm;
#else
    vec3 mr = texture(uMetalRoughMap, fs.uv).rgb;
#endif
    float ao        = mix(1.0, mr.r, uAOStrength) * fs.color.a;
    float roughness = clamp(mr.g * uRoughnessFactor, 0.04, 1.0);
    float metallic  = clamp(mr.b * uMetallicFactor, 0.0, 1.0);

#ifdef CAR
    float carGlass = 0.0, carDirt = 0.0, carCrumple = 0.0, carScratch = 0.0, carCrack = 0.0;
    vec3 carEmissive = vec3(0.0);
    if (uCarPart != 2) {
        vec4 role = texture(uRoleMap, fs.uv);
        vec4 lamp = texture(uLightMap, fs.uv);
        carGlass = role.g;
        if (uGlassPass ? carGlass < 0.5 : carGlass > 0.5) discard;
        vec3 mp0 = fs.modelPos;
        float bodyWidth = max(uBodyExtent.y - uBodyExtent.x, 1e-4);
        float carDamage = uCarPart == 0 ? carDamageAt(mp0.xy) : 0.0;
        if (carGlass > 0.5) {           // tinted rally glass: dark, mirror-smooth
            albedo = vec3(0.02, 0.025, 0.03); roughness = 0.05; metallic = 0.0;
            // Phase 4c: crazed glass near a smashed corner (noise iso-lines read as a crack web).
            if (carDamage > 0.3) {
                vec3 q = mp0 / bodyWidth * 38.0;
                float web = min(abs(carNoise(q) - 0.5), abs(carNoise(q * 2.3 + 5.0) - 0.5));
                carCrack = (1.0 - smoothstep(0.0, 0.035, web)) * smoothstep(0.3, 0.75, carDamage);
                albedo = mix(albedo, vec3(0.72, 0.75, 0.78), carCrack);
                roughness = mix(roughness, 0.6, carCrack);
            }
        } else if (role.b > 0.5) {      // tyres, rubber trim, grilles
            roughness = max(roughness, 0.88); metallic = 0.0;
        } else if (role.a > 0.5) {      // rims, chrome, bolts
            metallic = 0.9; roughness = 0.28;
        } else {                        // clear-coated paint base
            roughness = 0.42; metallic = 0.0;
        }
        // Lamps: tail/brake from the emission atlas, head from bright front texels.
        vec3 mp = fs.modelPos, mn = normalize(fs.modelNormal);
        // Phase 4c: a lamp on a smashed corner (>= 0.8) stays dark.
        bool leftSide = mp.x < 0.5 * (uBodyExtent.x + uBodyExtent.y);
        float tailAlive = 1.0 - (leftSide ? uLampsBroken.z : uLampsBroken.w);
        float headAlive = 1.0 - (leftSide ? uLampsBroken.x : uLampsBroken.y);
        float tail = lamp.r * (uCarLights.y * 1.2 + uCarLights.z * 6.0) * (uCarPart == 0 ? tailAlive : 1.0);
        float front = smoothstep(uBodyBounds.z - 25.0, uBodyBounds.z, mp.y) * step(0.3, mn.y);
        float head = lamp.g * front * uCarLights.x * 9.0 * (uCarPart == 0 ? headAlive : 1.0);
        carEmissive = vec3(1.0, 0.06, 0.03) * tail + vec3(1.0, 0.93, 0.8) * head;
        // Brake disc glow behind the spokes: inner radius band of the wheel.
        if (uCarPart == 1) {
            float r = length(mp.yz) / max(uBodyBounds.w, 1e-4);
            float disc = smoothstep(0.30, 0.38, r) * (1.0 - smoothstep(0.58, 0.66, r)) * (1.0 - role.b);
            carEmissive += vec3(1.0, 0.28, 0.05) * disc * uBrakeGlow * uBrakeGlow * 4.0;
        }
        // Phase 4c: scratched paint (primer, then bare steel in the deepest
        // gouges) and crumpled shading. Applied before dust and mud, so the
        // 4b dirt look carries over, and dirt clings to the scratches.
        if (carDamage > 0.02 && carGlass < 0.5 && role.b < 0.5) {
            vec3 q = mp0 / bodyWidth;
            // Gouges run in patches whose direction wanders with a slow noise
            // (scrapes follow the impact, not the body axes).
            float angle = carNoise(q * 2.0 + 11.0) * 6.2832;
            vec3 dir = normalize(vec3(cos(angle), sin(angle), 0.35 * sin(angle * 1.7)));
            float streak = abs(fract(dot(q, dir) * 70.0 + carNoise(q * 6.0) * 2.0) - 0.5);
            float patch = smoothstep(0.62 - 0.3 * carDamage, 0.78 - 0.3 * carDamage, carNoise(q * 5.0 + 7.0));
            float scratch = (1.0 - smoothstep(0.02, 0.06 + 0.05 * carDamage, streak)) * patch;
            float scuff = smoothstep(0.66 - 0.3 * carDamage, 0.9, carNoise(q * 26.0)) * patch;
            carScratch = clamp((scratch * 0.9 + scuff * 0.6) * smoothstep(0.1, 0.6, carDamage), 0.0, 1.0);
            albedo = mix(albedo, vec3(0.42, 0.43, 0.42), carScratch);
            float bare = carScratch * scratch * step(0.6, carDamage);
            albedo = mix(albedo, vec3(0.56, 0.57, 0.58), bare);
            metallic = mix(metallic, 0.85, bare);
            roughness = mix(roughness, 0.55, carScratch);
            carCrumple = smoothstep(0.2, 0.9, carDamage) * 0.85;
        }
        // Dust/mud: low on the body, sprayed behind the wheels, broken up by noise.
        float height = clamp((mp.z - uBodyBounds.x) / max(uBodyBounds.y - uBodyBounds.x, 1e-3), 0.0, 1.0);
        float splash = 0.55 * sceneNoise(fs.worldPos.xy * 2.3) + 0.45 * sceneNoise(fs.worldPos.yz * 7.1 + fs.worldPos.x);
        float coverage = max(uDirt.x, uDirt.y);
        carDirt = clamp(coverage * 1.6 - height * 1.3 + (splash - 0.5) * 0.9, 0.0, 1.0) * (uCarPart == 1 ? 1.3 : 1.0);
        carDirt = clamp(max(carDirt, carScratch * coverage * 0.8), 0.0, 1.0);
        vec3 dirtColour = mix(vec3(0.46, 0.39, 0.29), vec3(0.17, 0.12, 0.08), uDirt.y / max(coverage, 1e-3));
        albedo = mix(albedo, dirtColour, carDirt * (carGlass > 0.5 ? 0.6 : 0.9));
        roughness = mix(roughness, mix(0.92, 0.55, uDirt.z), carDirt);
        metallic *= 1.0 - carDirt;
    } else if (uGlassPass) discard;
#endif
    // ---- wetness: darker diffuse, smoother surface -------------------------
    albedo    *= mix(1.0, 0.55, uWetness * (1.0 - metallic));
    roughness  = mix(roughness, min(roughness, 0.12), uWetness);

    // ---- tangent-space normal mapping ---------------------------------------
#ifdef TERRAIN
    vec3 N = terrain.normal;
#else
    vec2 nxy = (texture(uNormalMap, fs.uv).xy * 2.0 - 1.0) * uNormalScale;
    vec3 nts = vec3(nxy, sqrt(max(1.0 - dot(nxy, nxy), 0.0)));
    mat3 TBN = mat3(normalize(fs.T), normalize(fs.B), normalize(fs.N));
    vec3 N   = normalize(TBN * nts);
#endif
#ifndef VEGETATION
    if (!gl_FrontFacing) N = -N;                            // two-sided foliage / panels
#ifdef CAR
    if (carCrumple > 0.0) {   // Phase 4c: faceted, creased panels where the body is dented
        vec3 facet = normalize(cross(dFdx(fs.worldPos), dFdy(fs.worldPos)));
        if (dot(facet, N) < 0.0) facet = -facet;
        N = normalize(mix(N, facet, carCrumple));
    }
#endif
#endif                                                      // (plants use bent, view-independent normals)
    vec3 V   = normalize(uCamPos - fs.worldPos);
    float NdotV = max(dot(N, V), 1e-4);

    // ---- scene weather: rain film, terrain puddles, snow cover -------------
    // World-space so cars and plants agree with terrain. Presentation only.
    if (uSceneEnabled) {
        vec3 worldN = normalize(mat3(uSceneFromLocal) * N);
        vec3 worldP = (uSceneFromLocal * vec4(fs.worldPos, 1.0)).xyz;
        float wet = uSceneWeather.x * mix(0.4, 1.0, smoothstep(-0.2, 0.7, worldN.z));
        float puddleWeight = 0.0;
#ifdef TERRAIN
        float field = 0.65 * sceneNoise(worldP.xy * 0.18) + 0.35 * sceneNoise(worldP.xy * 0.71);
        float threshold = 1.0 - 0.75 * uSceneWeather.y;
        float puddle = terrain.puddleable * smoothstep(0.96, 0.995, normalize(fs.N).z)
                     * smoothstep(threshold, threshold + 0.08, field) * step(0.001, uSceneWeather.y);
        puddleWeight = puddle;
        albedo *= mix(1.0, 0.4, puddle);
        roughness = mix(roughness, 0.02, puddle);   // mirror-like: sky IBL + sun glint
        N = normalize(mix(N, vec3(0.0, 0.0, 1.0), puddle));
#endif
        // A water film darkens porous albedo and adds a moderate sheen; only
        // standing water (puddles) becomes a mirror. Leaves stay fairly rough.
#ifdef VEGETATION
        albedo *= mix(1.0, 0.8, wet);
        roughness = mix(roughness, max(roughness * 0.75, 0.45), wet);
#else
        albedo *= mix(1.0, 0.6, wet * (1.0 - metallic));
        roughness = mix(roughness, max(roughness * 0.55, 0.28), wet * (1.0 - puddleWeight));
#endif
        float snow = uSceneWeather.z * smoothstep(0.3, 0.8, worldN.z);
        albedo = mix(albedo, vec3(0.82, 0.85, 0.9), snow);
        roughness = mix(roughness, 0.6, snow);
        metallic *= 1.0 - snow;
    }
    roughness = max(specularAA(N, roughness), 0.04);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);            // dielectric 4% / metal = albedo

    // ---- direct lighting -----------------------------------------------------
    vec3 Lo = evalLight(normalize(uSunDir), uSunColor * sunVisibility(), N, V, NdotV, albedo, metallic, roughness, F0);
#ifdef TERRAIN
    Lo += uSunColor * sunVisibility() * terrainGlint(fs.worldPos, N, V, normalize(uSunDir), terrain.sparkle) * 6.0;
#endif

    for (int i = 0; i < clamp(uPointLightCount, 0, MAX_POINT_LIGHTS); ++i) {            // headlights, fire, flash — keep count small
        vec3  toL  = uPointLightPos[i] - fs.worldPos;
        float dist = max(length(toL), 0.0001);
        float range = max(uPointLightColorRange[i].a, 0.0001);
        float att  = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);   // smooth finite-range window
        att = att * att / (dist * dist + 1.0);
        Lo += evalLight(toL / dist, uPointLightColorRange[i].rgb * att, N, V, NdotV,
                        albedo, metallic, roughness, F0);
    }

#ifdef VEGETATION
    {   // thin-leaf transmission: wrapped back-face term + forward scatter toward the viewer
        vec3 L = normalize(uSunDir);
        float back = 0.6 * max(dot(-N, L), 0.0) + 0.4 * pow(max(dot(V, -L), 0.0), 4.0);
        Lo += uTranslucency * albedo * uSunColor * sunVisibility() * back / PI;
    }
#endif
    // ---- world spot lights (headlights): world space, rotated into this
    //      draw's local frame (rigid model matrices) for the shared BRDF.
    if (uSceneEnabled && !uDepthOnly) {
        vec3 worldP = (uSceneFromLocal * vec4(fs.worldPos, 1.0)).xyz;
        mat3 toLocal = transpose(mat3(uSceneFromLocal));
        for (int i = 0; i < clamp(uSceneLightCount, 0, MAX_SCENE_LIGHTS); ++i) {
            vec3 toLight;
            vec3 radiance = sceneSpot(i, worldP, toLight);
            if (max(radiance.r, max(radiance.g, radiance.b)) > 0.0)
                Lo += evalLight(normalize(toLocal * toLight), radiance, N, V, NdotV, albedo, metallic, roughness, F0);
        }
    }
    // ---- clear-coat lobe (fixed F0 = 0.04) ------------------------------------
    float Fcc = 0.0;
    float clearcoat = uClearcoat;
#ifdef CAR
    if (uCarPart == 0 && carGlass < 0.5) clearcoat = texture(uRoleMap, fs.uv).r * (1.0 - carDirt);
    else clearcoat = 0.0;
#endif
    if (clearcoat > 0.0) {
        vec3  L  = normalize(uSunDir);
        vec3  H  = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0), NdotH = max(dot(N, H), 0.0), VdotH = max(dot(V, H), 0.0);
        float a  = uClearcoatRough * uClearcoatRough;
        Fcc      = clearcoat * F_Schlick(VdotH, vec3(0.04)).x;
        float sc = D_GGX(NdotH, a) * G_Smith(NdotV, NdotL, uClearcoatRough) * Fcc
                   / (4.0 * NdotV * NdotL + 1e-4);
        Lo = Lo * (1.0 - Fcc) + sc * uSunColor * sunVisibility() * NdotL;     // base layer attenuated by coat
    }

    // ---- image based lighting (ambient) --------------------------------------
    vec3 ambient;
    if (uHasIBL) {
        vec3 irradiance = textureLod(uIrradianceMap,uEnvironmentRotation*N,0.0).rgb;
        vec3 R=reflect(-V,N);
        vec3 prefiltered=environmentSpecular(R,roughness);
        vec2 brdf=texture(uBrdfLUT,vec2(NdotV,roughness)).rg;
        float Ess=clamp(brdf.x+brdf.y,0.0001,1.0),Ems=1.0-Ess;
        vec3 FssEss=clamp(F0*brdf.x+brdf.y,0.0,1.0);
        vec3 Favg=F0+(1.0-F0)/21.0;
        // Fdez-Aguera multi-scattering compensation. A white conductor has
        // FssEss=Ess and Fms=1, so roughness cannot lose furnace energy.
        vec3 Fms=FssEss*Favg/max(1.0-Ems*Favg,vec3(0.0001));
        vec3 multiple=Fms*Ems;
        vec3 kD=max(vec3(0),1.0-FssEss-multiple)*(1.0-metallic);
        ambient=prefiltered*FssEss+irradiance*(multiple+kD*albedo);
        if(clearcoat>0.0){vec3 coatN=normalize(fs.N);float coatNv=max(dot(coatN,V),0.0001);vec2 coatBrdf=texture(uBrdfLUT,vec2(coatNv,uClearcoatRough)).rg;float coatWeight=clamp(clearcoat*(0.04*coatBrdf.x+coatBrdf.y),0.0,1.0);ambient=ambient*(1.0-coatWeight)+environmentSpecular(reflect(-V,coatN),uClearcoatRough)*coatWeight;}
        ambient*=ao*uEnvIntensity;

    } else {
        ambient = uAmbientColor * albedo * ao;
    }

    vec3 color = (Lo + ambient) * uExposure;
#ifdef CAR
    color += carEmissive;   // lamps and hot discs are light sources: not shadowed or AO'd
#endif
    if(!uDepthOnly)color=sceneFog(color,(uSceneFromLocal*vec4(fs.worldPos,1)).xyz);

    // ---- tone-map + gamma 2.2 ---------------------------------------------------
    if (uToneMap) {
        color = tonemapACES(color);
        color = pow(color, vec3(1.0 / GAMMA));
    }
    if (uDebugView == 1) color = albedo;
    if (uDebugView == 2) color = N*0.5+0.5;
    if (uDebugView == 3) color = vec3(roughness);
    if (uDebugView == 4) color = vec3(metallic);
    if (uDebugView == 5) color = vec3(ao);
    if (uDebugView == 6) {int axis=int(mod(floor(fs.uv.x*12.0),3.0));color=(axis==0?normalize(fs.T):axis==1?normalize(fs.B):normalize(fs.N))*0.5+0.5;}
#ifdef TERRAIN
    if(uDebugView==7)color=normalize(fs.N)*0.5+0.5;
#endif
#ifdef CAR
    if (uGlassPass) { FragColor = vec4(color, mix(0.86, 0.97, carCrack)); return; }   // tinted glass; cracks scatter
#endif
    FragColor = vec4(color, baseSample.a);
}
