#pragma once

#include "neuralnetwork.h"
#include "tetris.h"

#include <algorithm>
#include <fstream>
#include <cstddef>
#include <random>
#include <chrono>
#include <string>
#include <array>
#include <cmath>

static double sigmoid(const double x)
{
    return 1.0 / (1.0 + std::exp(-x));
}

static double sigmoidDerivative(const double sigmoid) noexcept
{
    return sigmoid * (1.0 - sigmoid);
}

void NeuralNetwork::randomise()
{
    const std::uniform_real_distribution<double> uniformDistribution{ -1.0, 1.0 };
    const auto currentTime = std::chrono::system_clock::now();
    const auto rngSeed = currentTime.time_since_epoch().count();
    std::minstd_rand rng{ static_cast<unsigned>(rngSeed) };

    const auto randomNumber = [&uniformDistribution, &rng]() { return uniformDistribution(rng); };

    std::generate(inputToHiddenWeights_.begin(), inputToHiddenWeights_.end(), randomNumber);
    std::generate(hiddenBiases_.begin(), hiddenBiases_.end(), randomNumber);
    std::generate(hiddenToOutputWeights_.begin(), hiddenToOutputWeights_.end(), randomNumber);
    std::generate(outputBiases_.begin(), outputBiases_.end(), randomNumber);
}

bool NeuralNetwork::loadFromFile(const char* const fileName)
{
    std::ifstream inputFile{ fileName };
    if (!inputFile.is_open())
        return false;

    std::string line;
    while (std::getline(inputFile, line))
    {
        if (line.empty())
            continue;

        if (line == "#learning_rate")
        {
            inputFile >> learningRate_;
        }
        else if (line == "#input_to_hidden_weights")
        {
            std::copy_n(std::istream_iterator<double>(inputFile),
                inputToHiddenWeights_.size(), inputToHiddenWeights_.begin());
        }
        else if (line == "#hidden_biases")
        {
            std::copy_n(std::istream_iterator<double>(inputFile),
                hiddenBiases_.size(), hiddenBiases_.begin());
        }
        else if (line == "#hidden_to_output_weights")
        {
            std::copy_n(std::istream_iterator<double>(inputFile),
                hiddenToOutputWeights_.size(), hiddenToOutputWeights_.begin());
        }
        else if (line == "#output_biases")
        {
            std::copy_n(std::istream_iterator<double>(inputFile),
                outputBiases_.size(), outputBiases_.begin());
        }
    }

    return true;
}

bool NeuralNetwork::saveToFile(const char* const fileName) const
{
    constexpr auto copy = [](const auto& container, auto outputIterator) {
        std::copy(container.begin(), container.end(), outputIterator);
    };

    std::ofstream outputFile{ fileName };
    if (!outputFile.is_open())
        return false;

    outputFile << "#learning_rate\n";
    outputFile << learningRate_ << "\n\n";

    outputFile << "#input_to_hidden_weights\n";
    copy(inputToHiddenWeights_, std::ostream_iterator<double>(outputFile, " "));
    outputFile << "\n\n";

    outputFile << "#hidden_biases\n";
    copy(hiddenBiases_, std::ostream_iterator<double>(outputFile, " "));
    outputFile << "\n\n";

    outputFile << "#hidden_to_output_weights\n";
    copy(hiddenToOutputWeights_, std::ostream_iterator<double>(outputFile, " "));
    outputFile << "\n\n";

    outputFile << "#output_biases\n";
    copy(outputBiases_, std::ostream_iterator<double>(outputFile, " "));

    return true;
}

NeuralNetwork::OutputLayer NeuralNetwork::feedForward(const InputLayer& input) const
{
    return feedThroughOutputLayer(feedThroughHiddenLayer(input));
}

