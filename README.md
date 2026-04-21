# cache-bench

A benchmark suite that measures why one algorithm is faster than another using hardware performance counters.

Built as a teaching tool for students who want to understand the hardware behaviour behind their code.

---

## What you will see

```
╔══════════════════════════════╦════════════╦═════════════╦═════════════╦═════════════╦═══════╗
║ Benchmark                    ║ Time (ms)  ║ L1 Misses   ║ LLC Misses  ║ Branch Misp ║ IPC   ║
╠══════════════════════════════╬════════════╬═════════════╬═════════════╬═════════════╬═══════╣
║  ▸ vector_vs_list  (N=1,000,000)                                                            ║
╠══════════════════════════════╬════════════╬═════════════╬═════════════╬═════════════╬═══════╣
║ std::vector sum              ║        0.3 ║      12,400 ║         800 ║         200 ║   3.8 ║
║ std::list sum                ║        2.1 ║   1,002,000 ║     998,000 ║         210 ║   0.4 ║
...
```

Each column answers a different question about the hardware:

| Column | What it measures | High value means |
|---|---|---|
| **Time (ms)** | Wall-clock time per iteration | Slower |
| **L1 Misses** | Accesses that missed the L1 cache (closest, fastest) | Poor locality |
| **LLC Misses** | Accesses that missed every cache level and hit DRAM | Lots of DRAM traffic |
| **Branch Mispr** | Times the CPU predicted the wrong path | Wasted speculative work |
| **IPC** | Instructions retired per clock cycle | Higher = CPU is less stalled |

---

## Building

```bash
git clone https://github.com/nemeth06/cache-bench
cd cache-bench
cmake -B build && cmake --build build
./build/cache-bench
```

**Requirements:** Linux, CMake 3.20+, GCC 12+ or Clang 15+.

### Hardware counters

Full counter data requires either:

```bash
# Option 1 — run as root
sudo ./build/cache-bench

# Option 2 — lower the paranoia level (persists until reboot)
echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid
./build/cache-bench
```

Without this, the suite still runs and shows timing data with `--` in the counter columns.

---

## The benchmarks

### 1 — `std::vector` vs `std::list`

**What it does:** Sums one million integers stored either in a `std::vector` or a `std::list`.

**What to look at:** The LLC Miss and IPC columns.

A `std::vector` stores all elements back-to-back in one contiguous block of memory. When you read element 0, the CPU's hardware prefetcher recognizes the linear stride and fetches upcoming elements into the L1 cache ahead of time. Memory access latency is hidden, and the CPU can maintain a high Instruction Per Cycle (IPC) rate.

A `std::list` allocates each node separately. Even if nodes are allocated contiguously, following a linked list introduces a read-after-read data dependency known as pointer chasing. To process node $N+1$, the CPU pipeline must wait for the pointer address from node $N$ to resolve. This stalls the pipeline and destroys Instruction-Level Parallelism (ILP), causing low IPC. 

Furthermore, to simulate real-world memory fragmentation, this benchmark explicitly shuffles the list nodes across memory. Because the CPU cannot predict where the next node resides, the hardware prefetcher is useless. The working set size exceeds the L3 cache, meaning almost every element access results in a cache miss that must fetch from DRAM.

---

### 2 — Array of Structs vs Struct of Arrays

**What it does:** Sums only the `x` field of one million four-field particles (`x, y, z, w`).

**What to look at:** The L1 Miss and LLC Miss columns.

In the Array-of-Structs layout, each element in memory looks like: `[x][y][z][w][x][y][z][w]...`. To read all the `x` values, the CPU loads a cache line containing `x, y, z, w` together — but the loop only uses `x` and discards the rest. Three quarters of every cache line fetched is wasted bandwidth.

In the Struct-of-Arrays layout, all `x` values are stored together: `[x][x][x]...[x]` followed by `[y][y]...`. Reading only `x` loads cache lines that are 100% useful data. The LLC miss count drops dramatically.

If your loop only touches one field of a struct, consider whether your data layout is wasting memory bandwidth.

---

### 3 — Recursive vs iterative segment tree

**What it does:** Answers 100,000 random range-sum queries on an array of one million elements.

**What to look at:** The LLC Miss column, and the raw time difference.

Both implementations share the same $O(\log n)$ asymptotic complexity per query. The recursive version allocates each tree node individually on the heap. During a query, the traversal begins at the root. The top levels of the tree are accessed frequently and remain "hot" in the L1 or L2 cache. However, as the traversal descends into the wider, lower levels of the tree (spanning roughly 20 levels for $N=1,000,000$), the memory access pattern becomes sparse. Following pointers at these lower levels results in cache misses.

The iterative version stores the entire tree in a flat `std::vector`. The root is at index 1, and the children of node $k$ are always at $2k$ and $2k+1$. Nodes accessed during a query are spatially grouped, allowing adjacent accesses to share 64-byte cache lines. 

---

### 4 — Row-major vs column-major matrix traversal

**What it does:** Sums all elements of a 2048×2048 integer matrix, either row-by-row or column-by-column.

**What to look at:** The LLC Miss column and the time ratio.

C++ stores 2D arrays in row-major order. The element at `(r, c)` is at address `base + r * COLS + c`. 

Iterating `for r: for c:` (row-major) accesses elements sequentially. The hardware prefetcher loads upcoming cache lines before they are needed.

Iterating `for c: for r:` (column-major) steps through memory in strides of 16 KB. Each access lands in a completely different cache line. Because the matrix exceeds the Last Level Cache capacity, these cache lines are evicted before the outer loop advances to the next column. The prefetcher cannot assist, resulting in catastrophic LLC misses.

---

