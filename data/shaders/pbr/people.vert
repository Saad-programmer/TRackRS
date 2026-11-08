#version 330 core
// ============================================================================
// people.vert — instanced spectators, camera operators, tripods, reporters.
// Feeds pbr.frag compiled with -DVEGETATION (shared dither LOD fade, two-sided
// lighting with view-independent normals; translucency is set to 0).
// Vertex UV encodes the animation part (x) and the colour region (y); the
// instance carries palette + pose and the per-frame excitement (car nearby).
// PERF: arms are rotated about the shoulder with one sin/cos pair per vertex.
// ============================================================================
layout(location = 0) in vec3 aPosition;   // unit-height figure, facing +x
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec2 aUV;         // x: 0 body, .25 left arm, .5 right arm; y: colour region
layout(location = 5) in vec4 aColor;      // fixed colour (cameras, shoes), a = AO
layout(location = 6) in vec4 aInstanceA;  // feet position (world), height (m)
layout(location = 7) in vec4 aInstanceB;  // yaw, fade code, palette + 8 * pose, excitement
uniform mat4 uViewProjection;
uniform float uTime;
out VS_OUT { vec3 worldPos; vec2 uv; vec3 T; vec3 B; vec3 N; vec4 color; flat float fade; } vs;

// Original clothing palettes (linear albedo): outdoor jackets, hi-vis, team colours.
const vec3 JACKET[8] = vec3[8](vec3(.45,.03,.02), vec3(.02,.06,.25), vec3(.55,.42,.02), vec3(.04,.18,.05),
                               vec3(.6,.25,.0), vec3(.08,.08,.09), vec3(.5,.5,.52), vec3(.25,.02,.2));
const vec3 TROUSERS[4] = vec3[4](vec3(.03,.04,.08), vec3(.05,.05,.05), vec3(.12,.09,.06), vec3(.18,.18,.2));
const vec3 SKIN[4] = vec3[4](vec3(.55,.36,.27), vec3(.38,.22,.14), vec3(.2,.11,.06), vec3(.62,.45,.36));

void main() {
    vec3 p = aPosition;
    vec3 n = aNormal.xyz;
    float code = aInstanceB.z;
    int palette = int(mod(code, 8.0));
    int pose = int(floor(code / 8.0));
    float excite = aInstanceB.w;
    float phase = aInstanceA.x * 0.37 + aInstanceA.y * 0.11;
    if (aUV.x > 0.1) {
        // Arms swing up about the shoulder: cheering spectators wave when a car
        // approaches; calm ones only shift slightly.
        float side = aUV.x < 0.375 ? 1.0 : -1.0;
        vec3 pivot = vec3(0.0, 0.21 * side, 0.80);
        float raise = pose == 1 ? excite * (2.3 + 0.35 * sin(uTime * 9.0 + phase + side))
                                : 0.06 * sin(uTime * 1.3 + phase) + 0.35 * excite;
        float c = cos(raise), s = sin(raise) * side;
        vec3 v = p - pivot;
        p = pivot + vec3(v.x, v.y * c - v.z * s, v.y * s + v.z * c);
        n = vec3(n.x, n.y * c - n.z * s, n.y * s + n.z * c);
    }
    if (pose == 1) p.z += excite * 0.05 * max(sin(uTime * 9.0 + phase), 0.0);   // little jumps
    float height = aInstanceA.w;
    float cy = cos(aInstanceB.x), sy = sin(aInstanceB.x);
    mat2 yaw = mat2(cy, sy, -sy, cy);
    vec3 local = p * height;
    local.xy = yaw * local.xy;
    n.xy = yaw * n.xy;

    vec3 colour = aColor.rgb;
    if (aUV.y > 0.12) colour = aUV.y < 0.37 ? SKIN[palette % 4] : aUV.y < 0.62 ? JACKET[palette] : TROUSERS[palette % 4];
    vs.N = normalize(n);
    vs.T = abs(vs.N.z) < 0.95 ? normalize(cross(vec3(0.0, 0.0, 1.0), vs.N)) : vec3(1.0, 0.0, 0.0);
    vs.B = cross(vs.N, vs.T);
    vs.worldPos = aInstanceA.xyz + local;
    vs.uv = vec2(0.5);
    vs.color = vec4(colour, aColor.a);
    vs.fade = aInstanceB.y;
    gl_Position = uViewProjection * vec4(vs.worldPos, 1.0);
}
