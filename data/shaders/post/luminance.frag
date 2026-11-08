#version 330 core
// Log-luminance of the HDR frame into a small mip-mapped target; the 1x1 mip is
// the geometric mean. Centre-weighted so the road matters more than the sky rim.
in vec2 vUv;
out float outLogLuminance;
uniform sampler2D uHdr;
void main() {
    vec3 c = textureLod(uHdr, vUv, 0.0).rgb;
    float l = max(dot(c, vec3(0.2126, 0.7152, 0.0722)), 1e-4);
    float w = 1.0 - 0.5 * smoothstep(0.25, 0.75, length(vUv - 0.5) * 1.4142);
    outLogLuminance = log(l) * w + log(0.18) * (1.0 - w);
}
