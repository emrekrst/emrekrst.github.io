# Verification

Executed on 2026-10-22 on macOS arm64 using Apple clang 17.0.0.

```sh
make test
```

PASS, exit 0, no compiler warnings:

- ReLU/tanh values and output-based derivatives at negative, zero and positive inputs;
- central finite differences for every weight and bias of an actual 2–3–2 sigmoid ANN, comparing the half-squared-error derivative with the parameter change produced by `ANN::update`;
- maximum absolute numerical/analytical gradient difference: **5.19785e-12**, tolerance 1e-7;
- deterministic training demo: **Accuracy: 16/16** after 10,000 epochs.

The same actual-model gradient regression failed before the backpropagation fix: maximum absolute difference **0.00043418**. The original activation TODOs produced four missing-return compiler warnings. Production classes gained no test-only hooks: the test executable uses GCC/Clang's `-fno-access-control` solely to inspect model parameters for finite differences.

Limits: all sixteen test examples are also the training set; 16/16 demonstrates memorisation of this finite decoder task, not generalisation. Seed 7 is repeatable with the same C library; `rand()` sequences are not standardised across platforms. The end-to-end gradient check uses sigmoid; ReLU/tanh helpers have direct value/derivative checks. No benchmark, large dataset, GPU training, or malformed-shape fuzzing was run.
