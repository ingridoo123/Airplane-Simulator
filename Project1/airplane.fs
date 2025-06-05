#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 gColor;
uniform vec3 lightDir;
uniform vec3 viewPos;

void main()
{
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * gColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-lightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * gColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
} 