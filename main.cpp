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

		InputHistory inputHistory;
        int playerScore = 0, totalRowsCleared = 0, updatesSinceLastDrop = 0;
		static constexpr std::chrono::milliseconds frameDuration{ 1000 / 60 };
		std::chrono::milliseconds accumulatedTime{ 0 }, previousTime = currentTime();
        while (!window.shouldClose())
        {
			const std::chrono::milliseconds time = currentTime();
			accumulatedTime += time - previousTime;
			previousTime = time;

			while (accumulatedTime >= frameDuration)
			{
                inputHistory.update(getPlayerInput(window));

                const int difficultyLevel = totalRowsCleared / 10 + 1;

                const auto [shouldMergeWithGrid, newUpdatesSinceLastDrop] =
                    tetrimino.update(inputHistory,
                        grid, difficultyLevel, updatesSinceLastDrop);

                if (shouldMergeWithGrid)
                {
                    grid.merge(tetrimino);
                    tetrimino = nextTetrimino;
                    if (Tetris::collision(tetrimino, grid))
                    {
                        playerScore = 0;
                        totalRowsCleared = 0;
                        updatesSinceLastDrop = 0;
                        grid = Tetris::Grid{};
                    }

                    nextTetrimino =
                        Tetris::randomTetrimino(Tetris::Tetrimino::SpawnLocation);
                }
                
                updatesSinceLastDrop = newUpdatesSinceLastDrop;

                const int rowsCleared = grid.update();
                totalRowsCleared += rowsCleared;
                playerScore += rowsCleared * 100 * difficultyLevel;
                
                std::cout << totalRowsCleared << std::endl;
                
				++updatesSinceLastDrop;
				accumulatedTime -= frameDuration;
			}
            
            renderScene(tetrimino, nextTetrimino,
                grid, playerScore, totalRowsCleared);

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