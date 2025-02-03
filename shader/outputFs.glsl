#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D renderedImage;
uniform vec2 resolution;

void main()
{
    FragColor = texture(renderedImage, TexCoord);
}