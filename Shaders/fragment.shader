#version 330 core

in vec2 texture_coords;

out vec4 colour;

uniform vec4 u_colour;
uniform sampler2D u_sampler;

void main() {
    colour = texture(u_sampler, texture_coords) * u_colour;
}
