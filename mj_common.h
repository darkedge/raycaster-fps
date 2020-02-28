#pragma once
#include <stdint.h>

// Annotation macros
#define MJ_UNINITIALIZED
#define MJ_REF

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

// Raytracer resolution
static constexpr uint16_t MJ_RT_WIDTH  = 320;
static constexpr uint16_t MJ_RT_HEIGHT = 200;

// Window resolution
static constexpr uint16_t MJ_WND_WIDTH  = 1280;
static constexpr uint16_t MJ_WND_HEIGHT = 800;
