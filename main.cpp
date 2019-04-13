#include "GLEW\\glew.h"

#include "render.h"
#include "tetris.h"
#include "input.h"
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

		InputHistory inputHistory;
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
                inputHistory.previous = inputHistory.current;
                inputHistory.current = getPlayerInput(window);

                const auto [shouldMergeWithGrid, newUpdatesSinceLastDrop] =
                    tetrimino.update(inputHistory, grid, updatesSinceLastDrop);

                if (shouldMergeWithGrid)
                {
                    const Colour& tetriminoColour = tetrimino.colour();
                    const Tetris::Blocks& tetriminoBlocks = tetrimino.blocks();
                    for (const Position<int>& block : tetriminoBlocks)
                        grid[block.y][block.x] = tetriminoColour;

                    const Tetris::Tetrimino::Type nextType =
                        static_cast<Tetris::Tetrimino::Type>(generateRandomIndex(6));
                    tetrimino = Tetris::Tetrimino(nextType, Position<int>{ 4, 2 });
                }
                
                updatesSinceLastDrop = newUpdatesSinceLastDrop;

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