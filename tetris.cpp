#include "GLEW\\glew.h"
#include "GLFW\\glfw3.h"

#include "tetris.h"
#include "render.h"
#include "input.h"

#include <unordered_map>
#include <stdexcept>
#include <utility>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>

// Calling glfwTerminate also clears up any windows/resources so
// making this RAII struct to make sure it's definitely called
struct GLFW
{
    GLFW()
    {
        if (!glfwInit())
            throw std::runtime_error{ "Failed to initialise GLFW" };
    }

    ~GLFW()
    {
        glfwTerminate();
    }
};

static std::chrono::microseconds currentTime()
{
    using namespace std::chrono;

    const auto clockTime = steady_clock().now();
    return duration_cast<microseconds>(clockTime.time_since_epoch());
}

static std::array<char, 56> writeGameStateToBuffer(const int difficultyLevel,
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

static std::array<double, 188> toNeuralNetworkInputs(const int difficultyLevel,
    const int rowsCleared, const Tetris::Tetrimino::Type nextTetriminoType,
    const Tetris::Tetrimino & currentTetrimino, const Tetris::Grid & grid,
    const InputHistory & inputHistory)
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

static constexpr PlayerInput toPlayerInput(const std::array<double, 5>& neuralNetworkOutput) noexcept
{
    constexpr auto toKeyState = [](const double d) noexcept {
        return (d == 0.0) ? KeyState::Released : KeyState::Pressed;
    };

    PlayerInput translatedInput{};
    translatedInput.down = toKeyState(neuralNetworkOutput[0]);
    translatedInput.left = toKeyState(neuralNetworkOutput[1]);
    translatedInput.right = toKeyState(neuralNetworkOutput[2]);
    translatedInput.clockwise = toKeyState(neuralNetworkOutput[3]);
    translatedInput.antiClockwise = toKeyState(neuralNetworkOutput[4]);

    return translatedInput;
}

namespace Tetris
{
    // This could be a map but then it can't be constexpr and we would have to
    // write a function to retrieve the colour value for the Tetrimino constructor
    // if we want the map to be const so this seemed the simplest solution
    static constexpr Colour pieceColour(Tetrimino::Type type)
    {
        switch (type)
        {
        case Tetrimino::Type::T:        return Purple;
        case Tetrimino::Type::L:        return Cyan;
        case Tetrimino::Type::RL:       return Green;
        case Tetrimino::Type::S:        return Red;
        case Tetrimino::Type::Z:        return Blue;
        case Tetrimino::Type::Square:   return Yellow;
        case Tetrimino::Type::Long:     return White;

        default:
            throw std::domain_error {
				"Invalid Tetrimino type in function 'pieceColour'"
			};
        }
    }

    static constexpr Position<float> centreLocation(
        const Tetrimino::Type type, const Position<int>& topLeft)
    {
        switch (type)
        {
        case Tetrimino::Type::T:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::L:
            return topLeft + Position<float>{ 0.5f, 1.5f };

        case Tetrimino::Type::RL:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::S:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::Z:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::Square:
            return topLeft + Position<float>{ 1.0f, 1.0f };

        case Tetrimino::Type::Long:
            return topLeft + Position<float>{ 0.0f, 2.0f };

        default:
            throw std::domain_error {
				"Invalid Tetrimino type used in function 'centreLocation'"
			};
        }
    }

    static constexpr Tetrimino::Blocks blockLocations(
        const Tetrimino::Type type, const Position<int>& topLeft)
    {
        switch (type)
        {
        case Tetrimino::Type::T:
            return {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 2, 1 }
            };

        case Tetrimino::Type::L:
            return {
                topLeft,
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 0, 2 },
                topLeft + Position<int>{ 1, 2 }
            };

        case Tetrimino::Type::RL:
            return {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 1, 2 },
                topLeft + Position<int>{ 0, 2 }
            };

        case Tetrimino::Type::S:
            return {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 2, 0 },
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 1, 1 }
            };

        case Tetrimino::Type::Z:
            return {
                topLeft,
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 2, 1 }
            };

        case Tetrimino::Type::Square:
            return {
                topLeft,
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 0, 1 }
            };

        case Tetrimino::Type::Long:
            return {
                topLeft,
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 0, 2 },
                topLeft + Position<int>{ 0, 3 }
            };

        default:
            throw std::domain_error {
				"Invalid Tetrimino type in used in function 'blockLocations'"
			};
        }
    }

    Tetrimino::Tetrimino(const Type type, const Position<int>& topLeft)
        : blocks_{ blockLocations(type, topLeft) }
        , centre_{ centreLocation(type, topLeft) }
        , colour_{ pieceColour(type) }
        , type_{ type }
    {}

    void Tetrimino::rotate(const Direction direction) noexcept
    {
        if (direction != Direction::Clockwise &&
            direction != Direction::AntiClockwise)
            return;

        for (Position<int>& blockTopLeft : blocks_)
        {
            const Position<float> blockCentre =
                blockTopLeft + Position<float>{ 0.5f, 0.5f };

            const Position<float> rotatedBlockCentre =
                (direction == Direction::Clockwise) ?
            Position<float>{
                -blockCentre.y + centre_.x + centre_.y,
                 blockCentre.x - centre_.x + centre_.y
            } :
            Position<float>{
                 blockCentre.y + centre_.x - centre_.y,
                -blockCentre.x + centre_.x + centre_.y
            };

            blockTopLeft.x = static_cast<int>(std::floor(rotatedBlockCentre.x));
            blockTopLeft.y = static_cast<int>(std::floor(rotatedBlockCentre.y));
        }
    }

	void Tetrimino::shift(const Position<int>& shift) noexcept
	{
		for (Position<int>& block : blocks_)
			block += shift;

		centre_ += shift;
	}

    static bool shouldMove(const Tetrimino::Direction direction, const InputHistory& inputHistory)
    {

        constexpr auto check = [](const KeyStateHistory& keyStateHistory) noexcept {
            static constexpr int Delay = 3;

            return ((keyStateHistory.currentState == KeyState::Pressed &&
                keyStateHistory.previousState != KeyState::Pressed) ||
                (keyStateHistory.currentState == KeyState::Held &&
                keyStateHistory.numUpdatesInHeldState % Delay == 0));
        };

        switch (direction)
        {
        case Tetrimino::Direction::Down:
            return check(inputHistory.down);

        case Tetrimino::Direction::Left:
            return check(inputHistory.left);

        case Tetrimino::Direction::Right:
            return check(inputHistory.right);

        case Tetrimino::Direction::Clockwise:
            return check(inputHistory.clockwise);
        
        case Tetrimino::Direction::AntiClockwise:
            return check(inputHistory.antiClockwise);

        default:
            throw std::domain_error {
                "Unhandled player input in function 'shouldMove'"
            };
        }
    }

    bool collision(const Tetrimino& tetrimino, const Grid& grid)
    {
        const auto overlapsGrid = [&grid](const Position<int>& block) noexcept {
            return (block.x < 0 || block.x >= Grid::Columns ||
                block.y >= Grid::Rows || grid[block.y][block.x].has_value());
        };

        const Tetrimino::Blocks& blocks = tetrimino.blocks();
        return std::any_of(blocks.begin(), blocks.end(), overlapsGrid);
    }

    std::pair<bool, int> Tetrimino::update(const InputHistory& inputHistory,
        const Grid& grid, const int difficultyLevel, const int updatesSinceLastDrop)
    {
        if (shouldMove(Direction::Left, inputHistory))
        {
            shift({ -1, 0 });
            if (collision(*this, grid))
                shift({ 1, 0 });
        }

        if (shouldMove(Direction::Right, inputHistory))
        {
            shift({ 1, 0 });
            if (collision(*this, grid))
                shift({ -1, 0 });
        }

        // Fastest we can get is 6 drops per second
        const int updatesAllowedBeforeDrop = std::invoke([&difficultyLevel]() {
            const int updatesAllowed = 120 - 5 * (difficultyLevel - 1);
            return (updatesAllowed < 10) ? 10 : updatesAllowed;
        });

        const bool dropTetrimino =
            (shouldMove(Direction::Down, inputHistory) ||
            updatesSinceLastDrop >= updatesAllowedBeforeDrop);
        if (dropTetrimino)
        {
            shift({ 0, 1 });
            if (collision(*this, grid)) // Then we need to merge tetrimino...
            {						    // ...to grid and spawn another
                shift({ 0, -1 });
                return std::make_pair(true, 0);
            }
        }

        if (shouldMove(Direction::Clockwise, inputHistory))
        {
            rotate(Direction::Clockwise);

            if (collision(*this, grid) && !resolveRotationCollision(grid))
                rotate(Direction::AntiClockwise);
        }

        if (shouldMove(Direction::AntiClockwise, inputHistory))
        {
            rotate(Direction::AntiClockwise);

            if (collision(*this, grid) && !resolveRotationCollision(grid))
                rotate(Direction::Clockwise);
        }

        return std::make_pair(false, dropTetrimino ? 0 : updatesSinceLastDrop);
    }

    bool Tetrimino::resolveRotationCollision(const Grid& grid) noexcept
    {
        const std::pair<Blocks, Position<float>> initialState{ blocks_, centre_ };

        static constexpr int MaxNumAttempts = 4;
        for (int attempt = 1; attempt <= MaxNumAttempts; ++attempt)
        {
            const int xShift = ((attempt % 2 == 0) ? 1 : -1) * attempt;

            shift({ xShift, 0 });
            if (!collision(*this, grid))
                return true;
        }

        std::tie(blocks_, centre_) = initialState;
        return false;
    }

    Tetrimino randomTetrimino(const Position<int>& position)
    {
        const auto time = currentTime();
        const auto rngSeed = static_cast<std::uint32_t>(time.count());
        
        std::minstd_rand rng(rngSeed);
        const std::uniform_int_distribution<int>
            uniformDist(0, Tetrimino::TotalTypes - 1);

        const Tetrimino::Type randomType =
            static_cast<Tetrimino::Type>(uniformDist(rng));

        return Tetrimino(randomType, position);
    }

    Tetrimino::Blocks nextTetriminoBlockDisplayLocations(const Tetrimino::Type type)
    {
        static constexpr Position<int> DisplayLocationTopLeft = { 15, 13 };
        
        return blockLocations(type, DisplayLocationTopLeft);
    }

    const std::array<Grid::Cell, Grid::Columns>& Grid::operator[](const int row) const noexcept
    {
        return grid_[row];
    }

    int Grid::removeCompletedRows() noexcept
    {
        constexpr auto rowIsComplete =
            [](const std::array<Cell, Columns>& row) noexcept {
                return std::all_of(row.begin(), row.end(),
                    [](const Cell& cell) { return cell.has_value(); });
        };

        const auto startOfRemovedRows =
            std::remove_if(grid_.rbegin(), grid_.rend(), rowIsComplete);

        constexpr auto clearRow = [](std::array <Cell, Columns>& row) noexcept {
            for (Cell& cell : row)
                cell.reset();
        };

        std::for_each(startOfRemovedRows, grid_.rend(), clearRow);

        const int rowsCleared =
            static_cast<int>(std::distance(startOfRemovedRows, grid_.rend()));

        return rowsCleared;
    }

    void Grid::merge(const Tetrimino& tetrimino) noexcept
    {
        const Colour& tetriminoColour = tetrimino.colour();
        const Tetrimino::Blocks& tetriminoBlocks = tetrimino.blocks();
        for (const Position<int>& block : tetriminoBlocks)
            grid_[block.y][block.x] = tetriminoColour;
    }

    // I can't seem to come up with a way to not make this global or put
    // it in its own tiny .cpp file so going with this approach for now
    using KeyStateMap = std::unordered_map<char, KeyState>;
    KeyStateMap keyStateMap = {
        { 'A', KeyState::Released },
        { 'S', KeyState::Released },
        { 'D', KeyState::Released },
        { 'J', KeyState::Released },
        { 'K', KeyState::Released }
    };

    void keyStateCallback(GLFWwindow* const window, const int key,
        const int scancode, const int action, const int mods)
    {
        const KeyState newState = std::invoke([action]() {
            if (action == GLFW_PRESS)
                return KeyState::Pressed;
            else if (action == GLFW_REPEAT)
                return KeyState::Held;
            else
                return KeyState::Released;
        });

        const char keyAsChar = std::toupper(static_cast<char>(key));
        keyStateMap[keyAsChar] = newState;
    }

    PlayerInput getPlayerInput()
    {
        const KeyState down = keyStateMap['S'];
        const KeyState left = keyStateMap['A'];
        const KeyState right = keyStateMap['D'];
        const KeyState clockwise = keyStateMap['K'];
        const KeyState antiClockwise = keyStateMap['J'];

        return { down, left, right, clockwise, antiClockwise };
    }

    InputHistory update(InputHistory inputHistory, const PlayerInput& playerInput)
    {
        const auto updateKeyStateHistory =
            [](const KeyState newKeyState, const KeyState oldKeyState, int numUpdatesInHeldState) noexcept {
                if (newKeyState == KeyState::Held && oldKeyState == KeyState::Held)
                    ++numUpdatesInHeldState;
                else
                    numUpdatesInHeldState = 0;
                return KeyStateHistory{ newKeyState, oldKeyState, numUpdatesInHeldState };
        };

        inputHistory.down = updateKeyStateHistory(playerInput.down,
            inputHistory.down.currentState, inputHistory.down.numUpdatesInHeldState);
        inputHistory.left = updateKeyStateHistory(playerInput.left,
            inputHistory.left.currentState, inputHistory.left.numUpdatesInHeldState);
        inputHistory.right = updateKeyStateHistory(playerInput.right,
            inputHistory.right.currentState, inputHistory.right.numUpdatesInHeldState);
        inputHistory.clockwise = updateKeyStateHistory(playerInput.clockwise,
            inputHistory.clockwise.currentState, inputHistory.clockwise.numUpdatesInHeldState);
        inputHistory.antiClockwise = updateKeyStateHistory(playerInput.antiClockwise,
            inputHistory.antiClockwise.currentState, inputHistory.antiClockwise.numUpdatesInHeldState);

        return inputHistory;
    }

    void play()
    {
        const GLFW glfw;
        
        GLFWwindow* const window = glfwCreateWindow(640, 480, "Tetris AI", NULL, NULL);
        if (window == nullptr)
            throw std::runtime_error{ "Failed to create window" };

        glfwSetKeyCallback(window, keyStateCallback);
        glfwMakeContextCurrent(window);

        if (glewInit() != GLEW_OK)
            throw std::runtime_error{ "Failed to initialise GLEW" };

        std::ofstream trainingData("training_data.bin", std::ios::binary);

        Grid grid;
        Tetrimino tetrimino = randomTetrimino(Tetrimino::SpawnLocation);
        Tetrimino nextTetrimino = randomTetrimino(Tetrimino::SpawnLocation);

        InputHistory inputHistory;
        int playerScore = 0, totalRowsCleared = 0, updatesSinceLastDrop = 0;

        using namespace std::chrono_literals;
        static constexpr auto frameDuration = 16667us;      // ~60fps
        auto accumulatedTime = 0us, previousTime = currentTime();
        while (!glfwWindowShouldClose(window))
        {
            const auto time = currentTime();
            accumulatedTime += time - previousTime;
            previousTime = time;

            while (accumulatedTime >= frameDuration)
            {
                const PlayerInput playerInput = getPlayerInput();
                inputHistory = update(inputHistory, playerInput);

                const int difficultyLevel = totalRowsCleared / 10 + 1;

                const auto gameStateInBuffer = writeGameStateToBuffer(difficultyLevel,
                    totalRowsCleared, nextTetrimino.type(), tetrimino, grid, inputHistory);

                trainingData.write(gameStateInBuffer.data(), gameStateInBuffer.size());

                const auto [shouldMergeWithGrid, newUpdatesSinceLastDrop] =
                    tetrimino.update(inputHistory, grid, difficultyLevel, updatesSinceLastDrop);

                if (shouldMergeWithGrid)
                {
                    grid.merge(tetrimino);
                    tetrimino = nextTetrimino;
                    if (collision(tetrimino, grid))
                    {
                        playerScore = 0;
                        totalRowsCleared = 0;
                        updatesSinceLastDrop = 0;
                        grid = Grid{};
                    }

                    nextTetrimino = randomTetrimino(Tetrimino::SpawnLocation);
                }

                updatesSinceLastDrop = newUpdatesSinceLastDrop;

                const int rowsCleared = grid.removeCompletedRows();
                totalRowsCleared += rowsCleared;
                playerScore += rowsCleared * 100 * difficultyLevel;

                ++updatesSinceLastDrop;
                accumulatedTime -= frameDuration;
            }

            renderScene(tetrimino, nextTetrimino, grid, playerScore, totalRowsCleared);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
}
