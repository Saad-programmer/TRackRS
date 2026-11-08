#version 330 core
// Eye adaptation (mirrors trackrs::post::adaptLuminance): exponential, frame-rate
// independent, faster when the scene gets brighter.
out float outLuminance;
uniform sampler2D uLogLuminance;   // mip-mapped log luminance
uniform sampler2D uPrevious;       // 1x1 adapted luminance
uniform float uLogLod, uDt, uSpeedUp, uSpeedDown;
uniform bool uReset;
void main() {
    float target = exp(textureLod(uLogLuminance, vec2(0.5), uLogLod).r);
    float previous = texelFetch(uPrevious, ivec2(0), 0).r;
    if (uReset || !(previous > 0.0)) { outLuminance = target; return; }
    float speed = target > previous ? uSpeedUp : uSpeedDown;
    outLuminance = previous + (target - previous) * (1.0 - exp(-max(uDt, 0.0) * speed));
}
