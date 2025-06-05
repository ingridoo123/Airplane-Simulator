#version 330

layout(location = 0) out vec4 FragColor;

in vec4 Color;
in vec2 Tex;
in vec3 WorldPos;
in vec3 Normal;

uniform sampler2D gTextureHeight0;
uniform sampler2D gTextureHeight1;
uniform sampler2D gTextureHeight2;
uniform sampler2D gTextureHeight3;

uniform float gHeight0 = 80.0;
uniform float gHeight1 = 210.0;
uniform float gHeight2 = 250.0;
uniform float gHeight3 = 280.0;

uniform vec3 gReversedLightDir;
uniform vec3 gSecondLightDir;  // Direction of the second light
uniform vec3 gSecondLightColor = vec3(0.8, 0.2, 0.2);  // Reddish color for second light

// Light intensity uniforms
uniform float gMainLightIntensity = 0.5;    // Main light intensity 
uniform float gSecondLightIntensity = 0.0;  // Second light intensity 

vec4 CalcTexColor()
{
    vec4 TexColor;

    float Height = WorldPos.y;

    if (Height < gHeight0) {
       TexColor = texture(gTextureHeight0, Tex);
    } else if (Height < gHeight1) {
       vec4 Color0 = texture(gTextureHeight0, Tex);
       vec4 Color1 = texture(gTextureHeight1, Tex);
       float Delta = gHeight1 - gHeight0;
       float Factor = (Height - gHeight0) / Delta;
       TexColor = mix(Color0, Color1, Factor);
    } else if (Height < gHeight2) {
       vec4 Color0 = texture(gTextureHeight1, Tex);
       vec4 Color1 = texture(gTextureHeight2, Tex);
       float Delta = gHeight2 - gHeight1;
       float Factor = (Height - gHeight1) / Delta;
       TexColor = mix(Color0, Color1, Factor);
    } else if (Height < gHeight3) {
       vec4 Color0 = texture(gTextureHeight2, Tex);
       vec4 Color1 = texture(gTextureHeight3, Tex);
       float Delta = gHeight3 - gHeight2;
       float Factor = (Height - gHeight2) / Delta;
       TexColor = mix(Color0, Color1, Factor);
    } else {
       TexColor = texture(gTextureHeight3, Tex);
    }

    return TexColor;
}

void main()
{
    vec4 TexColor = CalcTexColor();
    vec3 Normal_ = normalize(Normal);

    // First light (main sun)
    float Diffuse1 = dot(Normal_, gReversedLightDir);
    Diffuse1 = max(0.6f, Diffuse1);
    float Ambient = 0.4f;
    Diffuse1 = (Diffuse1 + Ambient) * gMainLightIntensity;

    // Second light (red sun)
    float Diffuse2 = dot(Normal_, gSecondLightDir);
    Diffuse2 = max(0.3f, Diffuse2);  // Less intense than main light
    Diffuse2 = Diffuse2 * gSecondLightIntensity;

    // Combine both lights
    vec3 FinalColor = TexColor.rgb * (Diffuse1 + Diffuse2 * gSecondLightColor);

    FragColor = vec4(FinalColor, TexColor.a);
}