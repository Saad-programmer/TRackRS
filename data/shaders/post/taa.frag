#version 330 core
// Phase 3e temporal anti-aliasing resolve (linear HDR in, linear HDR out).
// History is reprojected per pixel: cars carry an explicit motion vector
// (uVelocity, written by the car velocity pass, b = 1); everything else is
// static scenery, reprojected from depth through the previous unjittered
// view-projection. The history is clamped to the current 3x3 neighbourhood
// in YCoCg (removes ghosting and disocclusion trails) and blended with
// luminance-weighted (Karis) feedback so HDR highlights do not flicker.
// Screen-door LOD dither, whose pattern rotates every frame, is averaged
// here into a smooth cross-fade.
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uCurrent, uHistory, uDepth, uVelocity;
uniform mat4 uInverseViewProjection;   // current, jittered
uniform mat4 uPreviousViewProjection;  // previous, unjittered
uniform vec2 uTexel;
uniform float uDepthScale;             // glDepthRange far (depth is stored * this)
uniform float uFeedback;               // history weight
uniform bool uReset;

vec3 toYCoCg(vec3 c) {
    return vec3(0.25 * c.r + 0.5 * c.g + 0.25 * c.b, 0.5 * c.r - 0.5 * c.b, -0.25 * c.r + 0.5 * c.g - 0.25 * c.b);
}
vec3 fromYCoCg(vec3 c) {
    return vec3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}
float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

void main() {
    vec3 current = max(textureLod(uCurrent, vUv, 0.0).rgb, vec3(0.0));
    if (uReset) { outColor = vec4(current, 1.0); return; }
    // Neighbourhood statistics (YCoCg box, compressed by a Reinhard weight so a
    // single HDR highlight does not blow the box open).
    vec3 lo = vec3(1e9), hi = vec3(-1e9);
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x) {
            vec3 s = toYCoCg(max(textureLod(uCurrent, vUv + vec2(x, y) * uTexel, 0.0).rgb, vec3(0.0)));
            s /= 1.0 + s.x;
            lo = min(lo, s);
            hi = max(hi, s);
        }
    // Where did this surface point lie last frame?
    vec4 motion = textureLod(uVelocity, vUv, 0.0);
    vec2 previousUv;
    if (motion.b > 0.5) {
        previousUv = vUv + motion.xy * 0.5;
    } else {
        float depth = clamp(textureLod(uDepth, vUv, 0.0).r / uDepthScale, 0.0, 1.0);
        vec4 world = uInverseViewProjection * vec4(vUv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
        world /= world.w;
        vec4 previous = uPreviousViewProjection * world;
        previousUv = previous.w > 0.0 ? previous.xy / previous.w * 0.5 + 0.5 : vec2(-1.0);
    }
    if (any(lessThan(previousUv, vec2(0.0))) || any(greaterThan(previousUv, vec2(1.0)))) {
        outColor = vec4(current, 1.0);   // disocclusion at the screen edge
        return;
    }
    vec3 history = toYCoCg(max(textureLod(uHistory, previousUv, 0.0).rgb, vec3(0.0)));
    history /= 1.0 + history.x;
    history = clamp(history, lo, hi);
    history /= max(1.0 - history.x, 1e-4);              // undo the Reinhard weight
    vec3 historyRgb = max(fromYCoCg(history), vec3(0.0));
    // Karis luminance weighting of the blend.
    float wc = (1.0 - uFeedback) / (1.0 + luma(current));
    float wh = uFeedback / (1.0 + luma(historyRgb));
    outColor = vec4((current * wc + historyRgb * wh) / (wc + wh), 1.0);
}
