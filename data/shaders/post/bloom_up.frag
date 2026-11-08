#version 330 core
// Dual-filter upsample (8-tap tent); additively blended onto the next level up.
in vec2 vUv;
out vec3 outColor;
uniform sampler2D uSource;
uniform vec2 uTexel;               // source (smaller) texel size
uniform float uWeight;
void main() {
    vec2 o = uTexel;
    vec3 sum = texture(uSource, vUv + vec2(-2.0 * o.x, 0.0)).rgb + texture(uSource, vUv + vec2(2.0 * o.x, 0.0)).rgb
             + texture(uSource, vUv + vec2(0.0, -2.0 * o.y)).rgb + texture(uSource, vUv + vec2(0.0, 2.0 * o.y)).rgb
             + 2.0 * (texture(uSource, vUv + vec2(-o.x, -o.y)).rgb + texture(uSource, vUv + vec2(o.x, -o.y)).rgb
                    + texture(uSource, vUv + vec2(-o.x, o.y)).rgb + texture(uSource, vUv + vec2(o.x, o.y)).rgb);
    outColor = sum / 12.0 * uWeight;
}
