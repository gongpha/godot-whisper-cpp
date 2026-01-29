#if defined(__arm__) || defined(_M_ARM) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/arm/cpu-feats.cpp"
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/arm/repack.cpp"
#endif

#if defined(__riscv) || defined(__riscv__)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/riscv/cpu-feats.cpp"
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/riscv/repack.cpp"
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/x86/cpu-feats.cpp"
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/x86/repack.cpp"
#endif

#if defined(__powerpc64__)
#include "../thirdparty/whisper.cpp/ggml/src/ggml-cpu/arch/powerpc/cpu-feats.cpp"
#endif