//linux types
#include <stdint.h>
#include <cstddef>
typedef uint16_t u16;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


#define FASTCALL
#define __fastcall
#define fastcall
#define ALIGN(x) __attribute__((aligned(x)))

void __debugbreak();

#ifdef _ANDROID
	#include <stdlib.h>
	#include <stdio.h>
	#include <android/log.h>

	#define  LOG_TAG    "libnullDC_Core"
	#ifdef printf 
	#undef printf
	#endif

	#define  printf(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif
