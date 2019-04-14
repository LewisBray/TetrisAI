#include "GLEW\\glew.h"

#include "arraybuffer.hpp"
#include "indexbuffer.hpp"
#include "position.hpp"
#include "texture2d.h"
#include "program.h"
#include "shader.h"
#include "render.h"

BlockDrawer::BlockDrawer()
{
    vertices_.setVertexAttribute(0, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(0));
    vertices_.setVertexAttribute(1, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    const Shader vertexShader("vertex.shader", GL_VERTEX_SHADER);
    const Shader fragmentShader("fragment.shader", GL_FRAGMENT_SHADER);

    program_.attach(vertexShader);
    program_.attach(fragmentShader);

    program_.link();
    program_.validate();
}

void BlockDrawer::operator()(const Position<int>& topLeft, const Colour& colour)
{
    // + 0.0f here for type conversion to avoid casts
    vertices_[0] = topLeft.x + 0.0f;
    vertices_[1] = topLeft.y + 1.0f;

    vertices_[4] = topLeft.x + 1.0f;
    vertices_[5] = topLeft.y + 1.0f;

    vertices_[8] = topLeft.x + 1.0f;
    vertices_[9] = topLeft.y + 0.0f;

    vertices_[12] = topLeft.x + 0.0f;
    vertices_[13] = topLeft.y + 0.0f;

    program_.use();
    program_.setUniform("uColour", colour);
    
    texture_.makeActive();
    program_.setTextureUniform("uBlock", texture_.number());

    vertices_.bind();
    vertices_.bufferData(GL_STATIC_DRAW);

    indices_.bind();
    indices_.bufferData(GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
