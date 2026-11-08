#version 330 core
// ============================================================================
// ssao.frag — half-resolution screen-space ambient occlusion from depth only.
// View-space position from the depth texture + inverse projection; normal from
// position derivatives; 12-tap hemisphere kernel (CPU-generated, uKernel) with
// a per-pixel rotation; range-checked so distant geometry does not darken.
// PERF: quarter-pixel count, 13 depth fetches, no normal buffer.
// ============================================================================
in vec2 vUv;
out float outAo;
uniform sampler2D uDepth;
uniform mat4 uProjection, uInverseProjection;
uniform vec3 uKernel[12];
uniform float uRadius;
vec3 viewPosition(vec2 uv) {
    float d = textureLod(uDepth, uv, 0.0).r;
    vec4 p = uInverseProjection * vec4(vec3(uv, d) * 2.0 - 1.0, 1.0);
    return p.xyz / p.w;
}
void main() {
    float depth = textureLod(uDepth, vUv, 0.0).r;
    if (depth >= 1.0) { outAo = 1.0; return; }            // sky
    vec3 p = viewPosition(vUv);
    vec3 n = normalize(cross(dFdx(p), dFdy(p)));
    if (dot(n, p) > 0.0) n = -n;                           // face the camera
    // Interleaved-gradient rotation around the normal (4x4 blur removes it).
    float angle = 6.2831853 * fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    vec3 helper = abs(n.z) < 0.9 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 t = normalize(cross(helper, n));
    vec3 b = cross(n, t);
    float c = cos(angle), s = sin(angle);
    vec3 tr = t * c + b * s, br = -t * s + b * c;
    float radius = uRadius * clamp(-p.z * 0.02 + 0.5, 0.5, 4.0); // grow slowly with distance
    float occlusion = 0.0;
    for (int i = 0; i < 12; ++i) {
        vec3 k = uKernel[i];
        vec3 q = p + (tr * k.x + br * k.y + n * k.z) * radius;
        vec4 clip = uProjection * vec4(q, 1.0);
        vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
        if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) continue;
        float sceneZ = viewPosition(uv).z;
        float range = smoothstep(0.0, 1.0, radius / max(abs(p.z - sceneZ), 1e-3));
        occlusion += (sceneZ >= q.z + 0.03 * radius ? 1.0 : 0.0) * range;
    }
    outAo = 1.0 - occlusion / 12.0;
}
