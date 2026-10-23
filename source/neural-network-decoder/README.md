# Four-bit neural-network decoder

A small C++ matrix and feedforward neural-network exercise. The demo trains a 4–8–16 network to map each four-bit input to one of sixteen one-hot output classes, using sigmoid activations and backpropagation without a machine-learning framework.

## Build, train and test

Requires Make and a GCC/Clang C++17 compiler. There are no third-party runtime dependencies.

```sh
make
./build/decoder
make test
make clean
```

The demo uses seed 7, 10,000 epochs and learning rate 0.1. It reports predictions for all sixteen inputs and exits nonzero unless all are classified correctly. `make test` stores the training transcript in ignored `build/training.log`.

The update rule optimises half the sum of squared output errors. The progress metric printed as `Avg Loss` is the average per-example sum of absolute errors; it is not the squared-error optimisation objective. Activation derivative helpers take the already activated output. ReLU uses derivative zero at zero.

## Provenance and changes

`Matrix.h/.cpp` and `NeuralNetwork.h/.cpp` carry Ivan Ling's University of Southampton teaching-material headers. Those headers and copyright notices are retained. The recovered decoder application and coursework modifications are associated with Emre Karausta. This example is not listed in the master CV.

Preparation completed the four unfinished ReLU/tanh activation and derivative functions, corrected hidden-layer error propagation to use the forward pass weights, and made the example reproducible and checked across all sixteen inputs. Added numerical gradient and activation tests. Original files were not changed.

No blanket open-source licence is granted for instructor material. Publication/redistribution rights must be confirmed separately.

See [VERIFICATION.md](VERIFICATION.md). Fixed model dimensions are intentional: callers must provide matching matrix, target and layer shapes; this educational API does not validate every malformed shape.
