#pragma once

#if defined(__clang__)
	#define PAFIOM_COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
	#define PAFIOM_COMPILER_GCC
#elif defined(_MSC_VER)
	#define PAFIOM_COMPILER_MSVC
#else
	#define PAFIOM_COMPILER_UNKNOWN
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
	#define PAFIOM_PLATFORM_WINDOWS
#elif defined(__linux__)
	#define PAFIOM_PLATFORM_LINUX
#elif defined(__APPLE__)
	#define PAFIOM_PLATFORM_APPLE
#else
	#define PAFIOM_PLATFORM_UNKNOWN
#endif

#ifndef PAFIOM_NUM_PRIORITIES
	#define PAFIOM_NUM_PRIORITIES 6
#endif