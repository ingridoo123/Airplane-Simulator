
#version 330

layout(location = 0) in vec3 Position;

// Uniforms
uniform mat4 gVP;
uniform mat4 gModel;

// Outputs to fragment shader
out vec3 WorldPos;
out vec3 Normal;

void main()
{
    vec4 WorldPosition = gModel * vec4(Position, 1.0);
    WorldPos = WorldPosition.xyz;
    gl_Position = gVP * WorldPosition;
    
    // Calculate normal from model matrix (assuming uniform scaling)
    // For a cube, we can derive normal from the vertex position
    Normal = normalize((gModel * vec4(normalize(Position), 0.0)).xyz);
}