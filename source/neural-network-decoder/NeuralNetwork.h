/*
@Filename: NeuralNetwork.h
@Author: Ivan Ling (ivan.ling@soton.ac.uk)
@Version: 1.2

@Date: 27/10/2025
@copyright Copyright (c) 2025
*/


#ifndef ANN_H
#define ANN_H

#include <vector>
#include "Matrix.h"

using namespace std;

enum ActivationType {
    SIGMOID,
    RELU,
    TANH
};

//Utility functions for Neural Network
namespace nn_utils {
	Matrix sigmoid(const Matrix& m); //Sigmoid function
	Matrix dsigmoid(const Matrix& m); //Derivative of the sigmoid for backprop
	Matrix relu(const Matrix& m);
	Matrix drelu(const Matrix& m);
	Matrix tanh(const Matrix& m);
	Matrix dtanh(const Matrix& m); 
	int argmax(const Matrix& m); // Returns the index with the highest probability in output layer
}


class Layer {
public:
	Matrix weights;
	Matrix bias;
	ActivationType activation; //////////////////////////////////////////////////
	Layer(int input_size, int output_size, ActivationType act_type); ////////////
	Layer();
};

class ANN {
private:
	vector<int> layerSizes; //A vector describing the size of your neural network layers
	vector<Layer> layers; //The layers in the network
	vector<Matrix> activations; //Output of each layers

public:
	ANN(vector<int> sizes,vector<ActivationType> activations);
	Matrix feedforward(Matrix input);
	double update(Matrix target, double training_rate);
};


#endif 
