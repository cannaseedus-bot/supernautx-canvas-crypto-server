# Phase 7.1 BUILD WORKAROUND
# Use pure C build to avoid MSVC STL cmath issues

This MSVC 19.50 (VS 2026 Insiders) has a known bug where <vector> and <string> 
transitively include <cmath> with broken float function declarations.

## Solution: Compile as Pure C (not C++)

Instead of fighting the toolchain, we can:
1. Keep all headers as C++ interfaces
2. Implement core math in pure C (no STL)
3. Link C object files to C++ interface

## Alternative: Use Clang-CL

```powershell
$env:CC = "clang-cl"
$env:CXX = "clang-cl"
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Alternative: Use Python Wrapper (Fastest)

Create Python interface that calls SIMD kernels directly.
