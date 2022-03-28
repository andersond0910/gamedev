#pragma once
struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer
{
    int sample_count_to_output;
    int samples_per_second;
    int16_t *samples;
};

/*
    services that the platform layer provides to the game
*/


/*
    services that the game provides to the platform layer
*/

//needs four things - timing, controller/keyboard input, bitmap to output, sound to output
static void GameUpdateAndRender(game_offscreen_buffer&,game_sound_output_buffer& ,int BlueOffset, int GreenOffset, int tone_hz);