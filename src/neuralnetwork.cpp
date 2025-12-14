#include "neuralnetwork.h"

NeuralNetwork::NeuralNetwork() {
    // Seed random number generator
    srand(time(0));
}

void NeuralNetwork::setup(int inputSize, int hiddenLayers, int neuronsPerLayer, int outputSize, ActivationType actType, TaskMode taskMode) {

    layers.clear();
    activation = actType;
    mode = taskMode;

    int prevSize = inputSize;

    // --- 1. HIDDEN LAYERS ---
    for(int i = 0; i < hiddenLayers; i++) {
        Layer l;
        l.numNeurons = neuronsPerLayer;
        l.numWeightsPerNeuron = prevSize;

        l.outputs.resize(neuronsPerLayer);
        l.deltas.resize(neuronsPerLayer);
        l.biases.resize(neuronsPerLayer);
        l.weights.resize(neuronsPerLayer * prevSize);

        // Random initialization for weights and biases
        for(int n = 0; n < neuronsPerLayer; n++) {
            l.biases[n] = randomWeight();
            for(int w = 0; w < prevSize; w++) {
                l.weights[n * prevSize + w] = randomWeight();
            }
        }
        layers.push_back(l);
        prevSize = neuronsPerLayer;
    }

    // --- 2. OUTPUT LAYER ---
    Layer outL;
    outL.numNeurons = outputSize;
    outL.numWeightsPerNeuron = prevSize;

    outL.outputs.resize(outputSize);
    outL.deltas.resize(outputSize);
    outL.biases.resize(outputSize);
    outL.weights.resize(outputSize * prevSize);

    for(int n = 0; n < outputSize; n++) {
        outL.biases[n] = randomWeight();
        for(int w = 0; w < prevSize; w++) {
            outL.weights[n * prevSize + w] = randomWeight();
        }
    }
    layers.push_back(outL);
}

void NeuralNetwork::reset() {
    // Re-randomize all weights and biases without changing architecture
    for(auto &layer : layers) {
        for(int n = 0; n < layer.numNeurons; n++) {
            layer.biases[n] = randomWeight();
        }
        for(size_t j = 0; j < layer.weights.size(); j++) {
            layer.weights[j] = randomWeight();
        }
    }
}

double NeuralNetwork::randomWeight() {
    // Returns a random value between -1.0 and 1.0
    return ((double)rand() / RAND_MAX) * 2.0 - 1.0;
}

double NeuralNetwork::activate(double x) {
    if (activation == ActivationType::TANH) return tanh(x);
    if (activation == ActivationType::LINEAR) return x;
    // Sigmoid: 1 / (1 + e^-x)
    return 1.0 / (1.0 + exp(-x));
}

double NeuralNetwork::activateDeriv(double y) {
    // y = activation output
    if (activation == ActivationType::TANH) return 1.0 - y * y;
    if (activation == ActivationType::LINEAR) return 1.0;
    // Sigmoid derivative: y * (1 - y)
    return y * (1.0 - y);
}

// --- PREDICT (Feed Forward) ---
std::vector<double> NeuralNetwork::predict(const std::vector<double> &inputs) {
    std::vector<double> currentInputs = inputs;

    for(size_t i = 0; i < layers.size(); i++) {
        Layer &layer = layers[i];
        bool isOutputLayer = (i == layers.size() - 1);

        for(int n = 0; n < layer.numNeurons; n++) {
            double sum = layer.biases[n];

            // Calculate weighted sum (Dot Product)
            for(size_t w = 0; w < currentInputs.size(); w++) {
                int weightIndex = n * layer.numWeightsPerNeuron + w;
                sum += currentInputs[w] * layer.weights[weightIndex];
            }

            // Apply activation function
            if (isOutputLayer && mode == TaskMode::REGRESSION) {
                layer.outputs[n] = sum; // Linear output for regression
            } else {
                layer.outputs[n] = activate(sum);
            }
        }
        currentInputs = layer.outputs;
    }
    return currentInputs;
}

// --- TRAIN (Backpropagation) ---
double NeuralNetwork::train(const std::vector<double> &inputs, const std::vector<double> &targets, double learningRate) {
    // 1. Forward Pass
    predict(inputs);

    Layer &outputLayer = layers.back();
    double totalError = 0.0;

    // 2. Calculate Output Layer Deltas
    for(int n = 0; n < outputLayer.numNeurons; n++) {
        double error = targets[n] - outputLayer.outputs[n];
        totalError += 0.5 * (error * error); // MSE = 0.5 * (target - output)^2

        double derivative;
        if (mode == TaskMode::REGRESSION) {
            derivative = 1.0;
        } else {
            derivative = activateDeriv(outputLayer.outputs[n]);
        }
        // Delta = Error * Derivative
        outputLayer.deltas[n] = error * derivative;
    }

    // 3. Calculate Hidden Layer Deltas (Backpropagate Error)
    // Loop from the second to last layer down to the first
    for(int i = (int)layers.size() - 2; i >= 0; i--) {
        Layer &curr = layers[i];
        Layer &next = layers[i+1];

        for(int n = 0; n < curr.numNeurons; n++) {
            double propagatedError = 0.0;

            // Sum errors from the next layer
            for(int nextN = 0; nextN < next.numNeurons; nextN++) {
                int weightIndex = nextN * next.numWeightsPerNeuron + n;
                propagatedError += next.deltas[nextN] * next.weights[weightIndex];
            }

            // Calculate delta for current neuron
            curr.deltas[n] = propagatedError * activateDeriv(curr.outputs[n]);
        }
    }

    // 4. Update Weights and Biases
    for(int i = (int)layers.size() - 1; i >= 0; i--) {
        Layer &layer = layers[i];
        // If i > 0 use previous layer outputs, if i == 0 use original inputs
        const std::vector<double> &currentLayerInputs = (i == 0) ? inputs : layers[i-1].outputs;

        for(int n = 0; n < layer.numNeurons; n++) {
            for(size_t w = 0; w < currentLayerInputs.size(); w++) {
                int weightIndex = n * layer.numWeightsPerNeuron + w;
                // Weight Update Rule: W_new = W_old + (LearningRate * Delta * Input)
                layer.weights[weightIndex] += learningRate * layer.deltas[n] * currentLayerInputs[w];
            }
            // Bias Update
            layer.biases[n] += learningRate * layer.deltas[n];
        }
    }

    return totalError;
}

// --- Getters ---
double NeuralNetwork::getWeight(int layerIdx, int neuronIdx, int weightIdx) const {
    // Bounds check with safe casting
    if (layerIdx < 0 || (size_t)layerIdx >= layers.size()) return 0.0;

    const Layer& l = layers[layerIdx];
    if (neuronIdx < 0 || (size_t)neuronIdx >= (size_t)l.numNeurons) return 0.0;
    if (weightIdx < 0 || (size_t)weightIdx >= (size_t)l.numWeightsPerNeuron) return 0.0;

    return l.weights[neuronIdx * l.numWeightsPerNeuron + weightIdx];
}

double NeuralNetwork::getBias(int layerIdx, int neuronIdx) const {
    // Bounds check
    if (layerIdx < 0 || (size_t)layerIdx >= layers.size()) return 0.0;

    const Layer& l = layers[layerIdx];
    if (neuronIdx < 0 || (size_t)neuronIdx >= (size_t)l.numNeurons) return 0.0;

    return l.biases[neuronIdx];
}
