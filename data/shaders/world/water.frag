#version 330 core
// ============================================================================
// water.frag — PBR stage water. Premultiplied-alpha output (blend ONE,
// ONE_MINUS_SRC_ALPHA): the transmitted lake bed is the framebuffer behind.
//   Reflection : Schlick Fresnel (F0 = 0.02) x prefiltered world sky IBL
//   Glints     : GGX sun lobe (CSM visibility) + world spot lights
//   Body       : Beer-Lambert absorption by depth, single-scatter tint
//   Extras     : shoreline foam, rain ripples (uSceneWeather.x), height fog
// PERF: 4 analytic wave octaves (no normal map fetch), 1-2 cube fetches.
// ============================================================================
in vec3 vWorld;
in float vDepth;
out vec4 outColor;
#include "scene.glsl"
uniform samplerCube uSkyPrefilter;
uniform float uSkyMaxLod = 0.0;
uniform bool uHasSky = false;
uniform vec3 uCamPos;
uniform float uTime = 0.0;
const float PI = 3.14159265359;

vec2 waveSlope(vec2 p, float t, float fade) {
    // Directional sines: (direction, wavelength m, amplitude m, speed m/s)
    const vec4 W[4] = vec4[4](vec4(0.83, 0.55, 9.0, 0.06), vec4(-0.38, 0.92, 5.3, 0.035),
                              vec4(0.97, -0.24, 2.7, 0.018), vec4(-0.71, -0.70, 1.3, 0.008));
    vec2 slope = vec2(0.0);
    for (int i = 0; i < 4; ++i) {
        float k = 2.0 * PI / W[i].z;
        float c = sqrt(9.81 / k);                          // deep-water dispersion
        slope += W[i].w * k * W[i].xy * cos(k * dot(W[i].xy, p) - k * c * t);
        fade *= 0.85;
    }
    return slope * fade * 1.8;                             // visible chop without aliasing
}
float ggx(float NdotH, float a) { float a2 = a * a; float d = NdotH * NdotH * (a2 - 1.0) + 1.0; return a2 / (PI * d * d + 1e-7); }

void main() {
    if (uDepthOnly) { outColor = vec4(0.0); return; }
    float depth = max(vDepth, 0.0);
    vec3 toCam = uCamPos - vWorld;
    float dist = length(toCam);
    vec3 V = toCam / max(dist, 1e-4);

    // Waves fade with distance (no sub-pixel aliasing); rain adds ripples.
    float fade = 1.0 / (1.0 + dist * 0.015);
    vec2 slope = waveSlope(vWorld.xy, uTime, fade);
    float rain = uSceneWeather.x;
    if (rain > 0.0) {
        vec2 q = vWorld.xy * 3.0 + uTime * 0.7;
        slope += (vec2(sceneNoise(q), sceneNoise(q + 17.3)) - 0.5) * 0.35 * rain * fade;
    }
    vec3 N = normalize(vec3(-slope, 1.0));
    float NdotV = max(dot(N, V), 1e-3);
    float F = 0.02 + 0.98 * pow(1.0 - NdotV, 5.0);
    float rough = 0.05 + 0.15 * rain;

    vec3 R = reflect(-V, N);
    R.z = abs(R.z);                                        // never sample below the horizon
    vec3 sky = uHasSky ? textureLod(uSkyPrefilter, R, rough * uSkyMaxLod).rgb
                       : uSolarSkyRadiance * 0.25 + uSceneAmbient;

    // Sun glint (GGX + Schlick at VdotH) with shadow visibility.
    vec3 L = normalize(uSolarDirection);
    vec3 H = normalize(V + L);
    float NdotL = dot(N, L);
    float glintF = 0.02 + 0.98 * pow(1.0 - max(dot(V, H), 0.0), 5.0);
    vec3 spec = uSolarRadiance * sceneVisibility(vWorld, vec3(0, 0, 1))
              * ggx(max(dot(N, H), 0.0), rough * rough) * glintF * step(0.0, NdotL) / (4.0 * NdotV);   // D*F/(4 NdotV), G ~ 1 for a smooth surface
    for (int i = 0; i < clamp(uSceneLightCount, 0, MAX_SCENE_LIGHTS); ++i) {
        vec3 toLight;
        vec3 radiance = sceneSpot(i, vWorld, toLight);
        vec3 Hs = normalize(V + toLight);
        float nl = dot(N, toLight);
        spec += radiance * ggx(max(dot(N, Hs), 0.0), rough * rough) * 0.02 * step(0.0, nl) / (4.0 * NdotV);
    }

    // Body: absorption by depth; single-scatter colour lit by sky and sun.
    float transmittance = exp(-depth * 0.55);
    vec3 scatter = vec3(0.012, 0.045, 0.05) * (uSceneAmbient * 4.0 + uSolarRadiance * max(L.z, 0.0) * 0.35);
    float body = (1.0 - transmittance) * 0.9;

    // Shoreline foam where the water is a few centimetres deep.
    float foam = (1.0 - smoothstep(0.02, 0.35, depth)) * smoothstep(0.35, 0.75, sceneNoise(vWorld.xy * 1.7 + uTime * 0.15));
    vec3 foamColour = vec3(0.8) * (uSceneAmbient * 2.0 + uSolarRadiance * max(L.z, 0.0) / PI);

    vec3 straight = mix(scatter, foamColour, foam);
    float coverage = clamp(body + foam * 0.8, 0.0, 1.0);
    // Premultiplied composition: body + reflection (reflection adds coverage).
    vec3 colour = straight * coverage * (1.0 - F) + sky * F + spec;
    float alpha = clamp(coverage * (1.0 - F) + F, 0.0, 1.0);
    alpha *= smoothstep(-0.02, 0.02, vDepth);               // soft edge at the shore line

    vec3 shaded = colour / max(alpha, 1e-3);
    shaded = sceneFog(shaded, vWorld);
    if (uSceneToneMap) { shaded = clamp((shaded * (2.51 * shaded + 0.03)) / (shaded * (2.43 * shaded + 0.59) + 0.14), 0.0, 1.0); shaded = pow(shaded, vec3(1.0 / 2.2)); }
    outColor = vec4(shaded * alpha, alpha);
}
