#include "GLEW\\glew.h"

#include "currenttime.h"
#include "render.h"
#include "tetris.h"
#include "input.h"
#include "glfw.h"

#include <exception>
#include <iostream>
#include <utility>
#include <chrono>

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

		Tetris::Grid grid;
        Tetris::Tetrimino tetrimino(Tetris::Tetrimino::Type::T, Position<int>{ 4, 2 });

        BlockDrawer drawBlock;
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
                    grid.merge(tetrimino);
                    tetrimino = Tetris::randomTetrimino(Position<int>{ 4, 2 });
                }
                
                updatesSinceLastDrop = newUpdatesSinceLastDrop;

                grid.update();

				++updatesSinceLastDrop;
				accumulatedTime -= frameDuration;
			}
			
			glClear(GL_COLOR_BUFFER_BIT);

            const Tetris::Tetrimino::Blocks& tetriminoBlocks = tetrimino.blocks();
            for (const Position<int>& blockTopLeft : tetriminoBlocks)
                drawBlock(blockTopLeft, tetrimino.colour());

			for (int col = 0; col < Tetris::Grid::Columns; ++col)
			{
				for (int row = 0; row < Tetris::Grid::Rows; ++row)
				{
					const Tetris::Grid::Cell& cell = grid[row][col];
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