#version 330 core
uniform samplerCube uSky;
uniform mat4 uInverseViewProjection;
uniform vec3 uCamera;
uniform vec2 uResolution;
out vec4 FragColor;
const float PI=3.14159265359;
#include "brdf.glsl"
void main(){vec2 ndc=gl_FragCoord.xy/uResolution*2.0-1.0;vec4 farPoint=uInverseViewProjection*vec4(ndc,1,1);vec3 direction=normalize(farPoint.xyz/farPoint.w-uCamera);vec3 radiance=textureLod(uSky,direction,0).rgb;FragColor=vec4(pow(tonemapACES(radiance),vec3(1.0/2.2)),1);}
