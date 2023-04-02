#version 330 core

in vec2 frag_texture_coords;
in vec4 frag_colour;

out vec4 o_colour;

uniform sampler2D u_sampler;

void main() {
    o_colour = texture(u_sampler, frag_texture_coords) * frag_colour;
}
