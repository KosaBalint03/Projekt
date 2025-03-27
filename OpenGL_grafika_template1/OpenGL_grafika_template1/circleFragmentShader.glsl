#version 330 core


in vec2 fragPos;
out vec4 FragColor;

uniform int isIntersecting;
uniform vec2 circleCenter;

void main() {
    float radius = 50.0 / 300.0;
    float distance = length(fragPos - circleCenter);

    vec3 innerColor = isIntersecting == 1 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 outerColor = isIntersecting == 1 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);

    vec3 color = mix(innerColor, outerColor, clamp(distance / radius, 0.0, 1.0));
    FragColor = vec4(color, 1.0);
}
