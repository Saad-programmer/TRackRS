#version 330 core
#include "scene.glsl"
uniform vec3 uTestPoint;
out vec4 outColor;
void main(){outColor=vec4(sceneVisibility(uTestPoint,vec3(0,0,1)));}
