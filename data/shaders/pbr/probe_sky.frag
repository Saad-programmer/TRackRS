#version 330 core
uniform samplerCube uSky;uniform int uFace;uniform float uSize;uniform mat3 uSkyRotation;out vec4 FragColor;
#include "cube.glsl"
void main(){FragColor=vec4(textureLod(uSky,uSkyRotation*cubeDirection(uFace,gl_FragCoord.xy/uSize*2.0-1.0),0).rgb,1);}
