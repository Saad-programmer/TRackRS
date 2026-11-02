#version 330 core
in vec4 vColor,vBackColor;
in vec2 vUv0,vUv1;
in float vFog,vSdf;
uniform sampler2D uTexture0,uTexture1;
uniform bool uTextured0,uTextured1,uFog,uTwoSided;
uniform vec4 uFogColor;
uniform float uDensity,uAlphaReference;
uniform int uAlphaFunction;
out vec4 outColor;
bool passes(float a){
 if(uAlphaFunction==512)return false;
 if(uAlphaFunction==513)return a<uAlphaReference;
 if(uAlphaFunction==514)return a==uAlphaReference;
 if(uAlphaFunction==515)return a<=uAlphaReference;
 if(uAlphaFunction==516)return a>uAlphaReference;
 if(uAlphaFunction==517)return a!=uAlphaReference;
 if(uAlphaFunction==518)return a>=uAlphaReference;
 return true;
}
void main(){
 vec4 color=(!gl_FrontFacing&&uTwoSided)?vBackColor:vColor;
 if(uTextured0){
  vec4 texel=texture(uTexture0,vUv0);
  if(vSdf>.5){float width=max(fwidth(texel.a)*.65,1.0/255.0);texel.a=smoothstep(.5-width,.5+width,texel.a);}
  color*=texel;
 }
 if(uTextured1){vec4 detail=texture(uTexture1,vUv1);color=vec4(clamp(color.rgb+detail.rgb-.5,0.0,1.0),color.a*detail.a);}
 if(!passes(color.a))discard;
 if(uFog)color.rgb=mix(uFogColor.rgb,color.rgb,clamp(exp(-uDensity*vFog),0.0,1.0));
 outColor=color;
}
