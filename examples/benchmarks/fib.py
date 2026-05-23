import time

def fib(n):
    a, b = 0, 1
    for _ in range(n):
        a, b = b, a + b
    return a

start = time.time()
result = fib(50)
end = time.time()

print(f"Fibonacci(50) = {result}")
print(f"Time taken: {end - start:.6f} seconds")
