#include "neural_network.h"
#include "util.h"

// TODO: not true that we are on domain [-1, 1] due to biases being able to be larger
// Taylor series approximation, gives very close answers on [-1, 1] domain which is all we care about.
// Will probably have to tweek number of terms based on accuracy/performance. Efficient expansion based
// on observation that 1 + x + x^2/2! + x^3/3! + ... = 1 + x/1(1 + (x/2)(1 + (x/3)(...)))).
static f32 exp(const f32 x) {
    f32 result = 1.0f;
    result = 1.0f + x / 8.0f * result;
    result = 1.0f + x / 7.0f * result;
    result = 1.0f + x / 6.0f * result;
    result = 1.0f + x / 5.0f * result;
    result = 1.0f + x / 4.0f * result;
    result = 1.0f + x / 3.0f * result;
    result = 1.0f + x / 2.0f * result;
    result = 1.0f + x / 1.0f * result;

    return result;
}

// TODO: replace sigmoid with ReLU
static f32 sigmoid(const f32 x) {
    return 1.0f / (1.0f + exp(-x));
}

// TODO: what is "derivative" of ReLU?
static f32 sigmoid_derivative(const f32 x) {
    const f32 sigmoid_x = sigmoid(x);
    return sigmoid_x * (1.0f - sigmoid_x);
}

static NeuralNetwork random_neural_network(u32 rng_seed) {
    NeuralNetwork neural_network = {};
    for (i32 row = 0; row < NeuralNetwork::HIDDEN_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::INPUT_LAYER_SIZE; ++column) {
            rng_seed = random_number(rng_seed);
            const f32 random_numerator = static_cast<f32>(rng_seed % 1000);
            const f32 random_float = random_numerator / 500.0f - 1.0f;
            neural_network.input_to_hidden_weights[row][column] = random_float;
        }
    }

    for (i32 i = 0; i < NeuralNetwork::HIDDEN_LAYER_SIZE; ++i) {
        rng_seed = random_number(rng_seed);
        const f32 random_numerator = static_cast<f32>(rng_seed % 1000);
        const f32 random_float = random_numerator / 500.0f - 1.0f;
        neural_network.hidden_biases[i] = random_float;
    }

    for (i32 row = 0; row < NeuralNetwork::OUTPUT_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::HIDDEN_LAYER_SIZE; ++column) {
            rng_seed = random_number(rng_seed);
            const f32 random_numerator = static_cast<f32>(rng_seed % 1000);
            const f32 random_float = random_numerator / 500.0f - 1.0f;
            neural_network.hidden_to_output_weights[row][column] = random_float;
        }
    }

    for (i32 i = 0; i < NeuralNetwork::OUTPUT_LAYER_SIZE; ++i) {
        rng_seed = random_number(rng_seed);
        const f32 random_numerator = static_cast<f32>(rng_seed % 1000);
        const f32 random_float = random_numerator / 500.0f - 1.0f;
        neural_network.output_biases[i] = random_float;
    }

    return neural_network;
}

static constexpr i8 NEURAL_NETWORK_HEADER[] = {'T', 'E', 'T', 'R', 'I', 'S', 'A', 'I'};

static u32 save_to_buffer(const NeuralNetwork& neural_network, i8* const buffer) {
    u32 bytes_written = 0;
    bytes_written += copy_bytes(NEURAL_NETWORK_HEADER, sizeof(NEURAL_NETWORK_HEADER), buffer + bytes_written);
    bytes_written += copy_bytes(reinterpret_cast<const i8*>(&NeuralNetwork::INPUT_LAYER_SIZE), sizeof(NeuralNetwork::INPUT_LAYER_SIZE), buffer + bytes_written);
    bytes_written += copy_bytes(reinterpret_cast<const i8*>(&NeuralNetwork::HIDDEN_LAYER_SIZE), sizeof(NeuralNetwork::HIDDEN_LAYER_SIZE), buffer + bytes_written);
    bytes_written += copy_bytes(reinterpret_cast<const i8*>(&NeuralNetwork::OUTPUT_LAYER_SIZE), sizeof(NeuralNetwork::OUTPUT_LAYER_SIZE), buffer + bytes_written);
    bytes_written += copy_bytes(reinterpret_cast<const i8*>(&neural_network), sizeof(neural_network), buffer + bytes_written);

    return bytes_written;
}

static u32 load_from_buffer(NeuralNetwork& neural_network, const i8* const buffer, const u32 buffer_size) {
    const u32 necessary_buffer_size = 8 +
        sizeof(NeuralNetwork::INPUT_LAYER_SIZE) +
        sizeof(NeuralNetwork::HIDDEN_LAYER_SIZE) +
        sizeof(NeuralNetwork::OUTPUT_LAYER_SIZE) +
        sizeof(NeuralNetwork);
    if (buffer_size < necessary_buffer_size) {
        return 0;
    }

    u32 bytes_read = 0;

    i8 header_buffer[sizeof(NEURAL_NETWORK_HEADER)] = {};
    bytes_read += copy_bytes(buffer, sizeof(header_buffer), header_buffer);
    if (compare_bytes(NEURAL_NETWORK_HEADER, header_buffer, sizeof(header_buffer)) != 0) {
        return 0;
    }

    i32 input_layer_size = 0;
    bytes_read += copy_bytes(buffer + bytes_read, sizeof(input_layer_size), reinterpret_cast<i8*>(&input_layer_size));
    if (input_layer_size != NeuralNetwork::INPUT_LAYER_SIZE) {
        return 0;
    }

    i32 hidden_layer_size = 0;
    bytes_read += copy_bytes(buffer + bytes_read, sizeof(hidden_layer_size), reinterpret_cast<i8*>(&hidden_layer_size));
    if (hidden_layer_size != NeuralNetwork::HIDDEN_LAYER_SIZE) {
        return 0;
    }

    i32 output_layer_size = 0;
    bytes_read += copy_bytes(buffer + bytes_read, sizeof(output_layer_size), reinterpret_cast<i8*>(&output_layer_size));
    if (output_layer_size != NeuralNetwork::OUTPUT_LAYER_SIZE) {
        return 0;
    }

    bytes_read += copy_bytes(buffer + bytes_read, sizeof(neural_network), reinterpret_cast<i8*>(&neural_network));

    return bytes_read;
}

