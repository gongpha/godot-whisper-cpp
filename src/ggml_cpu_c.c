#if !defined(_WIN32) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE  1 
#endif

#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/ggml-cpu.c"

#if defined(__arm__) || defined(_M_ARM) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/arm/quants.c"
#endif

// godot may not support some architectures, but we include them for completeness anyway :P

#if defined(__loongarch64)
#include "../thirdparty/llama/ggml/src/ggml-cpu/arch/loongarch/quants.c"
#endif

#if defined(__riscv) || defined(__riscv__)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/riscv/quants.c"
#endif

#if defined(__wasm__) || defined(__wasm32__) || defined(__wasm64__)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/wasm/quants.c"
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/x86/quants.c"
#endif

#if defined(__powerpc64__)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/powerpc/quants.c"
#endif