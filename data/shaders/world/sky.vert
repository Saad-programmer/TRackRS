#version 330 core
out vec2 vClip;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);vClip=p*2.-1.;gl_Position=vec4(vClip,1,1);}
