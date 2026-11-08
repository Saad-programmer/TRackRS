#version 330 core
// Dual-filter downsample (centre x4 + 4 diagonal taps, /8). The first level
// applies the soft-knee bright pass (mirrors trackrs::post::bloomPrefilter) and
// a Karis luma weight that suppresses single-pixel fireflies.
in vec2 vUv;
out vec3 outColor;
uniform sampler2D uSource;
uniform vec2 uTexel;               // source texel size
uniform bool uPrefilter;
uniform float uThreshold, uKnee;
vec3 prefilter(vec3 c) {
    float b = max(c.r, max(c.g, c.b));
    float soft = clamp(b - uThreshold + uKnee, 0.0, 2.0 * uKnee);
    soft = soft * soft / (4.0 * uKnee + 1e-5);
    return c * max(soft, b - uThreshold) / max(b, 1e-5);
}
vec3 tap(vec2 uv) {
    vec3 c = texture(uSource, uv).rgb;
    if (uPrefilter) { c = prefilter(min(c, vec3(64.0))); c /= 1.0 + dot(c, vec3(0.2126, 0.7152, 0.0722)); }
    return c;
}
void main() {
    vec2 o = uTexel;
    vec3 sum = tap(vUv) * 4.0 + tap(vUv + vec2(-o.x, -o.y)) + tap(vUv + vec2(o.x, -o.y))
             + tap(vUv + vec2(-o.x, o.y)) + tap(vUv + vec2(o.x, o.y));
    vec3 c = sum / 8.0;
    if (uPrefilter) c /= max(1.0 - dot(c, vec3(0.2126, 0.7152, 0.0722)), 1e-3);   // undo Karis weight
    outColor = c;
}
