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

        Tetris::Tetrimino tetrimino =
            Tetris::randomTetrimino(Tetris::Tetrimino::SpawnLocation);
        Tetris::Tetrimino nextTetrimino =
            Tetris::randomTetrimino(Tetris::Tetrimino::SpawnLocation);

        BlockDrawer drawBlock;
		InputHistory inputHistory;
        int playerScore = 0;
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
                inputHistory.update(getPlayerInput(window));

                const auto [shouldMergeWithGrid, newUpdatesSinceLastDrop] =
                    tetrimino.update(inputHistory, grid, updatesSinceLastDrop);

                if (shouldMergeWithGrid)
                {
                    grid.merge(tetrimino);
                    tetrimino = nextTetrimino;
                    nextTetrimino =
                        Tetris::randomTetrimino(Tetris::Tetrimino::SpawnLocation);
                }
                
                updatesSinceLastDrop = newUpdatesSinceLastDrop;

                const int rowsCleared = grid.update();
                playerScore += rowsCleared * 100;

                std::cout << playerScore << std::endl;

				++updatesSinceLastDrop;
				accumulatedTime -= frameDuration;
			}

            glClear(GL_COLOR_BUFFER_BIT);
           
            const Tetris::Tetrimino::Blocks& tetriminoBlocks = tetrimino.blocks();
            for (const Position<int>& blockTopLeft : tetriminoBlocks)
            {
                const Position<int> blockDrawPosition =
                    { blockTopLeft.x - Tetris::Grid::DisplayShift, blockTopLeft.y };
                drawBlock(blockDrawPosition, tetrimino.colour());
            }

			for (int row = 0; row < Tetris::Grid::Rows; ++row)
			{
			    for (int col = 0; col < Tetris::Grid::Columns; ++col)
				{
					const Tetris::Grid::Cell& cell = grid[row][col];
					if (!cell.has_value())
						continue;
					
                    const Position<int> cellDrawPosition =
                        { col - Tetris::Grid::DisplayShift, row };
                    drawBlock(cellDrawPosition, cell.value());
				}
			}

            for (int y = 0; y < Tetris::Grid::Rows; ++y)
            {
                const Position<int> leftOfGridBlock =
                    { 0 - Tetris::Grid::DisplayShift - 1, y };
                const Position<int> rightOfGridBlock =
                    { leftOfGridBlock.x + 1 + Tetris::Grid::Columns, y };
                drawBlock(leftOfGridBlock, Grey);
                drawBlock(rightOfGridBlock, Grey);
            }
        
            const Tetris::Tetrimino::Blocks nextTetriminoDisplayBlocks =
                Tetris::nextTetriminoBlockDisplayLocations(nextTetrimino.type());
            for (const Position<int>& blockTopLeft : nextTetriminoDisplayBlocks)
                drawBlock(blockTopLeft, nextTetrimino.colour());

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