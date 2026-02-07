#version 430 core
layout(location = 0) in vec2 aPos;

out vec2 TexCoords;

void main()
{
    //passthrough for frag shader
    TexCoords = aPos * 0.5 + 0.5; // [-1,1] -> [0,1] map
    gl_Position = vec4(aPos, 0, 1);
}