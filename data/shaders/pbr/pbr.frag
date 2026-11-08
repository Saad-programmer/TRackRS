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
#ifdef VEGETATION
uniform bool  uAlbedoGamma     = false; // legacy UNORM sprite cards: decode sRGB here
uniform float uTranslucency    = 0.0;   // back-lit leaf transmission weight
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
    float lodNoise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
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
    if (uClearcoat > 0.0) {
        vec3  L  = normalize(uSunDir);
        vec3  H  = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0), NdotH = max(dot(N, H), 0.0), VdotH = max(dot(V, H), 0.0);
        float a  = uClearcoatRough * uClearcoatRough;
        Fcc      = uClearcoat * F_Schlick(VdotH, vec3(0.04)).x;
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
        if(uClearcoat>0.0){vec3 coatN=normalize(fs.N);float coatNv=max(dot(coatN,V),0.0001);vec2 coatBrdf=texture(uBrdfLUT,vec2(coatNv,uClearcoatRough)).rg;float coatWeight=clamp(uClearcoat*(0.04*coatBrdf.x+coatBrdf.y),0.0,1.0);ambient=ambient*(1.0-coatWeight)+environmentSpecular(reflect(-V,coatN),uClearcoatRough)*coatWeight;}
        ambient*=ao*uEnvIntensity;

    } else {
        ambient = uAmbientColor * albedo * ao;
    }

    vec3 color = (Lo + ambient) * uExposure;
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
    FragColor = vec4(color, baseSample.a);
}
