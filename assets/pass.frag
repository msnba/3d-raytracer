#version 430
in vec2 uv;
out vec4 FragColor;

uniform sampler2D accumTex;

void main() {
    FragColor = texture(accumTex, uv);
}