function fib(n)
    if n < 2 then return n end
    return fib(n-1) + fib(n-2)
end

local start = os.clock()
local result = fib(32)
local end_time = os.clock()

print("Fibonacci(32) = " .. result)
print("Time taken: " .. (end_time - start) .. " seconds")
