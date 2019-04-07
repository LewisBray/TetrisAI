#include "GLEW\\glew.h"

#include "arraybuffer.hpp"
#include "indexbuffer.hpp"
#include "playerinput.h"
#include "texture2d.h"
#include "keystate.h"
#include "program.h"
#include "shader.h"
#include "tetris.h"
#include "glfw.h"

#include <exception>
#include <iostream>
#include <chrono>

std::chrono::milliseconds currentTime() noexcept
{
	using namespace std::chrono;

	const auto clockTime = steady_clock().now();
	return duration_cast<milliseconds>(clockTime.time_since_epoch());
}

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

        ArrayBuffer<float, 16> blockVertices({
            // vertex       // texture
            -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f, -0.5f,   1.0f, 0.0f,
             0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f,   0.0f, 1.0f
        });

        blockVertices.bind();
        blockVertices.setVertexAttribute(0, 2, GL_FLOAT,
            4 * sizeof(float), reinterpret_cast<void*>(0));
        blockVertices.setVertexAttribute(1, 2, GL_FLOAT,
            4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

		const auto updateBlockVertices = [&blockVertices](const Position<int>& blockTopLeft)
		{
			// + 0.0f here for type conversion to avoid casts
			blockVertices[0] = blockTopLeft.x + 0.0f;
			blockVertices[1] = blockTopLeft.y + 1.0f;

			blockVertices[4] = blockTopLeft.x + 1.0f;
			blockVertices[5] = blockTopLeft.y + 1.0f;

			blockVertices[8] = blockTopLeft.x + 1.0f;
			blockVertices[9] = blockTopLeft.y + 0.0f;

			blockVertices[12] = blockTopLeft.x + 0.0f;
			blockVertices[13] = blockTopLeft.y + 0.0f;
		};

        const IndexBuffer<unsigned, 6> blockIndices({
            0, 1, 2,
            2, 3, 0
        });

        blockIndices.bind();

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

		Tetris::Grid grid;
        Tetris::Tetrimino tetrimino(Tetris::Tetrimino::Type::T, Position<int>{ 4, 4 });

		int updatesSinceLastDrop = 0;
		std::chrono::milliseconds accumulatedTime{ 0 };
		std::chrono::milliseconds previousTime = currentTime();
		static constexpr std::chrono::milliseconds frameDuration{ 1000 / 60 };
        while (!window.shouldClose())
        {
			const std::chrono::milliseconds time = currentTime();
			accumulatedTime += time - previousTime;
			previousTime = time;

			while (accumulatedTime >= frameDuration)
			{
				const PlayerInput playerInput = getPlayerInput(window);

				// Update Tetrimino position, need to prevent holding keys down...
				if (playerInput.left == KeyState::Pressed)
				{
					tetrimino.shift({ -1, 0 });
					if (tetrimino.collides(grid))
						tetrimino.shift({ 1, 0 });
				}

				if (playerInput.right == KeyState::Pressed)
				{
					tetrimino.shift({ 1, 0 });
					if (tetrimino.collides(grid))
						tetrimino.shift({ -1, 0 });
				}

				if (playerInput.down == KeyState::Pressed ||
					updatesSinceLastDrop >= 120)	// ~2 seconds per drop for now...
				{									// ...make var for this later
					tetrimino.shift({ 0, 1 });
					if (tetrimino.collides(grid))	// Then merge tetrimino to...
					{								// ...grid and spawn another
						tetrimino.shift({ 0, -1 });
						const Colour& tetriminoColour = tetrimino.colour();
						const Tetris::Blocks& tetriminoBlocks = tetrimino.blocks();
						for (const Position<int>& block : tetriminoBlocks)
							grid[block.y][block.x] = tetriminoColour;

						tetrimino = Tetris::Tetrimino(Tetris::Tetrimino::Type::T, Position<int>{4, 4});
					}

					if (updatesSinceLastDrop >= 60)
						updatesSinceLastDrop = 0;
				}

				if (playerInput.rotateClockwise == KeyState::Pressed)
				{
					tetrimino.rotateClockwise();

					int xShift = 1;
					while (tetrimino.collides(grid))
					{
						tetrimino.shift({ xShift, 0 });
						xShift = -1 * xShift + ((xShift > 0) ? -1 : 1);
					}
				}

				if (playerInput.rotateAntiClockwise == KeyState::Pressed)
				{
					tetrimino.rotateAntiClockwise();

					int xShift = 1;
					while (tetrimino.collides(grid))
					{
						tetrimino.shift({ xShift, 0 });
						xShift = -1 * xShift + ((xShift > 0) ? -1 : 1);
					}
				}

				++updatesSinceLastDrop;
				accumulatedTime -= frameDuration;
			}
			
			glClear(GL_COLOR_BUFFER_BIT);

            shaderProgram.setUniform("uColour", tetrimino.colour());
            const Tetris::Blocks& tetriminoBlocks = tetrimino.blocks();
            for (const Position<int>& blockTopLeft : tetriminoBlocks)
            {
				updateBlockVertices(blockTopLeft);
                blockVertices.bufferData(GL_STATIC_DRAW);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

			for (int col = 0; col < Tetris::Columns; ++col)
			{
				for (int row = 0; row < Tetris::Rows; ++row)
				{
					const std::optional<Colour>& cell = grid[row][col];
					if (!cell.has_value())
						continue;

					shaderProgram.setUniform("uColour", cell.value());
					const Position<int> blockTopLeft{ col, row };
					updateBlockVertices(blockTopLeft);
					blockVertices.bufferData(GL_STATIC_DRAW);
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				}
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