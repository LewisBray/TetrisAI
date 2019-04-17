#version 330 core

in vec2 aVertexPos;
in vec2 aTexturePos;

out vec2 texturePos;

void main()
{
    mat4 transform = mat4(
       1.0 / 10.0,        0.0, 0.0, 0.0,
              0.0, -1.0 / 9.0, 0.0, 0.0,
              0.0,        0.0, 1.0, 0.0,
              0.0,        1.0, 0.0, 1.0
    );

    vec4 position = vec4(aVertexPos, 1.0, 1.0);

    gl_Position = transform * position;

    texturePos = aTexturePos;
}