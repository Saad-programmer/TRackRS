#version 330 core
// Rut, lip and print relief rebuilt per pixel, output as a multiplicative
// shading ratio over the already-lit terrain (blend DST_COLOR, ZERO).
#include "scene.glsl"
in vec3 vWorld;
in vec2 vSide;
in vec2 vUv;
in vec3 vProfile;
flat in int vKind;
in float vHalf;
uniform vec3 uCamPos,uSunDir;
uniform float uSunLum,uAmbLum;
out vec4 FragColor;
float trackHash(float n){return fract(sin(n*91.345)*47453.21);}
// Tyre: flat floor with rounded walls, a sine lip outside, faint tread blocks.
float tyreRelief(vec2 uv,vec3 p){
    float u=abs(uv.x);
    float floorDepth=-p.x*(1.-smoothstep(.6,1.,u));
    float lip=p.y*sin(3.14159*clamp((u-.9)/.8,0.,1.));
    float tread=-.18*p.x*step(u,.75)*smoothstep(.2,.6,sin(uv.y*25.));
    return floorDepth+lip+tread;
}
// Alternating left/right prints; `stride` in metres, sizes in metres.
float printRelief(vec2 uv,vec3 p,float stride,float length_,float width,float spread){
    float k=floor(uv.y/stride);
    float t=uv.y-k*stride;
    float side=mod(k,2.)<1.?-1.:1.;
    float jitter=(trackHash(k)-.5)*.25;
    vec2 d=vec2((uv.x*vHalf-side*spread-jitter*.1)/width,(t-.5*stride)/length_);
    float r=dot(d,d);
    return -p.x*(1.-smoothstep(.55,1.,r))+p.y*smoothstep(.85,1.1,r)*(1.-smoothstep(1.1,1.8,r));
}
float relief(vec2 uv){
    if(vKind==1)return printRelief(uv,vProfile,.95,.11,.10,.16);  // camel: broad pads
    if(vKind==2)return printRelief(uv,vProfile,.70,.13,.05,.10);  // foot
    return tyreRelief(uv,vProfile);
}
void main(){
    float h=relief(vUv);
    // Finite differences in metres: across (u scaled by half-width) and along.
    float eu=.03,ev=.02;
    float dhdx=(relief(vUv+vec2(eu,0))-relief(vUv-vec2(eu,0)))/(2.*eu*vHalf);
    float dhdy=(relief(vUv+vec2(0,ev))-relief(vUv-vec2(0,ev)))/(2.*ev);
    vec3 side=vec3(vSide,0),forward=vec3(-vSide.y,vSide.x,0);
    vec3 N=normalize(vec3(0,0,1)-side*dhdx-forward*dhdy);
    vec3 L=normalize(uSunDir);
    float visibility=sceneVisibility(vWorld,vec3(0,0,1));
    float occlusion=1.-.35*clamp(-h/max(vProfile.x,1e-4),0.,1.);
    float flat_=uSunLum*visibility*max(L.z,0.)+uAmbLum;
    float lit=uSunLum*visibility*max(dot(N,L),0.)+uAmbLum*occlusion;
    float ratio=clamp(lit/max(flat_,1e-4),.25,1.8);
    float disturbed=smoothstep(.0,.25,abs(h)/max(vProfile.x,1e-4));
    float albedo=mix(1.,vProfile.z,disturbed);
    float distance_=length(vWorld-uCamPos);
    float fade=1.-smoothstep(120.,220.,distance_);
    float edge=1.-smoothstep(.85,1.,abs(vUv.x)/(vKind==0?1.7:1.));
    FragColor=vec4(vec3(mix(1.,ratio*albedo,fade*edge)),1);
}
