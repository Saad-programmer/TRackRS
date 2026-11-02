#version 330 core
out vec4 FragColor;
const float PI=3.14159265359;
#include "brdf.glsl"
float radicalInverse(uint bits){bits=(bits<<16u)|(bits>>16u);bits=((bits&0x55555555u)<<1u)|((bits&0xAAAAAAAAu)>>1u);bits=((bits&0x33333333u)<<2u)|((bits&0xCCCCCCCCu)>>2u);bits=((bits&0x0F0F0F0Fu)<<4u)|((bits&0xF0F0F0F0u)>>4u);bits=((bits&0x00FF00FFu)<<8u)|((bits&0xFF00FF00u)>>8u);return float(bits)*2.3283064365386963e-10;}
void main(){
 float cell=floor(gl_FragCoord.x);
 float rough=cell==0.0?0.04:cell*0.1;
 float metal=mod(floor(gl_FragCoord.y),2.0);
 float angle=floor(gl_FragCoord.y/2.0)*0.8;
 vec3 N=vec3(0,0,1),V=normalize(vec3(angle,0,1)),F0=mix(vec3(.04),vec3(1),metal);
 float nv=dot(N,V),alpha=rough*rough;vec3 energy=vec3(0);
 const uint samples=4096u;
 for(uint i=0u;i<samples;++i){
  vec2 xi=vec2((float(i)+0.5)/float(samples),radicalInverse(i));float phi=2.0*PI*xi.x;
  float ct=sqrt((1.0-xi.y)/(1.0+(alpha*alpha-1.0)*xi.y));float st=sqrt(max(0.0,1.0-ct*ct));vec3 H=vec3(st*cos(phi),st*sin(phi),ct);vec3 L=normalize(reflect(-V,H));
  // Sampling distribution is the normalized GGX PDF, with no BRDF guard.
  // Evaluate the Jacobian at the sampled light direction, reconstructing H
  // as the production BRDF does. At roughness .04, using the pre-reflection
  // float H instead produces a four-percent normalization artifact.
  vec3 inverseH=normalize(V+L);float nh=inverseH.z;
  float a2=alpha*alpha,d=(1.0-nh*nh)+nh*nh*a2;
  float pdf=(a2/(PI*d*d))*nh/(4.0*max(dot(V,inverseH),1e-5));
  if(L.z>0.0)energy+=evalLight(L,vec3(1),N,V,nv,vec3(1),1.0,rough,F0)/max(pdf,1e-7);
  vec3 diffuseL=vec3(sqrt(xi.y)*cos(phi),sqrt(xi.y)*sin(phi),sqrt(1.0-xi.y));
  vec3 full=evalLight(diffuseL,vec3(1),N,V,nv,vec3(1),metal,rough,F0);
  vec3 spec=evalLight(diffuseL,vec3(1),N,V,nv,vec3(1),1.0,rough,F0);
  energy+=(full-spec)/(diffuseL.z/PI);
 }
 vec3 L=normalize(vec3(0.3,0.4,1));
 vec3 forward=evalLight(L,vec3(1),N,V,nv,vec3(1),metal,rough,F0)/L.z;
 vec3 reverse=evalLight(V,vec3(1),N,L,L.z,vec3(1),metal,rough,F0)/nv;
 vec3 relative=abs(forward-reverse)/max(max(forward,reverse),vec3(1e-6));
 FragColor=vec4(energy/float(samples),max(relative.r,max(relative.g,relative.b)));
}
