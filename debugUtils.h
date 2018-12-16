#ifndef ASSERT_H
#define ASSERT_H

#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/

#ifndef NDEBUG
#include <stdio.h>

#if defined (__GNUC__)

#define DEBUG_BREAK() __asm__ volatile("int $3")

#elif defined(_MSC_VER) /*__GNUC__/_MSC_VER*/

#include <intrin.h>
#pragma intrinsic(__debugbreak)

#define DEBUG_BREAK() __debugbreak()

#else /*__GNUC__/_MSC_VER*/

#error "Unsupported Compiler.  Try using gcc/g++, clang/clang++/clang-cl, or MSVC."

#endif /*__GNUC__/_MSC_VER*/
    
static inline void __assert(int32_t expr, const char* msg, const char* file, int32_t line)
{
    if(!expr)
    {
        printf("Assertion failed at %s:%d!\n%s\n", file, line, msg);
        DEBUG_BREAK();
    }
}


#define DEBUG_PRINT(FORMAT, ...) printf(FORMAT, __VA_ARGS__)

#define ASSERT(x, msg) __assert(x, msg, __FILE__, __LINE__)


#else /*NDEBUG*/
#define ASSERT(x, msg)
#define DEBUG_PRINT(FORMAT, ...)
#define DEBUG_BREAK()

#endif /*NDEBUG*/

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*ASSERT_H*/
