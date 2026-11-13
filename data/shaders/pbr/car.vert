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
uniform vec4 uDamage;              // Phase 4c: corner damage FL, FR, RL, RR (0..1)
uniform vec4 uBodyExtent;          // body model space: minX, maxX, minY (rear), maxY (front)
uniform vec4 uBodyBounds;          // minZ, maxZ, frontY, wheel radius
out VS_OUT { vec3 worldPos; vec2 uv; vec3 T; vec3 B; vec3 N; vec4 color; vec3 modelPos; vec3 modelNormal; } vs;
vec3 rotateNeedle(vec3 p, vec3 pivot, float a) {
    // Needles lie in the dash plane (x-z), rotating about the +y axis.
    vec3 d = p - pivot; float c = cos(a), s = sin(a);
    return pivot + vec3(d.x * c + d.z * s, d.y, -d.x * s + d.z * c);
}
// Mirrors trackrs::car::damageAt (engine/car/damage_visual.cpp): keep in step.
float carDamageAt(vec2 xy) {
    vec2 size = max(uBodyExtent.yw - uBodyExtent.xz, vec2(1e-4));
    vec2 corners[4] = vec2[4](vec2(uBodyExtent.x, uBodyExtent.w), vec2(uBodyExtent.y, uBodyExtent.w),
                              vec2(uBodyExtent.x, uBodyExtent.z), vec2(uBodyExtent.y, uBodyExtent.z));
    float strongest = 0.0;
    for (int i = 0; i < 4; ++i)
        strongest = max(strongest, uDamage[i] * (1.0 - smoothstep(0.12, 0.7, length((xy - corners[i]) / size))));
    return strongest;
}
float carHash(vec3 p) { return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453); }
float carNoise(vec3 p) {
    vec3 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(carHash(i), carHash(i + vec3(1, 0, 0)), f.x), mix(carHash(i + vec3(0, 1, 0)), carHash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(carHash(i + vec3(0, 0, 1)), carHash(i + vec3(1, 0, 1)), f.x), mix(carHash(i + vec3(0, 1, 1)), carHash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}
void main() {
    vec3 p = aPosition;
    vec3 n = aNormal;
    if (uCarPart == 0) {
        // Crumple: push the panel in along its normal, broken up by noise so
        // the dent is creased, not a smooth scoop. Up to ~8 % of the width
        // (~13 cm on a 1.6 m body) at a fully smashed corner.
        float d = carDamageAt(p.xy);
        if (d > 0.0) {
            float width = uBodyExtent.y - uBodyExtent.x;
            vec3 q = p / max(width, 1e-4);
            float crease = 0.35 + 0.65 * carNoise(q * 9.0) * carNoise(q * 23.0 + 3.1);
            p -= normalize(n) * d * crease * 0.08 * width;
            // A hanging bumper corner: only the bumper ends (outer 15 % of the
            // length) and their lowest quarter sag, so sills and arches keep
            // their shape (borrowed meshes are not watertight there).
            float low = 1.0 - smoothstep(0.0, 0.25, (p.z - uBodyBounds.x) / max(uBodyBounds.y - uBodyBounds.x, 1e-4));
            float along = abs((p.y - uBodyExtent.z) / max(uBodyExtent.w - uBodyExtent.z, 1e-4) * 2.0 - 1.0);
            float bumper = smoothstep(0.7, 0.9, along);
            p.z -= low * bumper * smoothstep(0.75, 1.0, d) * 0.05 * width;
        }
    }
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
