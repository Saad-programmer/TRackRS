#version 330 core
// ============================================================================
// props.vert — instanced Saharan props and animals (amendment A3.5).
// Feeds pbr.frag compiled with -DVEGETATION (shared LOD dither fade; normals
// are real geometry normals here, translucency 0).
// Mesh: metres, +x forward, +z up. uv.x tags animal legs (.25 FL, .5 FR,
// .75 BL, 1 BR), uv.y = 1 marks head/neck (gentle grazing nod).
// Instance A: position (world), scale. Instance B: yaw, fade code, tint, stride.
// PERF: one sin per leg vertex; rigid props skip all animation.
// ============================================================================
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 5) in vec4 aColor;      // linear albedo, a = AO
layout(location = 6) in vec4 aInstanceA;  // position, scale
layout(location = 7) in vec4 aInstanceB;  // yaw, fade code, tint 0..7, stride 0..1 (animals)
uniform mat4 uViewProjection;
uniform float uTime;
uniform int uAnimal;                      // 1 for camels and goats
uniform float uHip;                       // animals: leg-top height (m, unit scale)
uniform int uSign;                        // 1: sign faces, sample the sign atlas at aUV
out VS_OUT { vec3 worldPos; vec2 uv; vec3 T; vec3 B; vec3 N; vec4 color; flat float fade; } vs;

// Per-instance colour variation (multipliers): camel coats from pale to dark,
// goats black/brown/white, pisé shades, house paints. Authored for TrackRS.
const vec3 ANIMAL_TINT[8] = vec3[8](vec3(1.0), vec3(1.25,1.2,1.1), vec3(0.75,0.7,0.65), vec3(1.4,1.35,1.3),
                                    vec3(0.5,0.45,0.45), vec3(4.0,3.9,3.7), vec3(1.1,0.9,0.75), vec3(2.2,1.8,1.4));
const vec3 BUILT_TINT[8] = vec3[8](vec3(1.0), vec3(1.06,1.03,1.0), vec3(0.94,0.92,0.9), vec3(1.1,1.0,0.92),
                                   vec3(0.97,1.0,1.04), vec3(1.0,0.95,0.88), vec3(0.9,0.88,0.86), vec3(1.04,1.04,1.04));

void main() {
    vec3 p = aPosition;
    vec3 n = aNormal.xyz;
    int tint = int(aInstanceB.z);
    if (uAnimal == 1) {
        float stride = aInstanceB.w;
        if (aUV.x > 0.1) {
            // Walk cycle: diagonal leg pairs swing in opposition about the hip.
            int leg = int(round(aUV.x * 4.0));          // 1 FL, 2 FR, 3 BL, 4 BR
            float pair = (leg == 1 || leg == 4) ? 0.0 : 3.14159265;
            float swing = 0.35 * sin(stride * 6.2831853 + pair) * step(0.001, stride);
            // Rotate about the hip (leg top, uHip): each leg vertex keeps its
            // x/y and swings by its depth below the hip.
            float below = p.z - uHip;
            p = vec3(p.x + below * sin(swing), p.y, uHip + below * cos(swing));
        }
        if (aUV.y > 0.5) // grazing nod
            p.z -= 0.12 * max(sin(uTime * 0.7 + aInstanceA.x), 0.0) * smoothstep(0.0, 1.5, p.x);
    }
    float scale = aInstanceA.w;
    float cy = cos(aInstanceB.x), sy = sin(aInstanceB.x);
    mat2 yaw = mat2(cy, sy, -sy, cy);
    vec3 local = p * scale;
    local.xy = yaw * local.xy;
    n.xy = yaw * n.xy;
    vec3 colour = uSign == 1 ? aColor.rgb : aColor.rgb * (uAnimal == 1 ? ANIMAL_TINT[tint] : BUILT_TINT[tint]);
    vs.N = normalize(n);
    vs.T = abs(vs.N.z) < 0.95 ? normalize(cross(vec3(0.0, 0.0, 1.0), vs.N)) : vec3(1.0, 0.0, 0.0);
    vs.B = cross(vs.N, vs.T);
    vs.worldPos = aInstanceA.xyz + local;
    vs.uv = uSign == 1 ? aUV : vec2(0.5);
    vs.color = vec4(min(colour, vec3(0.85)), aColor.a);
    vs.fade = aInstanceB.y;
    gl_Position = uViewProjection * vec4(vs.worldPos, 1.0);
}
