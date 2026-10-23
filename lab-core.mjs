// Browser companions to the prepared C++ demos; no network or hardware access.
export function solveSudoku(board) {
  if (!Array.isArray(board) || board.length !== 81 ||
      !Array.from(board).every(n => Number.isInteger(n) && n >= 0 && n <= 9)) return null;
  const cells = [...board];
  const valid = (index, value) => {
    const row = Math.floor(index / 9), col = index % 9;
    for (let j = 0; j < 81; j++) {
      if (j === index || cells[j] !== value) continue;
      const r = Math.floor(j / 9), c = j % 9;
      if (r === row || c === col || (Math.floor(r / 3) === Math.floor(row / 3) && Math.floor(c / 3) === Math.floor(col / 3))) return false;
    }
    return true;
  };
  if (cells.some((value, index) => value !== 0 && !valid(index, value))) return null;
  function solve() {
    let best = -1, options = [];
    for (let index = 0; index < 81; index++) {
      if (cells[index] !== 0) continue;
      const candidates = [];
      for (let n = 1; n <= 9; n++) if (valid(index, n)) candidates.push(n);
      if (!candidates.length) return false;
      if (best < 0 || candidates.length < options.length) { best = index; options = candidates; }
    }
    if (best < 0) return true;
    for (const value of options) {
      cells[best] = value;
      if (solve()) return true;
    }
    cells[best] = 0;
    return false;
  }
  return solve() ? cells : null;
}

export function inferDecoder(input, model) {
  if (!Number.isInteger(input) || input < 0 || input > 15) throw new RangeError('Input must be an integer from 0 to 15');
  if (!Array.isArray(model?.layers) || model.layers.length !== 2) throw new TypeError('Expected a 4–8–16 sigmoid model');
  const activations = [[(input >> 3) & 1, (input >> 2) & 1, (input >> 1) & 1, input & 1]];
  for (let l = 0; l < 2; l++) {
    const { weights, bias } = model.layers[l] ?? {};
    const previous = activations[l], size = l === 0 ? 8 : 16;
    if (!Array.isArray(weights) || weights.length !== size || !Array.isArray(bias) || bias.length !== size ||
        !Array.from(bias).every(Number.isFinite) || !Array.from(weights).every(row => Array.isArray(row) && row.length === previous.length && Array.from(row).every(Number.isFinite))) {
      throw new TypeError('Invalid decoder weights or biases');
    }
    activations.push(weights.map((row, r) => {
      let sum = 0;
      for (let c = 0; c < row.length; c++) sum += row[c] * previous[c];
      return 1 / (1 + Math.exp(-(sum + bias[r])));
    }));
  }
  const output = activations[2];
  return { activations, prediction: output.indexOf(Math.max(...output)) };
}

const validTime = value => Number.isSafeInteger(value) && value >= 0;
export class RoverCore {
  constructor({ acceleration = 0.75, deceleration = 1.5, watchdogMs = 100 } = {}) {
    if (![acceleration, deceleration].every(v => Number.isFinite(v) && v > 0) || !Number.isSafeInteger(watchdogMs) || watchdogMs < 40) throw new RangeError('Invalid ramp rates or watchdog');
    this.acceleration = acceleration;
    this.deceleration = deceleration;
    this.watchdogMs = watchdogMs;
    this.left = this.right = 0;
    this.lastCommandMs = null;
    this.linear = this.angular = 0;
  }
  accept(linear, angular, nowMs) {
    if (![linear, angular].every(Number.isFinite) || !validTime(nowMs)) throw new RangeError('Command and time must be finite; time must be a nonnegative safe integer');
    this.linear = linear;
    this.angular = angular;
    this.lastCommandMs = nowMs;
  }
  tick(nowMs, dtSeconds) {
    if (!validTime(nowMs) || !Number.isFinite(dtSeconds) || dtSeconds <= 0 || (this.lastCommandMs !== null && nowMs < this.lastCommandMs)) throw new RangeError('Invalid control clock or elapsed time');
    const ageMs = this.lastCommandMs === null ? 4294967295 : Math.min(nowMs - this.lastCommandMs, 4294967295);
    if (this.lastCommandMs === null || nowMs - this.lastCommandMs >= this.watchdogMs) return { ...this.emergencyStop(), ageMs, stopped: true };
    // Scale first so finite extreme inputs cannot overflow the wheel sums.
    const scale = Math.max(1, Math.abs(this.linear), Math.abs(this.angular));
    const left = this.linear / scale - this.angular / scale, right = this.linear / scale + this.angular / scale;
    const divisor = Math.max(1 / scale, Math.abs(left), Math.abs(right));
    const ramp = (current, target) => {
      const reversing = current * target < 0;
      const rate = !reversing && Math.abs(target) > Math.abs(current) ? this.acceleration : this.deceleration;
      if (reversing) target = 0;
      const delta = rate * dtSeconds;
      return current < target ? Math.min(current + delta, target) : Math.max(current - delta, target);
    };
    this.left = ramp(this.left, left / divisor);
    this.right = ramp(this.right, right / divisor);
    return { left: this.left, right: this.right, ageMs, stopped: false };
  }
  emergencyStop() {
    this.left = this.right = 0;
    return { left: 0, right: 0 };
  }
}
