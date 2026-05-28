#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace tinytracer::renderer {

class Shader {
public:
  unsigned int ID_ = 0;

  Shader() = default;
  Shader(const char *vertexCode, const char *fragmentCode);
  Shader(const char *computeCode);

  ~Shader();
  Shader(const Shader &other);
  Shader &operator=(const Shader &other);
  Shader(Shader &&other) noexcept;
  Shader &operator=(Shader &&other) noexcept;

  void use() const;

  template <typename T_> void set(const std::string &name, T_ value) const;

private:
  mutable std::unordered_map<std::string, int> uniformCache_;

  int getLocation(const std::string &name) const;
  static unsigned int compileShader(unsigned int type, const char *source);
  static void linkShader(unsigned int program);
};

}