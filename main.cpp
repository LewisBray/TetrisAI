#include "GLEW\\glew.h"

#include "currenttime.h"
#include "render.h"
#include "tetris.h"
#include "input.h"
#include "glfw.h"

#include <exception>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <chrono>

#include <fstream>
#include <array>

std::array<char, 56> writeGameStateToBuffer(const int difficultyLevel,
    const int rowsCleared, const Tetris::Tetrimino::Type nextTetriminoType,
    const Tetris::Tetrimino& currentTetrimino, const Tetris::Grid& grid,
    const InputHistory& inputHistory)
{
    std::array<char, 56> buffer{};
    std::copy_n(reinterpret_cast<const char*>(&difficultyLevel), 4, buffer.begin());
    std::copy_n(reinterpret_cast<const char*>(&rowsCleared), 4, buffer.begin() + 4);
    buffer[8] = static_cast<char>(nextTetriminoType);
    buffer[9] = static_cast<char>(currentTetrimino.type());

    constexpr auto encodeAsInt16 = [](const Position<int> position) {
        return static_cast<std::int16_t>((position.x << 8) + position.y);
    };

    const Tetris::Tetrimino::Blocks& tetriminoBlocks = currentTetrimino.blocks();
    for (auto [blockPosition, offset] = std::make_pair(tetriminoBlocks.begin(), 10);
        blockPosition != tetriminoBlocks.end(); ++blockPosition, offset += 2)
    {
        const std::int16_t position = encodeAsInt16(*blockPosition);
        std::copy_n(reinterpret_cast<const char*>(&position), 2, buffer.begin() + offset);
    }

    for (int row = 0, offset = 18; row < Tetris::Grid::Rows; ++row, offset += 2)
    {
        int rowState = 0;
        for (int column = 0; column < Tetris::Grid::Columns; ++column)
            rowState |= (grid[row][column].has_value() << column);

        std::copy_n(reinterpret_cast<const char*>(&rowState), 2, buffer.begin() + offset);
    }

    // This could be saved in a byte but saving it in 2 makes
    // the whole game state a multiple of 8 bytes in the file
    int playerInputState = 0;
    playerInputState |= (inputHistory.down.currentState != KeyState::Released);
    playerInputState |= ((inputHistory.left.currentState != KeyState::Released) << 1);
    playerInputState |= ((inputHistory.right.currentState != KeyState::Released) << 2);
    playerInputState |= ((inputHistory.clockwise.currentState != KeyState::Released) << 3);
    playerInputState |= ((inputHistory.antiClockwise.currentState != KeyState::Released) << 4);
    std::copy_n(reinterpret_cast<const char*>(&playerInputState), 2, buffer.begin() + 54);

    return buffer;
}

std::array<double, 188> toNeuralNetworkInputs(const std::array<char, 56>& gameStateFromFile) noexcept
{
    std::array<double, 188> inputs{};
    const char* const data = gameStateFromFile.data();
    inputs[0] = static_cast<double>(*reinterpret_cast<const std::int32_t*>(data));
    inputs[1] = static_cast<double>(*reinterpret_cast<const std::int32_t*>(data + 4));
    inputs[2] = static_cast<double>(*reinterpret_cast<const char*>(data + 8));
    inputs[3] = static_cast<double>(*reinterpret_cast<const char*>(data + 9));
    
    for (int index = 4, offset = 10; index <= 4 + 4; index += 2, offset += 2)
    {
        const std::int16_t encodedBlockPosition = *reinterpret_cast<const std::int16_t*>(data + offset);
        inputs[index] = static_cast<double>(encodedBlockPosition & 0xFF00);
        inputs[index + 1] = static_cast<double>(encodedBlockPosition & 0xFF);
    }

    for (int row = 0, index = 12, offset = 18; row < Tetris::Grid::Rows; ++row)
    {
        const std::int16_t rowState = *reinterpret_cast<const std::int16_t*>(data + offset);
        for (int column = 0; column < Tetris::Grid::Columns; ++column, ++index, ++offset)
            inputs[index] = static_cast<double>((rowState & (1 << column)) != 0);
    }
    
    return inputs;
}

