#include "GLEW\\glew.h"

#include "arraybuffer.hpp"
#include "indexbuffer.hpp"
#include "texture2d.h"
#include "tetrimino.h"
#include "program.h"
#include "shader.h"
#include "glfw.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        const GLFW::GLFW glfw;

        const GLFW::Window window(640, 480, "Tetris AI");
        window.makeCurrentContext();

        if (glewInit() != GLEW_OK)
            throw std::exception{ "Failed to initialise GLEW" };

        std::cout << glGetString(GL_VERSION) << std::endl;

        // make constructors take std::initializer_list to clean this up
        ArrayBuffer<float, 16> vertices({
            // vertex       // texture
            -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f, -0.5f,   1.0f, 0.0f,
             0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f,   0.0f, 1.0f
        });

        vertices.bind();
        vertices.setVertexAttribute(0, 2, GL_FLOAT,
            4 * sizeof(float), reinterpret_cast<void*>(0));
        vertices.setVertexAttribute(1, 2, GL_FLOAT,
            4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

        const IndexBuffer<unsigned, 6> indices({
            0, 1, 2,
            2, 3, 0
        });

        indices.bind();

        const Shader vertexShader("vertex.shader", GL_VERTEX_SHADER);
        const Shader fragmentShader("fragment.shader", GL_FRAGMENT_SHADER);

        const Program shaderProgram;

        shaderProgram.attach(vertexShader);
        shaderProgram.attach(fragmentShader);

        shaderProgram.link();
        shaderProgram.validate();

        shaderProgram.use();

        const Texture2d blockTexture(0, "block.jpg", Texture2d::ImageType::JPG);
        shaderProgram.setTextureUniform("uBlock", blockTexture.number());
        blockTexture.makeActive();

        // Test rotation of all tetrimino types and adjust centres
        T tetrimino(Position<int>{ 4, 4 });

        while (!window.shouldClose())
        {
            glClear(GL_COLOR_BUFFER_BIT);

            // break all of this up into a draw function for tetriminos
            shaderProgram.setUniform("uColour", tetrimino.colour());

            const Blocks& tetriminoBlocks = tetrimino.blocks();
            for (const Position<int>& blockTopLeft : tetriminoBlocks)
            {
                vertices[0] = static_cast<float>(blockTopLeft.x);
                vertices[1] = static_cast<float>(blockTopLeft.y + 1);

                vertices[4] = static_cast<float>(blockTopLeft.x + 1);
                vertices[5] = static_cast<float>(blockTopLeft.y + 1);

                vertices[8] = static_cast<float>(blockTopLeft.x + 1);
                vertices[9] = static_cast<float>(blockTopLeft.y);

                vertices[12] = static_cast<float>(blockTopLeft.x);
                vertices[13] = static_cast<float>(blockTopLeft.y);

                vertices.bufferData(GL_STATIC_DRAW);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            window.swapBuffers();
            GLFW::pollEvents();
        }

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cout << error.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cout << "Unhandled exception" << std::endl;
        return -2;
    }
}