#version 330 core
// Final HDR resolve: exposure (auto or fixed) + bloom -> ACES fit -> gamma 2.2,
// light vignette. Output goes to the default framebuffer before the HUD.
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uHdr, uBloom, uAdapted, uAo;
uniform float uAoStrength;        // 0 disables ambient occlusion
uniform bool uAutoExposure;
uniform float uKey, uMinExposure, uMaxExposure, uBloomIntensity, uVignette;
uniform int uDebugView;            // 1 = exposure-only (no bloom), 2 = bloom only, 3 = AO
uniform vec2 uTexel;
// Desert atmosphere (amendment A3.4). uDesert: x heat, y dust, z time (s).
uniform vec4 uDesert;
uniform sampler2D uDepth;
uniform mat4 uInverseViewProjection;   // camera-relative: rotation + projection only
uniform mat4 uViewProjectionRotation;
uniform vec2 uDepthRange;              // x near, y far (perspective), z unused
uniform vec2 uWindScreen;              // projected downwind direction on screen
float desertNoise(vec2 p) {
    vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    float a = fract(sin(dot(i, vec2(127.1, 311.7))) * 43758.5453), b = fract(sin(dot(i + vec2(1, 0), vec2(127.1, 311.7))) * 43758.5453);
    float c = fract(sin(dot(i + vec2(0, 1), vec2(127.1, 311.7))) * 43758.5453), d = fract(sin(dot(i + vec2(1, 1), vec2(127.1, 311.7))) * 43758.5453);
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
float viewDistance(vec2 uv) {
    float z = textureLod(uDepth, uv, 0.0).r * 2.0 - 1.0;
    return 2.0 * uDepthRange.x * uDepthRange.y / max(uDepthRange.y + uDepthRange.x - z * (uDepthRange.y - uDepthRange.x), 1e-4);
}
uniform float uSharpen;            // > 0 while TAA runs: restores resolve softness
void main() {
    vec2 uv = vUv;
    float mirageWeight = 0.0;
    vec3 mirageSky = vec3(0.0);
    if (uDesert.x > 0.0) {
        vec4 farPoint = uInverseViewProjection * vec4(vUv * 2.0 - 1.0, 1.0, 1.0);
        vec3 dir = normalize(farPoint.xyz / farPoint.w);
        float dist = viewDistance(vUv);
        // Heat haze: refraction shimmer growing with distance and heat, strongest
        // just above hot ground near the horizon; none on the near car or the sky.
        float haze = uDesert.x * smoothstep(25.0, 400.0, dist) * (1.0 - smoothstep(0.02, 0.25, abs(dir.z)));
        vec2 wobble = vec2(desertNoise(vec2(vUv.x * 90.0, vUv.y * 160.0 - uDesert.z * 3.1)),
                           desertNoise(vec2(vUv.x * 110.0 + 7.3, vUv.y * 140.0 - uDesert.z * 2.3))) - 0.5;
        uv += wobble * uTexel * 3.2 * haze;
        // Mirage: on distant flat ground just below the horizon a hot air layer
        // bends sky light upwards, so the sky appears reflected about the horizon.
        mirageWeight = uDesert.x * smoothstep(-0.045, -0.006, dir.z) * (1.0 - smoothstep(-0.003, 0.0, dir.z))
                     * smoothstep(250.0, 900.0, dist) * (0.7 + 0.3 * desertNoise(vec2(vUv.x * 60.0, uDesert.z * 4.0)));
        if (mirageWeight > 0.0) {
            vec4 m = uViewProjectionRotation * vec4(dir.xy, -dir.z, 1.0);
            mirageSky = textureLod(uHdr, clamp(m.xy / m.w * 0.5 + 0.5, vec2(0.0), vec2(1.0)), 0.0).rgb;
        }
    }
    vec3 hdr = textureLod(uHdr, uv, 0.0).rgb;
    hdr = mix(hdr, mirageSky * 0.92, clamp(mirageWeight, 0.0, 0.85));
    if (uSharpen > 0.0) {   // 4-tap unsharp mask, clamped so it cannot ring negative
        vec3 n = textureLod(uHdr, vUv + vec2(0.0, uTexel.y), 0.0).rgb + textureLod(uHdr, vUv - vec2(0.0, uTexel.y), 0.0).rgb
               + textureLod(uHdr, vUv + vec2(uTexel.x, 0.0), 0.0).rgb + textureLod(uHdr, vUv - vec2(uTexel.x, 0.0), 0.0).rgb;
        hdr = max(hdr + uSharpen * (hdr - 0.25 * n), vec3(0.0));
    }
    hdr *= mix(1.0, textureLod(uAo, vUv, 0.0).r, uAoStrength);
    vec3 bloom = textureLod(uBloom, vUv, 0.0).rgb;
    float exposure = 1.0;
    if (uAutoExposure) {
        float adapted = texelFetch(uAdapted, ivec2(0), 0).r;
        exposure = adapted > 0.0 ? clamp(uKey / adapted, uMinExposure, uMaxExposure) : 1.0;
    }
    vec3 c = uDebugView == 2 ? bloom * uBloomIntensity
           : hdr * exposure + (uDebugView == 1 ? vec3(0.0) : bloom * uBloomIntensity * exposure);
    if (uDesert.y > 0.0) {
        // Chergui: blowing sand streaks travel downwind across the view, an
        // orange grade and lost contrast. Screen-space, presentation only.
        vec2 along = normalize(uWindScreen + vec2(1e-4, 0.0));
        vec2 across = vec2(-along.y, along.x);
        vec2 p = vec2(dot(vUv, along) * 9.0 - uDesert.z * 5.5, dot(vUv, across) * 140.0);
        float streaks = smoothstep(0.62, 0.95, desertNoise(p)) * 0.6
                      + smoothstep(0.7, 0.97, desertNoise(p * vec2(1.7, 0.6) + 13.1 - vec2(uDesert.z * 3.0, 0.0))) * 0.4;
        float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
        c = mix(c, l * vec3(1.3, 0.92, 0.62), 0.55 * uDesert.y);
        c = mix(c, vec3(l * 0.9 + 0.08) * vec3(1.15, 0.86, 0.6), 0.25 * uDesert.y);
        c += vec3(0.85, 0.6, 0.38) * streaks * 0.35 * uDesert.y * exposure;
    }
    c = clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0);
    c = pow(c, vec3(1.0 / 2.2));
    vec2 d = vUv - 0.5;
    c *= 1.0 - uVignette * smoothstep(0.35, 0.85, length(d * vec2(1.0, 0.8)) * 1.4142);
    if (uDebugView == 3) c = vec3(textureLod(uAo, vUv, 0.0).r);   // raw AO, not tone-mapped
    outColor = vec4(c, 1.0);
}
