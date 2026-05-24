import time

def fib(n):
    if n < 2: return n
    return fib(n-1) + fib(n-2)

start = time.time()
result = fib(32)
end = time.time()

print(f"Fibonacci(32) = {result}")
print(f"Time taken: {end - start:.6f} seconds")