void NeuralNetwork::backPropogate(const InputLayer& input, const OutputLayer& target)
{
    const HiddenLayer hidden = feedThroughHiddenLayer(input);
    const OutputLayer output = feedThroughOutputLayer(hidden);

    const OutputLayer hiddenToOutputError = target - output;
    const OutputLayer hiddenToOutputGradient =
        learningRate_ * (hiddenToOutputError % lal::map(output, sigmoidDerivative));

    hiddenToOutputWeights_ += hiddenToOutputGradient * lal::transpose(hidden);
    outputBiases_ += hiddenToOutputGradient;

    const HiddenLayer inputToHiddenError =
        lal::transpose(hiddenToOutputWeights_) * hiddenToOutputError;
    const HiddenLayer inputToHiddenGradient =
        learningRate_ * (inputToHiddenError % lal::map(hidden, sigmoidDerivative));

    inputToHiddenWeights_ += inputToHiddenGradient * lal::transpose(input);
    hiddenBiases_ += inputToHiddenGradient;
}

NeuralNetwork::HiddenLayer NeuralNetwork::feedThroughHiddenLayer(const InputLayer& input) const
{
    return lal::map(inputToHiddenWeights_ * input + hiddenBiases_, sigmoid);
}

NeuralNetwork::OutputLayer NeuralNetwork::feedThroughOutputLayer(const HiddenLayer& input) const
{
    return lal::map(hiddenToOutputWeights_ * input + outputBiases_, sigmoid);
}

static constexpr std::size_t GameStateBytes = 54;
using BinaryGameState = std::array<char, GameStateBytes>;
using BinaryPlayerInput = std::int16_t;

static NeuralNetwork::InputLayer toNeuralNetworkInputs(const BinaryGameState& gameStateFromFile) noexcept
{
    NeuralNetwork::InputLayer inputs{};
    const char* const data = gameStateFromFile.data();
    inputs[0][0] = static_cast<double>(*reinterpret_cast<const std::int32_t*>(data));
    inputs[1][0] = static_cast<double>(*reinterpret_cast<const std::int32_t*>(data + 4));
    inputs[2][0] = static_cast<double>(*reinterpret_cast<const char*>(data + 8));
    inputs[3][0] = static_cast<double>(*reinterpret_cast<const char*>(data + 9));

    for (int index = 4, offset = 10; index < 12; index += 2, offset += 2)
    {
        const std::int16_t encodedBlockPosition = *reinterpret_cast<const std::int16_t*>(data + offset);
        inputs[index][0] = static_cast<double>((encodedBlockPosition & 0xFF00) >> 8);
        inputs[index + 1][0] = static_cast<double>(encodedBlockPosition & 0xFF);
    }

    for (int row = 0, index = 12, offset = 18; row < Tetris::Grid::Rows; ++row, offset += 2)
    {
        const std::int16_t rowState = *reinterpret_cast<const std::int16_t*>(data + offset);
        for (int column = 0; column < Tetris::Grid::Columns; ++column, ++index)
        {
            const auto val = (rowState & (1 << column));
            inputs[index][0] = static_cast<double>(val != 0);
        }
    }

    return inputs;
}

static constexpr NeuralNetwork::OutputLayer toNeuralNetworkOutputs(const std::int16_t encodedInputs) noexcept
{
    NeuralNetwork::OutputLayer outputs{};
    for (std::size_t i = 0; i < NeuralNetwork::Outputs; ++i)
        outputs[i][0] = ((encodedInputs << i) != 0);

    return outputs;
}

NeuralNetwork train(NeuralNetwork neuralNetwork, const char* const trainingDataFileName)
{
    std::ifstream trainingDataFile{ trainingDataFileName, std::ios::binary };

    while (!trainingDataFile.eof())
    {
        BinaryGameState binaryGameState;
        trainingDataFile.read(binaryGameState.data(), GameStateBytes);
        std::ifstream::pos_type pos = trainingDataFile.tellg();

        BinaryPlayerInput binaryPlayerInput = 0;
        trainingDataFile.read(reinterpret_cast<char*>(&binaryPlayerInput), sizeof(binaryPlayerInput));
        pos = trainingDataFile.tellg();

        const NeuralNetwork::InputLayer gameState = toNeuralNetworkInputs(binaryGameState);
        const NeuralNetwork::OutputLayer playerInput = toNeuralNetworkOutputs(binaryPlayerInput);
        neuralNetwork.backPropogate(gameState, playerInput);
    }

    return neuralNetwork;
}
