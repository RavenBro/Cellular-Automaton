#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

uniform vec4 Transform[1764];
uniform int Types[1764];
out vec2 TexCoord;
flat out int Type;

void main()
{
    vec4 currentTransform = Transform[gl_InstanceID];
    vec4 screenTransform = currentTransform * vec4(2.0);
    vec4 out_pos = vec4(screenTransform[0], screenTransform[1], 1.0, 1.0) * vec4(position.x, position.y, position.z, 1.0);
    out_pos = out_pos + vec4(screenTransform[2], screenTransform[3], 0.0, 0.0);
    out_pos = out_pos + vec4(-1, -1, 0.0, 0.0);
    gl_Position = out_pos;
    TexCoord = vec2(texCoord.x, 1 - texCoord.y); 
    Type = Types[gl_InstanceID];
}