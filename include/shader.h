#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

enum ShaderType
{
    PATH,
    SOURCE
};

class Shader
{
public:
    unsigned int ID;

    Shader(const char *vertexPath, const char *fragmentPath, ShaderType type);

    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
};

#endif