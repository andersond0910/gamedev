#pragma once
struct game_offscreen_buffer{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

/*
    services that the platform layer provides to the game
*/


/*
    services that the game provides to the platform layer
*/

//needs four things - timing, controller/keyboard input, bitmap to output, sound to output
static void GameUpdateAndRender(game_offscreen_buffer&, int BlueOffset, int GreenOffset);