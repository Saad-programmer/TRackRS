#version 330 core
uniform samplerCube uSource;uniform int uFace;uniform float uSize;uniform float uRoughness;out vec4 FragColor;
#include "cube.glsl"
void main(){vec3 n=cubeDirection(uFace,gl_FragCoord.xy/uSize*2.0-1.0);if(uRoughness<0.01){FragColor=vec4(textureLod(uSource,n,0).rgb,1);return;}vec3 t=normalize(cross(abs(n.z)<0.999?vec3(0,0,1):vec3(1,0,0),n)),b=cross(n,t);vec3 sum=vec3(0);float total=0,a=uRoughness*uRoughness;
for(uint i=0u;i<128u;++i){vec2 xi=vec2((float(i)+0.5)/128.0,radical(i));float phi=6.28318530718*xi.x,ct=sqrt((1.0-xi.y)/(1.0+(a*a-1.0)*xi.y)),st=sqrt(max(0.0,1.0-ct*ct));vec3 h=normalize(t*cos(phi)*st+b*sin(phi)*st+n*ct),l=normalize(reflect(-n,h));float nl=max(dot(n,l),0.0);sum+=textureLod(uSource,l,0).rgb*nl;total+=nl;}
FragColor=vec4(sum/max(total,1e-6),1);}
