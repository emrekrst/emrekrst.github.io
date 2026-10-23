
#include <iostream>
#include <vector>
#include <cstdlib>

#include <iomanip>
#include "NeuralNetwork.h" 

using namespace std;


Matrix create_input_matrix(int n)
{
    vector<vector<double>> d(4, vector<double>(1, 0.0));

    d[0][0] = (n >> 3) & 1;
    d[1][0] = (n >> 2) & 1;
    d[2][0] = (n >> 1) & 1;
    d[3][0] = (n >> 0) & 1;
    return Matrix(d);
}

Matrix create_target_matrix(int n)
{
    vector<vector<double>> d(16, vector<double>(1, 0.0));
    if (n >= 0 && n < 16) {
        d[n][0] = 1.0;
    }
    return Matrix(d);
}

int main()
{
    srand(7); // Reproducible training demo.
    cout << "--- 4-Bit Binary Decoder Training ---" << endl << endl;
    vector<int> network_sizes = {4, 8, 16};

    vector<ActivationType> activations = {SIGMOID, SIGMOID};

    ANN decoder(network_sizes, activations);
   
    double training_rate = 0.1;
    int epochs = 10000; 
    cout << "Network: 4-8-16 | Epochs: " << epochs << " | Rate: " << training_rate << endl;

    vector<Matrix> inputs;
    vector<Matrix> targets;

    for (int i = 0; i < 16; i++)
    {
        inputs.push_back(create_input_matrix(i));
        targets.push_back(create_target_matrix(i));
    }
    cout << "Generated 16 input/target pairs." << endl << endl;

    cout << "Starting training..." << endl;
    cout << std::fixed << std::setprecision(8);

    for (int i = 0; i < epochs; i++)
    {
        double total_epoch_loss = 0.0;
        for (int j = 0; j < 16; j++)
        {
            decoder.feedforward(inputs[j]); 
            total_epoch_loss += decoder.update(targets[j], training_rate);
        }
        if ((i + 1) % 100 == 0)
        {
            cout << "Epoch: " << setw(5) << (i + 1) << " | Avg Loss: " << (total_epoch_loss / 16.0) << endl;
        }
    }

    cout << "\n--- Training Complete: Testing ---" << endl;
    cout << std::fixed << std::setprecision(2);

    vector<int> test_values;
    for (int i = 0; i < 16; ++i) test_values.push_back(i);
    int correct = 0;

    for (int val : test_values)
    {
        Matrix test_input = create_input_matrix(val);
        Matrix test_output = decoder.feedforward(test_input);

        cout << "-----------------------------------" << endl;
        cout << "Input: " << val << endl;
        cout << "Correct Output Index: " << val << endl;
        
        int predicted = nn_utils::argmax(test_output);
        correct += predicted == val;
        cout << "Network Output Index: " << predicted << endl;

        cout << "Output Vector: ";
        test_output.transpose().print_matrix();
        cout << endl;
    }
    cout << "Accuracy: " << correct << "/16" << endl;
    return correct == 16 ? 0 : 1;
}