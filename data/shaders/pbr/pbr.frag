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
} fs;

out vec4 FragColor;

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

// CSM interface only. Phase 2c does not allocate or bake shadow cascades.
#ifdef USE_CSM
uniform sampler2DArrayShadow uShadowMap;
uniform mat4 uLightViewProj[4];
uniform vec4 uCascadeSplits;
uniform mat4 uView;
uniform int uCascadeCount;
uniform bool uShadowEnabled;
float sunVisibility() {
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
float sunVisibility() { return 1.0; }
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
void main()
{
    // ---- fetch material (3 texture reads for the whole surface) -----------
    vec4 baseSample = texture(uAlbedoMap, fs.uv) * uBaseColorFactor;
    #ifdef ALPHA_TO_COVERAGE
    if (uAlphaCutoff > 0.0) baseSample.a = clamp((baseSample.a-uAlphaCutoff)/max(fwidth(baseSample.a),0.001)+0.5,0.0,1.0);
#else
    if (uAlphaCutoff > 0.0 && baseSample.a < uAlphaCutoff) discard;
#endif

    vec3 albedo = baseSample.rgb * fs.color.rgb;            // vertex colour = baked wear/AO tint (white if unused)
    vec3 mr     = texture(uMetalRoughMap, fs.uv).rgb;
    float ao        = mix(1.0, mr.r, uAOStrength) * fs.color.a;
    float roughness = clamp(mr.g * uRoughnessFactor, 0.04, 1.0);
    float metallic  = clamp(mr.b * uMetallicFactor, 0.0, 1.0);

    // ---- wetness: darker diffuse, smoother surface -------------------------
    albedo    *= mix(1.0, 0.55, uWetness * (1.0 - metallic));
    roughness  = mix(roughness, min(roughness, 0.12), uWetness);

    // ---- tangent-space normal mapping ---------------------------------------
    vec2 nxy = (texture(uNormalMap, fs.uv).xy * 2.0 - 1.0) * uNormalScale;
    vec3 nts = vec3(nxy, sqrt(max(1.0 - dot(nxy, nxy), 0.0)));
    mat3 TBN = mat3(normalize(fs.T), normalize(fs.B), normalize(fs.N));
    vec3 N   = normalize(TBN * nts);
    if (!gl_FrontFacing) N = -N;                            // two-sided foliage / panels
    vec3 V   = normalize(uCamPos - fs.worldPos);
    float NdotV = max(dot(N, V), 1e-4);

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
        vec3 F  = F_SchlickRoughness(NdotV, F0, roughness);
        vec3 kD = (1.0 - F) * (1.0 - metallic);
        vec3 irradiance = texture(uIrradianceMap, N).rgb;
        vec3 R = reflect(-V, N);
        vec3 prefiltered = textureLod(uPrefilterMap, R, roughness * uPrefilterMaxLod).rgb;
        vec2 brdf = texture(uBrdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specIBL = prefiltered * (F * brdf.x + brdf.y);

        // clear-coat reflection: sharp mip, own Fresnel
        if (uClearcoat > 0.0) {
            vec3  Rc  = reflect(-V, normalize(fs.N));       // coat ignores the base normal map: glassy orange-peel-free
            vec3  cc  = textureLod(uPrefilterMap, Rc, uClearcoatRough * uPrefilterMaxLod).rgb;
            float fc  = uClearcoat * F_SchlickRoughness(NdotV, vec3(0.04), uClearcoatRough).x;
            specIBL   = specIBL * (1.0 - fc) + cc * fc;
            kD       *= (1.0 - fc);
        }
        ambient = (kD * irradiance * albedo + specIBL) * ao * uEnvIntensity;
    } else {
        ambient = uAmbientColor * albedo * ao;
    }

    vec3 color = (Lo + ambient) * uExposure;

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
    FragColor = vec4(color, baseSample.a);
}
