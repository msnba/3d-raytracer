#version 330 core
out vec4 FragColor;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vec3 ambient = 0.05f * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diffuse = max(dot(norm, lightDir), 0.0);

    float specularStrength = 0.5f;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    int shine = 64;
    vec3 specular = specularStrength * pow(max(dot(viewDir, reflectDir), 0.0), shine) * lightColor;  
    FragColor = vec4((ambient + diffuse * lightColor + specular) * objectColor, 1.0);
}