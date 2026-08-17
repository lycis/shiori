#include <stdio.h>
#include "common.h"

static void print_compiler(void) {
#if defined(__clang__)
    printf("compiler: clang %d.%d.%d\n",
           __clang_major__,
           __clang_minor__,
           __clang_patchlevel__);

#elif defined(__GNUC__)
    printf("compiler: gcc %d.%d.%d\n",
           __GNUC__,
           __GNUC_MINOR__,
           __GNUC_PATCHLEVEL__);

#elif defined(_MSC_VER)
    printf("compiler: msvc %d\n", _MSC_VER);

#else
    printf("compiler: unknown\n");
#endif
}

static void print_platform(void)
{
#if defined(_WIN32)
    printf("platform: windows");
#elif defined(__linux__)
    printf("platform: linux");
#elif defined(__APPLE__)
    printf("platform: macos");
#else
    printf("platform: unknown");
#endif
}

static void print_architecture(void) {
#if defined(__x86_64__) || defined(_M_X64)
    printf(" x86_64");
#elif defined(__aarch64__) || defined(_M_ARM64)
    printf(" arm64");
#elif defined(__i386__) || defined(_M_IX86)
    printf(" x86");
#else
    printf(" unknown-arch");
#endif

    printf("\n");
}

static void print_c_standard(void)
{
#if defined(__STDC_VERSION__)
    #if __STDC_VERSION__ >= 202311L
        printf("c standard: C23\n");
    #elif __STDC_VERSION__ >= 201710L
        printf("c standard: C17\n");
    #elif __STDC_VERSION__ >= 201112L
        printf("c standard: C11\n");
    #elif __STDC_VERSION__ >= 199901L
        printf("c standard: C99\n");
    #else
        printf("c standard: pre-C99\n");
    #endif
#else
    printf("c standard: C90\n");
#endif
}

int command_version(int argc, char * argv[]) {
    printf("%s %s\n", APP_NAME, APP_VERSION);
    print_compiler();
    print_platform();
    print_architecture();
    print_c_standard();
    return R_OK;
}