static void feed_forward(const NeuralNetwork& neural_network, const NeuralNetwork::InputLayer& input, NeuralNetwork::OutputLayer& output) {
    // feed through hidden layer
    NeuralNetwork::HiddenLayer hidden_activations = {};
    for (i32 row = 0; row < NeuralNetwork::HIDDEN_LAYER_SIZE; ++row) {
        f32 z = 0.0f;
        for (i32 column = 0; column < NeuralNetwork::INPUT_LAYER_SIZE; ++column) {
            z += neural_network.input_to_hidden_weights[row][column] * input[column];
        }

        hidden_activations[row] = sigmoid(z);
    }

    // feed through output layer
    for (i32 row = 0; row < NeuralNetwork::OUTPUT_LAYER_SIZE; ++row) {
        f32 z = 0.0f;
        for (i32 column = 0; column < NeuralNetwork::HIDDEN_LAYER_SIZE; ++column) {
            z += neural_network.hidden_to_output_weights[row][column] * input[column];
        }

        output[row] = sigmoid(z);
    }
}

static f32 cost_derivative(const f32 activation, const f32 target) {
    return activation - target;
}

static void back_propagate(
    const NeuralNetwork& neural_network,
    const NeuralNetwork::InputLayer& input,
    const NeuralNetwork::OutputLayer& target,
    NeuralNetwork& neural_network_delta
) {
    // feed through hidden layer
    NeuralNetwork::HiddenLayer hidden_zs = {};
    NeuralNetwork::HiddenLayer hidden_activations = {};
    for (i32 row = 0; row < NeuralNetwork::HIDDEN_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::INPUT_LAYER_SIZE; ++column) {
            hidden_zs[row] += neural_network.input_to_hidden_weights[row][column] * input[column];
        }

        hidden_zs[row] += neural_network.hidden_biases[row];
        hidden_activations[row] = sigmoid(hidden_zs[row]);
    }

    // feed through output layer
    NeuralNetwork::OutputLayer output_zs = {};
    NeuralNetwork::OutputLayer output_activations = {};
    for (i32 row = 0; row < NeuralNetwork::OUTPUT_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::HIDDEN_LAYER_SIZE; ++column) {
            output_zs[row] += neural_network.hidden_to_output_weights[row][column] * input[column];
        }

        output_zs[row] += neural_network.output_biases[row];
        output_activations[row] = sigmoid(output_zs[row]);
    }

    // back propagate from output to hidden
    NeuralNetwork::OutputLayer hidden_to_output_gradient = {};
    for (i32 i = 0; i < NeuralNetwork::OUTPUT_LAYER_SIZE; ++i) {
        const f32 hidden_to_output_error = cost_derivative(output_activations[i], target[i]);
        hidden_to_output_gradient[i] = hidden_to_output_error * sigmoid_derivative(output_zs[i]);
    }

    // collect delta values for hidden to output weights and biases
    for (i32 row = 0; row < NeuralNetwork::OUTPUT_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::HIDDEN_LAYER_SIZE; ++column) {
            neural_network_delta.hidden_to_output_weights[row][column] += hidden_to_output_gradient[row] * hidden_activations[column];
        }
    }

    for (i32 i = 0; i < NeuralNetwork::OUTPUT_LAYER_SIZE; ++i) {
        neural_network_delta.output_biases[i] += hidden_to_output_gradient[i];
    }

    // back propagate from hidden to input
    NeuralNetwork::HiddenLayer input_to_hidden_gradient = {};
    for (i32 row = 0; row < NeuralNetwork::OUTPUT_LAYER_SIZE; ++row) {
        for  (i32 column = 0; column < NeuralNetwork::HIDDEN_LAYER_SIZE; ++column) {
            input_to_hidden_gradient[column] += neural_network.hidden_to_output_weights[row][column] * hidden_to_output_gradient[row];
        }
    }

    for (i32 i = 0; i < NeuralNetwork::HIDDEN_LAYER_SIZE; ++i) {
        input_to_hidden_gradient[i] *= sigmoid_derivative(hidden_zs[i]);
    }

    for (i32 row = 0; row < NeuralNetwork::HIDDEN_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::INPUT_LAYER_SIZE; ++column) {
            neural_network_delta.input_to_hidden_weights[row][column] += input_to_hidden_gradient[row] * input[column];
        }
    }

    for (i32 i = 0; i < NeuralNetwork::HIDDEN_LAYER_SIZE; ++i) {
        neural_network_delta.hidden_biases[i] += input_to_hidden_gradient[i];
    }
}
