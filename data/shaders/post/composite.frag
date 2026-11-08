#version 330 core
// Final HDR resolve: exposure (auto or fixed) + bloom -> ACES fit -> gamma 2.2,
// light vignette. Output goes to the default framebuffer before the HUD.
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uHdr, uBloom, uAdapted;
uniform bool uAutoExposure;
uniform float uKey, uMinExposure, uMaxExposure, uBloomIntensity, uVignette;
uniform int uDebugView;            // 1 = exposure-only (no bloom), 2 = bloom only
void main() {
    vec3 hdr = textureLod(uHdr, vUv, 0.0).rgb;
    vec3 bloom = textureLod(uBloom, vUv, 0.0).rgb;
    float exposure = 1.0;
    if (uAutoExposure) {
        float adapted = texelFetch(uAdapted, ivec2(0), 0).r;
        exposure = adapted > 0.0 ? clamp(uKey / adapted, uMinExposure, uMaxExposure) : 1.0;
    }
    vec3 c = uDebugView == 2 ? bloom * uBloomIntensity
           : hdr * exposure + (uDebugView == 1 ? vec3(0.0) : bloom * uBloomIntensity * exposure);
    c = clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0);
    c = pow(c, vec3(1.0 / 2.2));
    vec2 d = vUv - 0.5;
    c *= 1.0 - uVignette * smoothstep(0.35, 0.85, length(d * vec2(1.0, 0.8)) * 1.4142);
    outColor = vec4(c, 1.0);
}
