#include "GLEW\\glew.h"

#include "arraybuffer.hpp"
#include "indexbuffer.hpp"
#include "playerinput.h"
#include "keystate.h"
#include "render.h"
#include "tetris.h"
#include "glfw.h"

#include <exception>
#include <iostream>
#include <utility>
#include <random>
#include <chrono>

static std::chrono::milliseconds currentTime()
{
	using namespace std::chrono;

	const auto clockTime = steady_clock().now();
	return duration_cast<milliseconds>(clockTime.time_since_epoch());
}

static int generateRandomIndex(const int lastIndex)
{
	const std::chrono::milliseconds time = currentTime();
	const std::uint32_t rngSeed = static_cast<std::uint32_t>(time.count());
	std::minstd_rand rng(rngSeed);
	const std::uniform_int_distribution<int> uniformDist(0, lastIndex);

	return uniformDist(rng);
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

        BlockDrawer drawBlock;

		Tetris::Grid grid;
        Tetris::Tetrimino tetrimino(Tetris::Tetrimino::Type::T, Position<int>{ 4, 2 });

		PlayerInput previousInput;
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
				if ((playerInput.left == KeyState::Pressed &&
					previousInput.left != KeyState::Pressed) ||
					playerInput.left == KeyState::Held)
				{
					tetrimino.shift({ -1, 0 });
					if (tetrimino.collides(grid))
						tetrimino.shift({ 1, 0 });
				}

				if ((playerInput.right == KeyState::Pressed &&
					previousInput.right != KeyState::Pressed) ||
					playerInput.right == KeyState::Held)
				{
					tetrimino.shift({ 1, 0 });
					if (tetrimino.collides(grid))
						tetrimino.shift({ -1, 0 });
				}

				if ((playerInput.down == KeyState::Pressed &&
					previousInput.down != KeyState::Pressed) ||
					playerInput.down == KeyState::Held ||
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

						const Tetris::Tetrimino::Type nextType =
							static_cast<Tetris::Tetrimino::Type>(generateRandomIndex(6));
						tetrimino = Tetris::Tetrimino(nextType, Position<int>{ 4, 2 });
					}

					if (updatesSinceLastDrop >= 60)
						updatesSinceLastDrop = 0;
				}

				if ((playerInput.rotateClockwise == KeyState::Pressed &&
					previousInput.rotateClockwise != KeyState::Pressed) ||
					playerInput.rotateClockwise == KeyState::Held)
				{
					tetrimino.rotateClockwise();

					int xShift = 1;
					while (tetrimino.collides(grid))
					{
						tetrimino.shift({ xShift, 0 });
						xShift = -1 * xShift + ((xShift > 0) ? -1 : 1);
					}
				}

				if ((playerInput.rotateAntiClockwise == KeyState::Pressed &&
					previousInput.rotateAntiClockwise != KeyState::Pressed) ||
					previousInput.rotateAntiClockwise == KeyState::Held)
				{
					tetrimino.rotateAntiClockwise();

					int xShift = 1;
					while (tetrimino.collides(grid))
					{
						tetrimino.shift({ xShift, 0 });
						xShift = -1 * xShift + ((xShift > 0) ? -1 : 1);
					}
				}

				const auto rowIsComplete = [](const std::array<std::optional<Colour>, Tetris::Columns>& row)
				{
					for (const std::optional<Colour>& cell : row)
						if (!cell.has_value())
							return false;

					return true;
				};

				// Check for completed lines in grid, remove
				// them and push remaining lines down
				int rowToFillIn = -1;
				for (int row = Tetris::Rows - 1; row >= -0; --row)
				{
                    if (rowIsComplete(grid[row]))
					{
						for (std::optional<Colour>& cell : grid[row])
							cell = std::nullopt;
						if (rowToFillIn == -1)
							rowToFillIn = row;
						else
							rowToFillIn = std::max(row, rowToFillIn);
					}

					if (rowToFillIn != -1 && row < rowToFillIn)
					{
						for (int col = 0; col < Tetris::Columns; ++col)
							std::swap(grid[row][col], grid[rowToFillIn][col]);
						--rowToFillIn;
					}
				}

				++updatesSinceLastDrop;
				previousInput = playerInput;
				accumulatedTime -= frameDuration;
			}
			
			glClear(GL_COLOR_BUFFER_BIT);

            const Tetris::Blocks& tetriminoBlocks = tetrimino.blocks();
            for (const Position<int>& blockTopLeft : tetriminoBlocks)
                drawBlock(blockTopLeft, tetrimino.colour());

			for (int col = 0; col < Tetris::Columns; ++col)
			{
				for (int row = 0; row < Tetris::Rows; ++row)
				{
					const std::optional<Colour>& cell = grid[row][col];
					if (!cell.has_value())
						continue;
					
                    const Position<int> blockTopLeft{ col, row };
                    drawBlock(blockTopLeft, cell.value());
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