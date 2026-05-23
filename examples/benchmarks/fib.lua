function fib(n)
    local a, b = 0, 1
    for i = 1, n do
        a, b = b, a + b
    end
    return a
end

local start = os.clock()
local result = fib(50)
local end_time = os.clock()

print("Fibonacci(50) = " .. result)
print("Time taken: " .. (end_time - start) .. " seconds")
