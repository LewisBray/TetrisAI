#version 330 core

in vec2 texturePos;

out vec4 colour;

uniform vec4 uColour;
uniform sampler2D uBlock;

void main()
{
    colour = texture(uBlock, texturePos) * uColour;
}