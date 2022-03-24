#include<cstdint>
#include "handmade.h"


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

static void GameUpdateAndRender(game_offscreen_buffer& buffer, int BlueOffset, int GreenOffset)
{
    RenderWeirdGradient(buffer,BlueOffset,GreenOffset);
}