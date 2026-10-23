/*
@Filename: NeuralNetwork.cpp
@Author: Ivan Ling (ivan.ling@soton.ac.uk)
@Version: 1.2

@Date: 27/10/2025
@copyright Copyright (c) 2025
*/


#include "NeuralNetwork.h"
#include <cmath>      

using namespace std;


//A layer refers to the neurons in a single layer
//Each will have their own wieghts and biases

Layer::Layer(int input_size, int output_size, ActivationType act_type) ////////////////
{
	weights = Matrix(output_size, input_size, true);
	bias = Matrix(output_size, 1, true);
	activation = act_type;
}

Layer::Layer() {}


ANN::ANN(vector<int> sizes, vector<ActivationType> activations)
{
	this->layerSizes = sizes;
	for (size_t i = 0; i + 1 < sizes.size(); ++i) {
		layers.push_back(Layer(sizes[i], sizes[i+1], activations[i]));/////////////////
	}
}

Matrix ANN::feedforward(Matrix input)
{
	activations.clear();
	activations.push_back(input); //First activation layer is always the input
	Matrix current_activation = input;

	for (size_t i=0; i<layers.size(); ++i){
		Layer& current_layer = layers[i];
		Matrix z = (current_layer.weights * current_activation) + current_layer.bias;

		switch (current_layer.activation)
        {
            case SIGMOID:
                current_activation = nn_utils::sigmoid(z);
                break;
            case RELU:
                current_activation = nn_utils::relu(z); 
                break;
            case TANH:
                current_activation = nn_utils::tanh(z);
                break;
			// can add more cases, FLEXIBILITY+++
        }
		activations.push_back(current_activation);
	}

	return current_activation;
}

double ANN::update(Matrix target, double training_rate)
{
	Matrix output_activation = activations.back();
	Matrix error_matrix = target - output_activation;

	double absError = 0.0;
	for(int i=0; i<error_matrix.getRows(); ++i) {
		absError += abs(error_matrix.data[i][0]);
	}



	for (int i = layers.size() - 1; i>=0; --i){
		Matrix current_output = activations[i+1];
		Matrix derivative;

		switch(layers[i].activation){
			case SIGMOID:
				derivative = nn_utils::dsigmoid(current_output);
				break;
			case RELU:
				derivative = nn_utils::drelu(current_output);
				break;
			case TANH:
				derivative = nn_utils::dtanh(current_output);
				break;
		}

		Matrix gradient = error_matrix.element_multiply(derivative);
		Matrix scaled_gradient = gradient * training_rate;

		Matrix prev_output = activations[i];
		Matrix prev_output_transposed = prev_output.transpose();
		Matrix delta_weights = scaled_gradient * prev_output_transposed;
		
        // Propagate through the weights used by this forward pass, before updating them.
        Matrix previous_error;
        if (i > 0) previous_error = layers[i].weights.transpose() * gradient;
		layers[i].weights = layers[i].weights + delta_weights;
		layers[i].bias = layers[i].bias + scaled_gradient;
		if(i>0){
            error_matrix = previous_error; 
		}
	}

	return absError;
}


// Utility functions for ANN
Matrix nn_utils::sigmoid(const Matrix& m)
{
	vector<vector<double>> result_data(m.getRows(), vector<double>(m.getCol()));
	for (int i = 0; i < m.getRows(); ++i)
	{
		for (int j = 0; j < m.getCol(); ++j)
		{
			result_data[i][j] = 1.0 / (1.0 + exp(-m.data[i][j]));
		}
	}
	return Matrix(result_data);
}

Matrix nn_utils::relu(const Matrix& m)
{
    Matrix result(m.getRows(), m.getCol(), false);
    for (int i = 0; i < m.getRows(); ++i)
        for (int j = 0; j < m.getCol(); ++j)
            result.data[i][j] = m.data[i][j] > 0 ? m.data[i][j] : 0.0;
    return result;
}

Matrix nn_utils::tanh(const Matrix& m)
{
    Matrix result(m.getRows(), m.getCol(), false);
    for (int i = 0; i < m.getRows(); ++i)
        for (int j = 0; j < m.getCol(); ++j)
            result.data[i][j] = std::tanh(m.data[i][j]);
    return result;
}

Matrix nn_utils::dsigmoid(const Matrix& m)
{
	vector<vector<double>> result_data(m.getRows(), vector<double>(m.getCol()));
	for (int i = 0; i < m.getRows(); ++i)
	{
		for (int j = 0; j < m.getCol(); ++j)
		{
			result_data[i][j] = m.data[i][j] * (1.0 - m.data[i][j]);
		}
	}
	return Matrix(result_data);
}

Matrix nn_utils::drelu(const Matrix& m)
{
    Matrix result(m.getRows(), m.getCol(), false);
    for (int i = 0; i < m.getRows(); ++i)
        for (int j = 0; j < m.getCol(); ++j)
            result.data[i][j] = m.data[i][j] > 0 ? 1.0 : 0.0;
    return result;
}

Matrix nn_utils::dtanh(const Matrix& m)
{
    Matrix result(m.getRows(), m.getCol(), false);
    for (int i = 0; i < m.getRows(); ++i)
        for (int j = 0; j < m.getCol(); ++j)
            result.data[i][j] = 1.0 - m.data[i][j] * m.data[i][j];
    return result;
}

int nn_utils::argmax(const Matrix& m)
{
	if (m.getCol() != 1)
	{
		cout<< "argmax only supports 1-column matrices!";
	}
	double max_val = -99999;
	int max_idx = 0;
	for (int i = 0; i < m.getRows(); i++)
	{
		if (m.data[i][0] > max_val)
		{
			max_val = m.data[i][0];
			max_idx = i;
		}
	}
	return max_idx;
}
