#version 330 core
in vec2 vClip;
out vec4 outColor;
uniform mat4 uInverseViewProjection;
uniform vec3 uCamera,uSunDirection,uSunRadiance;
uniform float uDaylight;
uniform bool uLinear;
#include "sky.glsl"
void main(){
 vec4 p=uInverseViewProjection*vec4(vClip,1,1);
 vec3 direction=normalize(p.xyz/p.w-uCamera);
 vec3 c=analyticSky(direction,uSunDirection,uSunRadiance,uDaylight);
 if(!uLinear){c=clamp((c*(2.51*c+.03))/(c*(2.43*c+.59)+.14),0.,1.);c=pow(c,vec3(1./2.2));}
 outColor=vec4(c,1);
}
