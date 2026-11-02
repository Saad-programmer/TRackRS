#version 330 core
uniform sampler2D legacyColor;
layout(std140) uniform Frame { ivec4 extent; };
out vec4 outColor;
void main() {
    ivec2 pixel = clamp(ivec2(gl_FragCoord.xy), ivec2(0), extent.zw - ivec2(1));
    outColor = texelFetch(legacyColor, pixel, 0);
}
