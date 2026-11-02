// Appendix A shared production BRDF.
float D_GGX(float NdotH, float alpha)                       // alpha = roughness^2
{
    float a2 = alpha * alpha;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

float G_SchlickGGX(float NdotX, float k)
{
    return NdotX / (NdotX * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)   // direct-light remap
{
    float r = roughness + 1.0;
    float k = (r * r) * 0.125;
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    float f = clamp(1.0 - cosTheta, 0.0, 1.0);
    float f2 = f * f;
    return F0 + (1.0 - F0) * (f2 * f2 * f);                // x^5 without pow()
}

vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    float f = clamp(1.0 - cosTheta, 0.0, 1.0);
    float f2 = f * f;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * (f2 * f2 * f);
}

// Kaplanyan/Tokuyoshi specular AA: kills sparkle on normal-mapped, high-frequency surfaces
float specularAA(vec3 N, float roughness)
{
    vec3 dndx = dFdx(N), dndy = dFdy(N);
    float variance = 0.25 * (dot(dndx, dndx) + dot(dndy, dndy));
    return sqrt(clamp(roughness * roughness + min(2.0 * variance, 0.18), 0.0, 1.0));
}

vec3 tonemapACES(vec3 x)                                    // Narkowicz fit
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Cook-Torrance + Lambert for ONE light. Returns outgoing radiance contribution.
vec3 evalLight(vec3 L, vec3 radiance, vec3 N, vec3 V, float NdotV,
               vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float alpha = roughness * roughness;
    float D = D_GGX(NdotH, alpha);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3  F = F_Schlick(VdotH, F0);

    vec3 specular = (D * G) * F / (4.0 * NdotV * NdotL + 1e-4);
    vec3 kD       = (vec3(1.0) - F) * (1.0 - metallic);     // energy conservation
    vec3 diffuse  = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

