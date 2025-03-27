#version 330 core
layout (location = 0) in vec2 aPos;
uniform float xOffset;
out vec2 fragPos;

void main() {
    fragPos = aPos + vec2(xOffset, 0.0); // Fragment poz�ci� mozgat�sa
    gl_Position = vec4(fragPos, 0.0, 1.0);
}
