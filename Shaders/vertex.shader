#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texture_coords;
layout (location = 2) in vec4 a_colour;

out vec2 frag_texture_coords;
out vec4 frag_colour;

void main() {
    mat2 scaling = mat2(
        1.0 / 10.0,        0.0,
               0.0, -1.0 / 9.0
    );

    vec2 translation = vec2(-1.0, 1.0);

    vec2 position = scaling * a_position + translation;

    gl_Position = vec4(position, 0.0, 1.0);
    frag_texture_coords = a_texture_coords;
    frag_colour = a_colour;
}
