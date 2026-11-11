#version 330 core
// Writes the NDC motion (previous - current) and a valid flag in b.
in vec4 vCurrent, vPrevious;
out vec4 outVelocity;
void main() {
    vec2 current = vCurrent.xy / vCurrent.w, previous = vPrevious.xy / vPrevious.w;
    outVelocity = vec4(previous - current, 1.0, 1.0);
}
