#ifndef __DEFINES_H
#define __DEFINES_H

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    // C11 estándar
    #define THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
    // Microsoft Visual C++
    #define THREAD_LOCAL __declspec(thread)
#else
    // GCC o Clang
    #define THREAD_LOCAL __thread
#endif

#endif