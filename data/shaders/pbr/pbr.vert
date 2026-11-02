#version 330 core
// ============================================================================
// pbr.vert — TrackRS PBR vertex stage (GLSL 330 core)
// Works with BOTH vertex layouts in vertex_layout.h:
//   * Vertex        (56 B, explicit bitangent)  -> compile with -DEXPLICIT_BITANGENT
//   * VertexPacked  (32 B, 2_10_10_10 N/T, handedness in T.w, B = cross(N,T)*w)
// PERF: no per-vertex matrix inverse — uNormalMatrix is computed on the CPU once
//       per draw (or fetched per-instance). All varyings are interpolated in
//       world space so the fragment stage needs no view-space transforms.
// ============================================================================

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;     // normalised by the GPU (GL_TRUE) when packed
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aTangent;    // xyz = tangent, w = handedness (+1/-1).
                                          // If fed from a vec3 attribute, w defaults to 1.0.
#ifdef EXPLICIT_BITANGENT
layout(location = 4) in vec3 aBitangent;
#endif
layout(location = 5) in vec4 aColor;      // optional: baked AO / wear / paint-mask (RGBA8, normalised)

#ifdef INSTANCED
layout(location=6) in mat4 aModel;
layout(location=10) in mat3 aNormalMatrix;
layout(location=13) in float aParity;
#else
uniform mat4 uModel;
uniform mat3 uNormalMatrix;
uniform float uModelParity;
#endif
uniform mat4 uViewProj;


out VS_OUT {
    vec3 worldPos;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
    vec4 color;
} vs;

void main()
{
#ifdef INSTANCED
    mat4 model = aModel;
    mat3 normalMatrix = aNormalMatrix;
    float parity = aParity;
#else
    mat4 model = uModel;
    mat3 normalMatrix = uNormalMatrix;
    float parity = uModelParity;
#endif
    vec4 wp = model * vec4(aPosition, 1.0);
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = mat3(model) * aTangent.xyz;
    T = normalize(T - dot(T, N) * N);
#ifdef EXPLICIT_BITANGENT
    float signUV = dot(cross(aNormal, aTangent.xyz), aBitangent) < 0.0 ? -1.0 : 1.0;
#else
    float signUV = aTangent.w;
#endif
    vec3 B = cross(N, T) * signUV * parity;

    vs.worldPos = wp.xyz;
    vs.uv       = aUV;
    vs.T = T; vs.B = B; vs.N = N;
    vs.color    = aColor;
    gl_Position = uViewProj * wp;
}
