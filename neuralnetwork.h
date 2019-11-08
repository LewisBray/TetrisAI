#pragma once

#include "matrix.hpp"

#include <cstddef>

class NeuralNetwork
{
public:
    static constexpr std::size_t Inputs = 192;
    static constexpr std::size_t Hiddens = 64;
    static constexpr std::size_t Outputs = 5;

    using InputLayer = lal::column_vector<double, Inputs>;
    using HiddenLayer = lal::column_vector<double, Hiddens>;
    using OutputLayer = lal::column_vector<double, Outputs>;

    void randomise();
    bool loadFromFile(const char* fileName);
    bool saveToFile(const char* fileName) const;

    OutputLayer feedForward(const InputLayer& input) const;
    void backPropogate(const InputLayer& input, const OutputLayer& target);

private:
    HiddenLayer feedThroughHiddenLayer(const InputLayer& input) const;
    OutputLayer feedThroughOutputLayer(const HiddenLayer& input) const;

    double learningRate_ = 0.01;
    lal::matrix<double, Hiddens, Inputs> inputToHiddenWeights_;
    HiddenLayer hiddenBiases_;
    lal::matrix<double, Outputs, Hiddens> hiddenToOutputWeights_;
    OutputLayer outputBiases_;
};

NeuralNetwork train(NeuralNetwork neuralNetwork, const char* trainingDataFile);