### 5 — Predecessor query: sorted vector vs `std::map`

**What it does:** Inserts $N$ integers one at a time into an ordered container, and after each insertion finds the largest element already present that is less than or equal the new value (a predecessor query). Runs at $N = 500$, $2000$, $10000$, and $50000$.

**What to look at:** Watch which implementation wins at each size, and at which $N$ the answer flips.

A sorted `std::vector` supports $O(\log n)$ lookup via binary search. But insertion requires shifting all elements to the right of the insertion point, which is $O(n)$. At small N this shift is cheap, and the cache-friendly binary search makes lookups fast.

A `std::map` is a red-black tree. Both insertion and lookup are $O(\log n)$, but every tree node is a separate heap allocation. Traversing the tree accesses 20 pointers scattered across memory, nearly all cache misses.

At small $N$ ($\leq 10000$ in practice), the vector wins: the $O(n)$ shift touches only a few kilobytes of contiguous memory, which is faster than accessing 20 random pointers. At large $N$ ($\geq 50000$), the $O(n)$ shift cost dominates and the map wins.

Using `std::map` because it's $O(\log n)$ is not always the right call. For small $N$ or when you know lookups vastly outnumber insertions a sorted vector is often faster.

---

## Adding your own benchmark

The suite provides a plugin interface so you can benchmark your own code without modifying the built-in files.

**Step 1.** Create a header file anywhere (e.g. `benchmarks/my_bench.hpp`):

```cpp
#pragma once
#include "plugin.hpp"
#include "utils.hpp"

struct MyBench {
    static constexpr std::string_view group_name = "my_benchmark_name";
    static constexpr std::string_view name_a     = "algorithm_a";
    static constexpr std::string_view name_b     = "algorithm_b";

    static void run_a() {
        // your first implementation
        int result = 0;
        // ...
        do_not_optimize(result);  // prevent compiler from removing dead code
    }

    static void run_b() {
        // your second implementation
        int result = 0;
        // ...
        do_not_optimize(result);
    }
};
```

**Step 2.** Add two lines to `src/main.cpp`:

```cpp
#include "benchmarks/my_bench.hpp"   // at the top
// ...
runner.add_plugin<MyBench>();         // inside main(), after the other add_plugin calls
```

**Step 3.** Rebuild:

```bash
cmake --build build
./build/cache-bench
```

Your benchmark pair appears as a new group in the table.

### Tips for meaningful benchmarks

- **Use `do_not_optimize()`** on any result your function computes. Without it, the compiler may detect that the output is unused and remove the entire computation.
- **Build your data structures outside `run_a`/`run_b`** using static or global variables initialised at startup. The benchmark harness already flushes the cache between iterations — you do not need to, and rebuilding the data structure inside the timed function will obscure the result.
- **Keep both implementations doing the same logical work** so the comparison is fair.
- **Use `warmup = 3, iterations = 20`** (the defaults) for most benchmarks. Increase iterations for very fast functions (< 0.1 ms) where clock noise is significant.

---

## Understanding the counter columns

### What is a cache?

Your CPU has three levels of cache between the registers and main memory (DRAM):

```
CPU core
  └─ L1 cache  (~32 KB, ~4 cycles)
       └─ L2 cache  (~256 KB, ~12 cycles)
            └─ L3 / LLC cache  (~8–32 MB, ~40 cycles)
                     └─ DRAM  (GBs, ~200 cycles)
```

When the CPU reads a memory address, it checks L1 first. If found (a *hit*), the data arrives in ~4 cycles. If not (*miss*), it checks L2, then LLC, then DRAM. A DRAM access costs ~50× more than an L1 hit.

Data is transferred in 64-byte *cache lines*. When you read one byte, the CPU loads the surrounding 63 bytes into cache automatically. This is why contiguous access patterns are fast: after the first miss, subsequent nearby accesses are already in cache.

### What is IPC?

A modern CPU can issue multiple instructions per clock cycle. When instructions are waiting for data from memory, they stall and IPC drops below 1. When data arrives from L1 cache and the CPU pipeline is full, IPC can reach 3–5. IPC is the clearest single indicator of how cache-friendly a workload is.

### Why does `-fno-tree-vectorize` matter?

The benchmark suite explicitly compiles with `-O2` and `-fno-tree-vectorize`. Modern compilers (like GCC 12+) enable auto-vectorization by default at `-O2`. Vectorization uses SIMD (Single Instruction, Multiple Data) instructions, which process multiple elements per clock cycle. While highly efficient, SIMD vastly alters memory access patterns and can obscure the fundamental cache mechanics this suite demonstrates. Disabling it ensures the generated code reflects the memory accesses written in C++.

---

### Adapting Data Sizes to Your Hardware

To accurately measure LLC misses, your benchmark's memory footprint must exceed your CPU's L3 cache capacity by roughly 1.5x to 2x. If the data fits entirely in the cache, DRAM latency is masked, flattening the performance differences between algorithms.

**Check your L3 cache size (Linux):**
```bash
lscpu | grep "L3"
```

**Sizing Guidelines:**
Adjust the constants in the benchmark headers based on your hardware. 
* **1D Vectors:** 4 bytes per 32-bit integer. (N = 1,000,000 is ~4 MB; N = 10,000,000 is ~40 MB).
* **2D Matrices:** A 2048×2048 integer matrix is 16 MB; 4096×4096 is 64 MB.
* **Lists/Trees:** ~24–32 bytes per node on 64-bit systems.

**Modification Example:**
```cpp
constexpr int N = 10'000'000; // Increase 1D elements
constexpr size_t rows = 4096; // Increase matrix dimensions
constexpr size_t cols = 4096;
```
## Licence

MIT
