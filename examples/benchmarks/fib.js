const { performance } = require('perf_hooks');

function fib(n) {
    let a = 0n, b = 1n;
    for (let i = 0; i < n; i++) {
        [a, b] = [b, a + b];
    }
    return a;
}

const start = performance.now();
const result = fib(50);
const end = performance.now();

console.log(`Fibonacci(50) = ${result}`);
console.log(`Time taken: ${(end - start) / 1000} seconds`);
