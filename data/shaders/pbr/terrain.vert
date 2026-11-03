#version 330 core
layout(location=0) in vec2 aGrid;
uniform sampler2D uHeight;
uniform mat4 uViewProjection;
uniform vec3 uCamPos;
uniform vec4 uPatch; // sample origin XY, sample span, horizontal scale
out VS_OUT { vec3 worldPos; vec2 uv; vec3 T; vec3 B; vec3 N; vec4 color; } fs;
float heightAt(vec2 pixel) {
    ivec2 size=textureSize(uHeight,0);
    ivec2 p=ivec2(floor(pixel));
    vec2 f=fract(pixel);
    ivec2 a=(p%size+size)%size;
    ivec2 b=((p+ivec2(1))%size+size)%size;
    float h00=texelFetch(uHeight,a,0).r;
    float h10=texelFetch(uHeight,ivec2(b.x,a.y),0).r;
    float h01=texelFetch(uHeight,ivec2(a.x,b.y),0).r;
    float h11=texelFetch(uHeight,b,0).r;
    return f.x<f.y ? h00+(h01-h00)*f.y+(h11-h01)*f.x
                       : h00+(h10-h00)*f.x+(h11-h10)*f.y;
}
void main() {
    vec2 pixel=uPatch.xy+aGrid*(uPatch.z/32.0);
    float radius=8.0*uPatch.z*uPatch.w;
    float morph=clamp((length(pixel*uPatch.w-uCamPos.xy)-0.7*radius)/(0.3*radius),0.0,1.0);
    pixel=uPatch.xy+(aGrid-mod(aGrid,vec2(2.0))*morph)*(uPatch.z/32.0);
    fs.worldPos=vec3(pixel*uPatch.w,heightAt(pixel));
    float dx=heightAt(pixel+vec2(1,0))-heightAt(pixel-vec2(1,0));
    float dy=heightAt(pixel+vec2(0,1))-heightAt(pixel-vec2(0,1));
    fs.N=normalize(vec3(-dx,-dy,2.0*uPatch.w));
    fs.T=normalize(vec3(fs.N.z,0,-fs.N.x));
    fs.B=cross(fs.N,fs.T);
    fs.uv=pixel;
    fs.color=vec4(1);
    gl_Position=uViewProjection*vec4(fs.worldPos,1);
}
