#version 330 core
// ============================================================================
// vegetation.vert — instanced procedural plants + sprite-card impostors.
// Feeds pbr.frag compiled with -DVEGETATION.
// PERF: 24 B static vertex (packed normal/colour/UV) + 32 B per instance; the
//       yaw rotation is a 2x2 on xy (no per-instance matrix). Wind is a few
//       sines per vertex, weighted by height^2 so roots and trunks stay put.
// ============================================================================
layout(location = 0) in vec3 aPosition;   // unit-height plant space
layout(location = 1) in vec4 aNormal;     // GL_INT_2_10_10_10_REV, normalised
layout(location = 2) in vec2 aUV;         // unorm16
layout(location = 5) in vec4 aColor;      // rgb linear albedo, a = AO
layout(location = 6) in vec4 aInstanceA;  // xyz root (world), w height (m)
layout(location = 7) in vec4 aInstanceB;  // x yaw, y fade code, z tint

uniform mat4 uViewProjection;
uniform vec4 uVegetationWind;              // xy direction*strength, z time (s), w flex

out VS_OUT {
    vec3 worldPos;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
    vec4 color;
    flat float fade;
} vs;

void main()
{
    float c = cos(aInstanceB.x), s = sin(aInstanceB.x);
    mat2 yaw = mat2(c, s, -s, c);
    float height = aInstanceA.w;
    vec3 local = aPosition * height;
    local.xy = yaw * local.xy;

    float weight = clamp(aPosition.z, 0.0, 1.0);
    weight *= weight * uVegetationWind.w;
    float phase = dot(aInstanceA.xy, vec2(0.071, 0.113));
    float t = uVegetationWind.z;
    float sway = 0.55 + 0.6 * sin(t * 1.7 + phase) + 0.25 * sin(t * 4.3 + phase * 2.1);
    vec2 offset = uVegetationWind.xy * (sway * weight * height * 0.035);
    local.xy += offset;
    local.z -= 0.5 * dot(offset, offset) / max(local.z, 0.05);   // keep the stem length ~constant

    vec3 n = aNormal.xyz;
    n.xy = yaw * n.xy;
    vs.N = normalize(n);
    vs.T = abs(vs.N.z) < 0.95 ? normalize(cross(vec3(0.0, 0.0, 1.0), vs.N)) : vec3(1.0, 0.0, 0.0);
    vs.B = cross(vs.N, vs.T);
    vs.worldPos = aInstanceA.xyz + local;
    vs.uv = aUV;
    vs.color = vec4(aColor.rgb * aInstanceB.z, aColor.a);
    vs.fade = aInstanceB.y;
    gl_Position = uViewProjection * vec4(vs.worldPos, 1.0);
}
