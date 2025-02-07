#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D renderedImage;

void main()
{
    FragColor = texture(renderedImage, TexCoords);
}