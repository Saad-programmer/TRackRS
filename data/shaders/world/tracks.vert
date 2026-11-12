#version 330 core
// Ground-hugging track ribbons (amendment A3.3); see engine/terrain/tracks_renderer.h.
layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aSide;
layout(location=2) in vec2 aUv;      // across (half-widths), along (m)
layout(location=3) in vec3 aProfile; // depth, lip, darken
layout(location=4) in vec2 aKind;    // kind, half-width (m)
uniform mat4 uViewProjection;
uniform vec3 uCamPos;
out vec3 vWorld;
out vec2 vSide;
out vec2 vUv;
out vec3 vProfile;
flat out int vKind;
out float vHalf;
void main(){
    vWorld=aPosition;vSide=aSide;vUv=aUv;vProfile=aProfile;vKind=int(aKind.x+.5);vHalf=aKind.y;
    // The ribbon heights come from the full-resolution heightmap, but CDLOD
    // draws coarser grids (every 2nd/4th sample, geomorphed) away from the
    // camera, so on hummocky ground the drawn surface can sit decimetres
    // above them. Pull the ribbon toward the eye in proportion to range; the
    // tracks fade out by 220 m, so the tolerance stays bounded. (A
    // depth-projected screen-space decal is the exact follow-up.)
    vec3 toEye=uCamPos-aPosition;float range=length(toEye);
    vec3 lifted=aPosition+toEye/max(range,1e-3)*min(.06+.03*range,.5*range);
    gl_Position=uViewProjection*vec4(lifted,1);
}