std::array<double, 188> toNeuralNetworkInputs(const int difficultyLevel,
    const int rowsCleared, const Tetris::Tetrimino::Type nextTetriminoType,
    const Tetris::Tetrimino& currentTetrimino, const Tetris::Grid& grid,
    const InputHistory& inputHistory)
{
    std::array<double, 188> inputs{};
    inputs[0] = static_cast<double>(difficultyLevel);
    inputs[1] = static_cast<double>(rowsCleared);
    inputs[2] = static_cast<double>(nextTetriminoType);
    inputs[3] = static_cast<double>(currentTetrimino.type());

    const Tetris::Tetrimino::Blocks& tetriminoBlocks = currentTetrimino.blocks();
    for (auto [index, blockPosition] = std::make_pair(4, tetriminoBlocks.begin());
        blockPosition != tetriminoBlocks.end(); index += 2, ++blockPosition)
    {
        inputs[index] = static_cast<double>(blockPosition->x);
        inputs[index + 1] = static_cast<double>(blockPosition->y);
    }

    for (int row = 0, index = 12; row < Tetris::Grid::Rows; ++row)
        for (int column = 0; column < Tetris::Grid::Columns; ++column, ++index)
            inputs[index] = static_cast<double>(grid[row][column].has_value());

    return inputs;
}

constexpr std::array<double, 5> toNeuralNetworkOuputs(const char encodedInputs) noexcept
{
    std::array<double, 5> outputs{};
    for (std::size_t i = 0; i < outputs.size(); ++i)
        outputs[i] = ((encodedInputs << i) != 0);

    return outputs;
}

constexpr PlayerInput toPlayerInput(const std::array<double, 5>& neuralNetworkOuput) noexcept
{
    constexpr auto toKeyState = [](const double d) noexcept {
        return ((d == 0.0) ? KeyState::Released : KeyState::Pressed);
    };

    PlayerInput translatedInput{};
    translatedInput.down = toKeyState(neuralNetworkOuput[0]);
    translatedInput.left = toKeyState(neuralNetworkOuput[1]);
    translatedInput.right = toKeyState(neuralNetworkOuput[2]);
    translatedInput.rotateClockwise = toKeyState(neuralNetworkOuput[3]);
    translatedInput.rotateAntiClockwise = toKeyState(neuralNetworkOuput[4]);

    return translatedInput;
}

using namespace std::literals::chrono_literals;

int main()
{
    try
    {
        const GLFW::GLFW glfw;

        const GLFW::Window window(640, 480, "Tetris AI");
        window.makeCurrentContext();

        if (glewInit() != GLEW_OK)
            throw std::runtime_error{ "Failed to initialise GLEW" };

        std::cout << glGetString(GL_VERSION) << std::endl;

        std::ofstream trainingData("training_data.bin", std::ios::binary);

		Tetris::Grid grid;

        Tetris::Tetrimino tetrimino =
            Tetris::randomTetrimino(Tetris::Tetrimino::SpawnLocation);
        Tetris::Tetrimino nextTetrimino =
            Tetris::randomTetrimino(Tetris::Tetrimino::SpawnLocation);

		InputHistory inputHistory;
        int playerScore = 0, totalRowsCleared = 0, updatesSinceLastDrop = 0;
		static constexpr auto frameDuration = 16667us;      // ~60fps
		auto accumulatedTime = 0us, previousTime = currentTime();
        while (!window.shouldClose())
        {
			const auto time = currentTime();
			accumulatedTime += time - previousTime;
			previousTime = time;

			while (accumulatedTime >= frameDuration)
			{
                inputHistory.update(getPlayerInput(window));
                const int difficultyLevel = totalRowsCleared / 10 + 1;

                const auto gameStateInBuffer = writeGameStateToBuffer(difficultyLevel,
                    totalRowsCleared, nextTetrimino.type(), tetrimino, grid, inputHistory);

                trainingData.write(gameStateInBuffer.data(), gameStateInBuffer.size());
                
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

                const int rowsCleared = grid.removeCompletedRows();
                totalRowsCleared += rowsCleared;
                playerScore += rowsCleared * 100 * difficultyLevel;

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