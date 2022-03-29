#pragma once
#include<windows.h>
#include<cstdint>

struct win32_offscreen_buffer{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int BytesPerPixel = 4;
	int Pitch;
	int MemorySize;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
    int32_t samples_per_second = 48000;
    int32_t bytes_per_sample = sizeof(int16_t)*2;
    uint32_t running_sample_index = 0;
    uint32_t buffer_size = samples_per_second * bytes_per_sample;
    float tSine = 0;
    int latency_sample_count;
};