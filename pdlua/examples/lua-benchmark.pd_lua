local Benchmark = pd.Class:new():register("lua-benchmark")

function Benchmark:initialize(name, atoms)
  self.inlets = 1;
  pd.post("Hello, world!")
  return true
end

local clock = os.clock

local function benchmark(name, fn)
    local start = clock()
    fn()
    local elapsed = clock() - start
    pd.post(string.format("%-20s %.6f sec", name, elapsed))
end

function Benchmark:in_1_bang()
    local N = 1e7  -- adjust if too slow/fast on your machine

    pd.post("benchmarking pdlua:")
    pd.post("-----------------------------------")

    -- 1. Empty loop
    benchmark("empty loop", function()
        for i = 1, N do end
    end)

    -- 2. Arithmetic
    benchmark("arithmetic", function()
        local x = 0
        for i = 1, N do
            x = x + i * 2 - 3
        end
    end)

    -- 3. Function calls
    local function f(x)
        return x + 1
    end

    benchmark("function calls", function()
        local x = 0
        for i = 1, N do
            x = f(x)
        end
    end)

    -- 4. Table access
    benchmark("table access", function()
        local t = {}
        for i = 1, 100 do
            t[i] = i
        end

        local sum = 0
        for i = 1, N do
            sum = sum + t[(i % 100) + 1]
        end
    end)

    -- 5. Math library
    benchmark("math.sin", function()
        local x = 0
        for i = 1, N do
            x = x + math.sin(i)
        end
    end)
    pd.post("-----------------------------------")
end
