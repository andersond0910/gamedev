#include<cstdint>
#include<cmath>
#include "handmade.h"

static void
OutputGameSound(game_sound_output_buffer& sound_buffer, int tone_hz)
{
    const float Pi32 = 3.14159265359;

    static float tSine = 0;
    int16_t tone_volume = 3000;
    auto sample = sound_buffer.samples;
    int wave_period = sound_buffer.samples_per_second/tone_hz;
            
    for(int sample_index = 0; sample_index < sound_buffer.sample_count_to_output; ++sample_index)
    {
        float sine_value = sin(tSine);
        int16_t sample_value = (int16_t)(sine_value * tone_volume);
        *sample++ = sample_value;//write to left
        *sample++ = sample_value;//write to right
        tSine += (2.0f * Pi32* 1.0f) / (float)wave_period;
    }
}

static void
RenderWeirdGradient(game_offscreen_buffer& buffer, int BlueOffset, int GreenOffset)
{
	auto Row = reinterpret_cast<uint8_t*>(buffer.Memory);
	for(int y=0;y<buffer.Height;++y)
	{
		auto Pixel = reinterpret_cast<uint32_t*>(Row);
		for(int x = 0; x < buffer.Width; ++x)
		{
			//Pixel in memory:  xx RR GG BB
			uint32_t Blue = x+BlueOffset, Green = y+GreenOffset;
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += buffer.Pitch;
	}
}

static void GameUpdateAndRender(game_offscreen_buffer& buffer,game_sound_output_buffer& sound_buffer ,int BlueOffset, int GreenOffset, int tone_hz)
{
    OutputGameSound(sound_buffer, tone_hz);
    RenderWeirdGradient(buffer,BlueOffset,GreenOffset);
}