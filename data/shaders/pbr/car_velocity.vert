#version 330 core
// Phase 3e: per-object motion vectors for cars (body and wheels). Rasterised
// with the frame's jittered view-projection so it matches the depth buffer;
// motion is measured between the unjittered current and previous frames.
layout(location = 0) in vec3 aPosition;
uniform mat4 uViewProjection;          // current, jittered (rasterisation)
uniform mat4 uCurrentViewProjection;   // current, unjittered
uniform mat4 uPreviousViewProjection;  // previous, unjittered
uniform mat4 uModel, uPreviousModel;
out vec4 vCurrent, vPrevious;
void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vCurrent = uCurrentViewProjection * world;
    vPrevious = uPreviousViewProjection * (uPreviousModel * vec4(aPosition, 1.0));
    gl_Position = uViewProjection * world;
}
