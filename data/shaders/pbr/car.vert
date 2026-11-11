#version 330 core
// ============================================================================
// car.vert — Phase 4b CarRenderer: body / wheel (VertexPacked, 32 B) and the
// procedural interior (24 B vegetation-style vertices, animated gauges).
// Feeds pbr.frag compiled with -DCAR. World-space varyings; model-space
// position/normal are passed for light placement and dirt masks.
// PERF: one mat4 + one mat3 per draw; needles rotate with one sin/cos pair.
// ============================================================================
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aTangent;
layout(location = 5) in vec4 aColor;
uniform mat4 uModel, uViewProj;
uniform mat3 uNormalMatrix;
uniform int uCarPart;              // 0 body, 1 wheel, 2 interior
uniform vec3 uTachoPivot, uSpeedPivot;
uniform vec2 uNeedles;             // tacho, speed angles (radians, 0 = up)
out VS_OUT { vec3 worldPos; vec2 uv; vec3 T; vec3 B; vec3 N; vec4 color; vec3 modelPos; vec3 modelNormal; } vs;
vec3 rotateNeedle(vec3 p, vec3 pivot, float a) {
    // Needles lie in the dash plane (x-z), rotating about the +y axis.
    vec3 d = p - pivot; float c = cos(a), s = sin(a);
    return pivot + vec3(d.x * c + d.z * s, d.y, -d.x * s + d.z * c);
}
void main() {
    vec3 p = aPosition;
    vec3 n = aNormal;
    if (uCarPart == 2 && aUV.x > 0.6) {
        bool tacho = aUV.x > 0.9;
        float a = tacho ? uNeedles.x : uNeedles.y;
        p = rotateNeedle(p, tacho ? uTachoPivot : uSpeedPivot, a);
        n = rotateNeedle(n, vec3(0.0), a);
    }
    vec4 world = uModel * vec4(p, 1.0);
    vs.worldPos = world.xyz;
    vs.uv = aUV;
    vs.N = normalize(uNormalMatrix * n);
    vec3 t = uCarPart == 2 ? (abs(vs.N.z) < 0.95 ? cross(vec3(0.0, 0.0, 1.0), vs.N) : vec3(1.0, 0.0, 0.0))
                           : uNormalMatrix * aTangent.xyz;
    vs.T = normalize(t - dot(t, vs.N) * vs.N);
    vs.B = cross(vs.N, vs.T) * (uCarPart == 2 ? 1.0 : (aTangent.w < 0.0 ? -1.0 : 1.0));
    vs.color = uCarPart == 2 ? vec4(aColor.rgb, 1.0) : vec4(1.0);
    vs.modelPos = p;
    vs.modelNormal = n;
    gl_Position = uViewProj * world;
}
