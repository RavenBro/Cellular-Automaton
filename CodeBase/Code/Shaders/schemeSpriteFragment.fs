#version 330 core

in vec2 TexCoord;
flat in int Type;

uniform sampler2D Texture1;
uniform sampler2D Texture2;
uniform sampler2D Texture3;

out vec4 color;

void main()
{
    if (Type == 1)
    {
        color = texture(Texture1, TexCoord);
    }
    if (Type == 2)
    {
        color = texture(Texture2, TexCoord);
    }
    if (Type == 3)
    {
        color = texture(Texture3, TexCoord);
    }
}