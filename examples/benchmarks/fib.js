const { performance } = require('perf_hooks');

function fib(n) {
    if (n < 2) return n;
    return fib(n-1) + fib(n-2);
}

const start = performance.now();
const result = fib(32);
const end = performance.now();

console.log(`Fibonacci(32) = ${result}`);
console.log(`Time taken: ${(end - start) / 1000} seconds`);
