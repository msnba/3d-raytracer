#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdexcept>

#include "tinytracer/renderer/shader.h"

namespace tinytracer::renderer {

Shader::Shader(const char *vertexCode, const char *fragmentCode) {
  unsigned int vert = compileShader(GL_VERTEX_SHADER, vertexCode);
  unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

  ID_ = glCreateProgram();
  glAttachShader(ID_, vert);
  glAttachShader(ID_, frag);
  linkShader(ID_);

  glDeleteShader(vert);
  glDeleteShader(frag);
}

Shader::Shader(const char *computeCode) {
  unsigned int comp = compileShader(GL_COMPUTE_SHADER, computeCode);

  ID_ = glCreateProgram();
  glAttachShader(ID_, comp);
  linkShader(ID_);

  glDeleteShader(comp);
}

Shader::~Shader() {
  if (ID_)
    glDeleteProgram(ID_);
}

Shader::Shader(const Shader &other) : uniformCache_(other.uniformCache_) {
  ID_ = other.ID_;
}

Shader &Shader::operator=(const Shader &other) {
  if (this != &other) {
    if (ID_)
      glDeleteProgram(ID_);
    ID_ = other.ID_;
    uniformCache_ = other.uniformCache_;
  }

  return *this;
}

Shader::Shader(Shader &&other) noexcept
    : ID_(other.ID_), uniformCache_(std::move(other.uniformCache_)) {
  other.ID_ = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept {
  if (this != &other) {
    if (ID_)
      glDeleteProgram(ID_);

    ID_ = other.ID_;
    uniformCache_ = std::move(other.uniformCache_);
    other.ID_ = 0;
  }

  return *this;
}

void Shader::use() const { glUseProgram(ID_); }

template <> void Shader::set(const std::string &name, bool value) const {
  glUniform1i(getLocation(name), (int)value);
}

template <> void Shader::set(const std::string &name, int value) const {
  glUniform1i(getLocation(name), value);
}

template <>
void Shader::set(const std::string &name, unsigned int value) const {
  glUniform1ui(getLocation(name), value);
}

template <> void Shader::set(const std::string &name, float value) const {
  glUniform1f(getLocation(name), value);
}

template <> void Shader::set(const std::string &name, glm::mat4 value) const {
  glUniformMatrix4fv(getLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

template <> void Shader::set(const std::string &name, glm::vec3 value) const {
  glUniform3fv(getLocation(name), 1, glm::value_ptr(value));
}

template <> void Shader::set(const std::string &name, glm::vec2 value) const {
  glUniform2fv(getLocation(name), 1, glm::value_ptr(value));
}

int Shader::getLocation(const std::string &name) const {
  auto it = uniformCache_.find(name);
  if (it != uniformCache_.end())
    return it->second;

  int location = glGetUniformLocation(ID_, name.c_str());
  uniformCache_[name] = location;
  return location;
}

unsigned int Shader::compileShader(unsigned int type, const char *source) {
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  int success;
  char infoLog[1024];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
    throw std::runtime_error(
        "Shader compilation failed: " + std::string(infoLog) + "\n");
  }
  return shader;
}

void Shader::linkShader(unsigned int program) {
  glLinkProgram(program);

  int success;
  char infoLog[1024];
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 1024, NULL, infoLog);
    throw std::runtime_error("Shader linking failed: " + std::string(infoLog) +
                             "\n");
  }
}

}