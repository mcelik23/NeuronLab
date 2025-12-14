#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Enums for Network Configuration
enum class ActivationType { SIGMOID, TANH, LINEAR };
enum class TaskMode { CLASSIFICATION, REGRESSION };

struct Layer {
    int numNeurons;
    int numWeightsPerNeuron;

    std::vector<double> outputs;
    std::vector<double> deltas;
    std::vector<double> weights;
    std::vector<double> biases;
};

class NeuralNetwork {
public:
    NeuralNetwork();

    // Initialization
    void setup(int inputSize, int hiddenLayers, int neuronsPerLayer, int outputSize, ActivationType actType, TaskMode mode);
    void reset();

    // Core Operations
    std::vector<double> predict(const std::vector<double> &inputs);
    double train(const std::vector<double> &inputs, const std::vector<double> &targets, double learningRate);

    // Getters & Accessors
    double getWeight(int layerIdx, int neuronIdx, int weightIdx) const;
    double getBias(int layerIdx, int neuronIdx) const;
    int getLayerCount() const { return (int)layers.size(); }
    int getLayerSize(int i) const { return layers[i].numNeurons; }

private:
    std::vector<Layer> layers;
    ActivationType activation;
    TaskMode mode;

    // Internal Helpers
    double activate(double x);
    double activateDeriv(double y);
    double randomWeight();
};

#endif // NEURALNETWORK_H
