#version 330 core
out vec4 outColor;
uniform int uFace;
uniform vec3 uSunDirection,uSunRadiance;
uniform float uDaylight;
#include "sky.glsl"
vec3 faceDirection(vec2 p){if(uFace==0)return vec3(1,-p.y,-p.x);if(uFace==1)return vec3(-1,-p.y,p.x);if(uFace==2)return vec3(p.x,1,p.y);if(uFace==3)return vec3(p.x,-1,-p.y);if(uFace==4)return vec3(p.x,-p.y,1);return vec3(-p.x,-p.y,-1);}
void main(){
 vec3 n=normalize(faceDirection(gl_FragCoord.xy/8.*2.-1.));
 vec3 up=abs(n.z)<.95?vec3(0,0,1):vec3(0,1,0),t=normalize(cross(up,n)),b=cross(n,t);
 vec3 sum=vec3(0);
 // Cosine-weighted deterministic hemisphere samples yield irradiance / pi.
 for(int i=0;i<128;++i){float u=(float(i)+.5)/128.,v=fract(float(i)*.61803398875);float r=sqrt(u),phi=6.2831853*v;vec3 d=t*(r*cos(phi))+b*(r*sin(phi))+n*sqrt(1.-u);sum+=analyticSky(d,uSunDirection,uSunRadiance,uDaylight);}
 outColor=vec4(sum/128.,1);
}
