#version 330 core
// 4x4 depth-aware box blur of the half-resolution AO (removes the rotation
// pattern without bleeding across silhouettes).
in vec2 vUv;
out float outAo;
uniform sampler2D uAo, uDepth;
uniform vec2 uTexel;              // AO texel size
uniform mat4 uInverseProjection;
float viewZ(vec2 uv) {
    vec4 p = uInverseProjection * vec4(vec3(uv, textureLod(uDepth, uv, 0.0).r) * 2.0 - 1.0, 1.0);
    return p.z / p.w;
}
void main() {
    float centre = viewZ(vUv), sum = 0.0, weight = 0.0;
    for (int y = -2; y < 2; ++y)
        for (int x = -2; x < 2; ++x) {
            vec2 uv = vUv + (vec2(x, y) + 0.5) * uTexel;
            float w = 1.0 / (1.0 + abs(viewZ(uv) - centre) * 4.0);
            sum += textureLod(uAo, uv, 0.0).r * w;
            weight += w;
        }
    outAo = sum / max(weight, 1e-4);
}
