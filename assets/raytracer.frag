#version 450 core
out vec4 FragColor;
in vec2 TexCoords;
void main()
{
    FragColor = vec4(TexCoords,TexCoords);
}