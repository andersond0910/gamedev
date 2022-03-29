#include<cstdint>
#include<cmath>
#include "handmade.h"

internal void OutputGameSound(game_sound_output_buffer& sound_buffer, int tone_hz)
{
    const float Pi32 = 3.14159265359;

    local_persistent float t_sine = 0;
    int16_t tone_volume = 3000;
    auto sample = sound_buffer.samples;
    int wave_period = sound_buffer.samples_per_second/tone_hz;
            
    for(int sample_index = 0; sample_index < sound_buffer.sample_count_to_output; ++sample_index)
    {
        float sine_value = sin(t_sine);
        int16_t sample_value = (int16_t)(sine_value * tone_volume);
        *sample++ = sample_value;//write to left
        *sample++ = sample_value;//write to right
        t_sine += (2.0f * Pi32* 1.0f) / (float)wave_period;
    }
}

internal void RenderWeirdGradient(game_offscreen_buffer& buffer, int& blue_offset, int green_offset)
{
	auto Row = reinterpret_cast<uint8_t*>(buffer.Memory);
	for(int y=0;y<buffer.Height;++y)
	{
		auto Pixel = reinterpret_cast<uint32_t*>(Row);
		for(int x = 0; x < buffer.Width; ++x)
		{
			//Pixel in memory:  xx RR GG BB
			uint32_t Blue = x+blue_offset, Green = y+green_offset;
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += buffer.Pitch;
	}
}

internal void GameUpdateAndRender(game_input& input,game_offscreen_buffer& buffer,game_sound_output_buffer& sound_buffer)
{
    local_persistent int blue_offset = 0, green_offset = 0, tone_hz = 256;
    game_controller_input player_1 = input.controllers[0];

    //TODO: needs tuning for analog movement
    if(player_1.is_analog)
    {
        tone_hz = 256 + static_cast<int>(128.0f * (player_1.end_x));
        blue_offset += static_cast<int>(4.0f*player_1.end_y);
    }
    //needs tuning for digital movement
    else
    {

    }

    if(player_1.down.ended_down)
    {
        green_offset += 1;
    }

    OutputGameSound(sound_buffer, tone_hz);
    RenderWeirdGradient(buffer,blue_offset, green_offset);
}