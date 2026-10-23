# Verification

Executed on 2026-10-22 on macOS arm64 with Apple clang 17.0.0 and Python 3.14.0. Run commands from this project directory.

```sh
make test
make clean
make test CXXFLAGS="-std=c++17 -O1 -g -Wall -Wextra -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all"
```

PASS, exit 0, with no compiler warnings:

- supplied puzzle matches a hand-checked complete 9×9 solution;
- complete invalid board is rejected;
- a locally valid but unsatisfiable puzzle returns false and preserves clues;
- missing file, too few/many entries, nonnumeric entries, out-of-range values and conflicting clues are rejected;
- move boundary checks reject invalid coordinates/values, including minimum-integer CLI input before coordinate subtraction;
- CLI exits on EOF and recovers from nonnumeric row/column/number input;
- autosolve and quit finish within a five-second subprocess timeout.

Before fixes, independent regressions failed because the original solver accepted an all-ones complete board and the loader returned normally for a missing file. CLI EOF and nonnumeric input exceeded a subprocess timeout. The test suite now exercises those paths. An additional UBSan regression caught signed overflow when converting the minimum integer to a zero-based coordinate; coordinates are now checked before subtraction.

Limits: solution uniqueness is not checked. The solver reports the first solution. No external puzzle corpus, performance benchmark, or exhaustive proof was run. This is an educational application, not a general Sudoku library.
