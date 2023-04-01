#version 330 core

in vec2 a_position;
in vec2 a_texture_coords;

out vec2 texture_coords;

void main() {
    mat2 scaling = mat2(
        1.0 / 10.0,        0.0,
               0.0, -1.0 / 9.0
    );

    vec2 translation = vec2(-1.0, 1.0);

    vec2 position = scaling * a_position + translation;

    gl_Position = vec4(position, 0.0, 1.0);
    texture_coords = a_texture_coords;
}
