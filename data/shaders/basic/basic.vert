#version 330 core
layout(location=0) in vec4 aPosition;
layout(location=1) in vec3 aEye;
layout(location=2) in vec3 aNormal;
layout(location=3) in vec4 aColor;
layout(location=4) in vec2 aUv0;
layout(location=5) in vec2 aUv1;
layout(location=6) in float aSdf;
layout(std140) uniform Frame { mat4 uViewProjection; };
uniform mat4 uView,uProjection,uTextureMatrix0,uTextureMatrix1;
uniform bool uBaked,uStream,uLighting,uGenerate0,uGenerate1;
uniform vec4 uColor,uAmbient,uDiffuse,uSpecular,uLight,uMaterial,uMaterialSpecular;
uniform vec4 uPlaneS0,uPlaneT0,uPlaneS1,uPlaneT1;
uniform float uShininess,uPointSize;
uniform bool uSdf;
out vec4 vColor,vBackColor;
out vec2 vUv0,vUv1;
out float vFog,vSdf;
vec4 illuminate(vec3 n,vec3 eye) {
  vec3 l=normalize(uLight.xyz-eye*uLight.w);
  float ndl=max(dot(n,l),0.0);
  vec3 halfVector=normalize(l+vec3(0,0,1));
  float spec=ndl>0.0?pow(max(dot(n,halfVector),0.0),uShininess):0.0;
  return clamp(vec4(uMaterial.rgb*(uAmbient.rgb+uDiffuse.rgb*ndl)+uMaterialSpecular.rgb*uSpecular.rgb*spec,uMaterial.a),0.0,1.0);
}
void main(){
  vec4 eye=uBaked?vec4(aEye,1):uView*aPosition;
  gl_Position=uBaked?aPosition:uViewProjection*aPosition;
  gl_PointSize=uPointSize;
  vec3 n=normalize(uBaked?aNormal:transpose(inverse(mat3(uView)))*aNormal);
  vColor=uLighting?illuminate(n,eye.xyz):(uStream?aColor:uColor);
  vBackColor=uLighting?illuminate(-n,eye.xyz):vColor;
  vFog=abs(eye.z);
  vUv0=uStream?aUv0:(uTextureMatrix0*(uGenerate0?vec4(dot(uPlaneS0,aPosition),dot(uPlaneT0,aPosition),0,1):vec4(aUv0,0,1))).xy;
  vUv1=uStream?aUv1:(uTextureMatrix1*(uGenerate1?vec4(dot(uPlaneS1,aPosition),dot(uPlaneT1,aPosition),0,1):vec4(aUv0,0,1))).xy;
  vSdf=uStream?aSdf:(uSdf?1.0:0.0);
}
