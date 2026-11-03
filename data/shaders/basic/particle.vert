#version 330 core
layout(location=0) in vec3 center;
layout(location=1) in vec3 axisX;
layout(location=2) in vec3 axisY;
layout(location=3) in vec4 leftColor;
layout(location=4) in vec4 rightColor;
layout(location=5) in vec4 uvRect;
layout(location=6) in float swapUv;
layout(std140) uniform Frame { mat4 uViewProjection; };
uniform mat4 uView;
uniform bool uPoints;
uniform float uPointSize;
out vec3 vSceneEye,vSceneNormal;
out vec4 vColor,vBackColor;
out vec2 vUv0,vUv1;
out float vFog,vSdf;
void main(){
 const vec2 corners[6]=vec2[6](vec2(-1,-1),vec2(1,-1),vec2(1,1),vec2(-1,-1),vec2(1,1),vec2(-1,1));
 vec2 corner=uPoints?vec2(0):corners[gl_VertexID];
 vec4 position=vec4(center+axisX*corner.x+axisY*corner.y,1);
 gl_Position=uViewProjection*position;
 gl_PointSize=uPointSize;
 vColor=mix(leftColor,rightColor,corner.x*.5+.5);vBackColor=vColor;
 vec2 uvCorner=corner*.5+.5;
 if(swapUv>.5)uvCorner=uvCorner.yx;
 vUv0=mix(uvRect.xy,uvRect.zw,uvCorner);vUv1=vUv0;
 vSceneEye=(uView*position).xyz;vSceneNormal=vec3(0,0,1);
 vFog=abs((uView*position).z);vSdf=0.0;
}
