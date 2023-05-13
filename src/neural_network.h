#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include "tetris_ai.h"

struct NeuralNetwork {
    static constexpr i32 INPUT_LAYER_SIZE = 192;
    static constexpr i32 HIDDEN_LAYER_SIZE = 64;
    static constexpr i32 OUTPUT_LAYER_SIZE = 5;

    using InputLayer = f32[INPUT_LAYER_SIZE];
    using HiddenLayer = f32[HIDDEN_LAYER_SIZE];
    using OutputLayer = f32[OUTPUT_LAYER_SIZE];
    using InputToHiddenMatrix = f32[HIDDEN_LAYER_SIZE][INPUT_LAYER_SIZE];
    using HiddenToOutputMatrix = f32[OUTPUT_LAYER_SIZE][HIDDEN_LAYER_SIZE];

    InputToHiddenMatrix input_to_hidden_weights;
    HiddenLayer hidden_biases;
    HiddenToOutputMatrix hidden_to_output_weights;
    OutputLayer output_biases;
};

static NeuralNetwork random_neural_network(u32 rng_seed);
static u32 save_to_buffer(const NeuralNetwork& neural_network, i8* buffer);
static u32 load_from_buffer(NeuralNetwork& neural_network, const i8* buffer, u32 buffer_size);
static void feed_forward(const NeuralNetwork& neural_network, const NeuralNetwork::InputLayer& input, NeuralNetwork::OutputLayer& output);
static void back_propagate(const NeuralNetwork& neural_network, const NeuralNetwork::InputLayer& input, const NeuralNetwork::OutputLayer& target, NeuralNetwork& neural_network_delta);

#endif
