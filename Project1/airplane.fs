#version 330

layout(location = 0) out vec4 FragColor;

// Inputs from vertex shader
in vec3 WorldPos;
in vec3 Normal;

// Uniform variables for lighting (same as terrain)
uniform vec3 gReversedLightDir;
uniform vec3 gSecondLightDir;
uniform vec3 gSecondLightColor = vec3(0.8, 0.2, 0.2);
uniform float gMainLightIntensity = 0.5;
uniform float gSecondLightIntensity = 0.0;

// Uniform for airplane color (set by your CubeTechnique::SetColor)
uniform vec3 gAirplaneColor = vec3(0.8, 0.8, 0.9);

void main()
{
    vec3 Normal_ = normalize(Normal);
    
    // First light (main sun) - same calculation as terrain
    float Diffuse1 = dot(Normal_, gReversedLightDir);
    Diffuse1 = max(0.6f, Diffuse1);
    float Ambient = 0.4f;
    Diffuse1 = (Diffuse1 + Ambient) * gMainLightIntensity;
    
    // Second light (red sun) - same calculation as terrain
    float Diffuse2 = dot(Normal_, gSecondLightDir);
    Diffuse2 = max(0.3f, Diffuse2);
    Diffuse2 = Diffuse2 * gSecondLightIntensity;
    
    // Combine both lights with airplane color
    vec3 FinalColor = gAirplaneColor * (Diffuse1 + Diffuse2 * gSecondLightColor);
    
    // Ensure minimum visibility even in complete darkness
    float MinBrightness = 0.1f;
    FinalColor = max(FinalColor, gAirplaneColor * MinBrightness);
    
    FragColor = vec4(FinalColor, 1.0);
}