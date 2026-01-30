#version 450 core
layout(location = 0) in vec2 aPos;

out vec2 TexCoords;

void main()
{
    //passthrough for frag shader
    TexCoords = aPos * 0.5 + 0.5; // convert from [-1,1] to [0,1] for fragment shader
    gl_Position = vec4(aPos, 0.0, 1.0);
}