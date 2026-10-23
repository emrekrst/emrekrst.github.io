import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { solveSudoku, inferDecoder, RoverCore } from '../lab-core.mjs';

const puzzle = '530070000600195000098000060800060003400803001700020006060000280000419005000080079'.split('').map(Number);
const solution = '534678912672195348198342567859761423426853791713924856961537284287419635345286179'.split('').map(Number);
test('Sudoku solves the source puzzle without changing input', () => {
  const before = [...puzzle];
  assert.deepEqual(solveSudoku(puzzle), solution);
  assert.deepEqual(puzzle, before);
  assert.notEqual(solveSudoku(solution), solution);
});
test('Sudoku rejects malformed, conflicting and unsatisfiable boards', () => {
  for (const board of [null, [], Array(81), Array(81).fill(10), Array(81).fill(1), Array(81).fill(NaN)]) assert.equal(solveSudoku(board), null);
  const impossible = [...puzzle]; impossible[2] = 1;
  const before = [...impossible];
  assert.equal(solveSudoku(impossible), null);
  assert.deepEqual(impossible, before);
});
test('rover normalizes mixed commands and stops at watchdog boundary', () => {
  const rover = new RoverCore();
  assert.deepEqual(rover.tick(0, 0.02), { left: 0, right: 0, ageMs: 4294967295, stopped: true });
  rover.accept(1, 1, 0);
  assert.deepEqual(rover.tick(99, 2), { left: 0, right: 1, ageMs: 99, stopped: false });
  assert.deepEqual(rover.tick(100, 0.02), { left: 0, right: 0, ageMs: 100, stopped: true });
});
test('rover reverses via zero and emergency stop resets applied motion', () => {
  const rover = new RoverCore();
  rover.accept(1, 0, 0);
  assert.equal(rover.tick(0, 1).left, 0.75);
  rover.accept(-1, 0, 10);
  assert.equal(rover.tick(10, 1).left, 0);
  assert.equal(rover.tick(11, 1).left, -0.75);
  assert.deepEqual(rover.emergencyStop(), { left: 0, right: 0 });
  assert.equal(rover.tick(12, 0.02).left, -0.015);
});
test('rover preserves source turn signs, proportional mixing and deceleration', () => {
  const rover = new RoverCore();
  rover.accept(0, 1, 0);
  assert.deepEqual(rover.tick(0, 2), { left: -1, right: 1, ageMs: 0, stopped: false });
  rover.accept(0, 0, 1);
  assert.deepEqual(rover.tick(1, 0.2), { left: -0.7, right: 0.7, ageMs: 0, stopped: false });
  rover.emergencyStop();
  rover.accept(1, 0.5, 2);
  const mixed = rover.tick(2, 2);
  assert.ok(Math.abs(mixed.left - 1 / 3) < 1e-15);
  assert.equal(mixed.right, 1);
  rover.accept(Number.MAX_VALUE, Number.MAX_VALUE, 3);
  const extreme = rover.tick(3, 2);
  assert.equal(extreme.left, 0);
  assert.equal(extreme.right, 1);
});
test('rover rejects nonfinite commands, invalid clocks and invalid config', () => {
  for (const config of [{ acceleration: 0 }, { deceleration: NaN }, { watchdogMs: 39 }, { watchdogMs: Infinity }]) assert.throws(() => new RoverCore(config));
  const rover = new RoverCore();
  for (const args of [[NaN, 0, 0], [0, Infinity, 0], [0, 0, Infinity], [0, 0, -1]]) assert.throws(() => rover.accept(...args));
  rover.accept(0, 1, 20);
  for (const args of [[19, 0.01], [NaN, 1], [Infinity, 1], [20, 0], [20, NaN]]) assert.throws(() => rover.tick(...args));
});
test('decoder predicts all 16 values and matches original C++ activations', () => {
  const model = JSON.parse(readFileSync(new URL('../assets/decoder-weights.json', import.meta.url)));
  const before = JSON.stringify(model);
  assert.equal(model.origin.seed, 7);
  assert.equal(model.origin.epochs, 10000);
  for (let input = 0; input < 16; input++) {
    const actual = inferDecoder(input, model);
    assert.equal(actual.prediction, input);
    assert.deepEqual(actual.activations.map(layer => layer.length), [4, 8, 16]);
    actual.activations.forEach((layer, l) => layer.forEach((v, j) => assert.ok(Math.abs(v - model.reference[input].activations[l][j]) < 1e-12)));
  }
  for (const input of [-1, 16, 1.5, NaN, '3']) assert.throws(() => inferDecoder(input, model));
  assert.throws(() => inferDecoder(0, {}));
  assert.equal(JSON.stringify(model), before);
  for (const corrupt of [
    { layers: [{ weights: [], bias: [] }, model.layers[1]] },
    { layers: [{ ...model.layers[0], bias: Array(8).fill(NaN) }, model.layers[1]] },
  ]) assert.throws(() => inferDecoder(0, corrupt));
});
