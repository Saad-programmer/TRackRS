#version 330 core
// Camera-snapped water grid: xy world position + water depth (m) per vertex.
layout(location = 0) in vec3 aGrid;
uniform mat4 uViewProjection;
uniform float uWaterHeight;
out vec3 vWorld;
out float vDepth;
void main() {
    vWorld = vec3(aGrid.xy, uWaterHeight);
    vDepth = aGrid.z;
    gl_Position = uViewProjection * vec4(vWorld, 1.0);
}
