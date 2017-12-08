#include "handmade.h"

void RenderWeirdGradient(game_offscreen_buffer* buffer, int xOffsetVal, int yOffsetVal)
{
	uint8_t* row = (uint8_t*)buffer->memory;
	for(int Y = 0; Y < buffer->height; Y++)
	{
		uint8_t* pixel = (uint8_t*)row;
		for(int X = 0; X < buffer->width; X++)
		{
			/*
				pixel in memory: BB GG RR xx
				little endian architecture
				0x xxBBGGRR
			*/
			
			//BLUE
			*pixel = (uint8_t)(X + xOffsetVal); // using * because we're writing to the memory location stored in the pixel variabe
			pixel++; // not using * because we're moving the pointer (working out where the memory location is for the next pixel)

			//GREEN
			*pixel = (uint8_t)(Y - yOffsetVal);
			pixel++;

			//RED
			*pixel = (uint8_t)(buffer->width - X + xOffsetVal);
			pixel++;

			// PADDING BYTE
			*pixel = 0;
			pixel++;
			
		}
		row += buffer->pitch;
	}
}

void GameUpdateAndRender(game_offscreen_buffer* buffer, int xOffsetVal, int yOffsetVal)
{
	RenderWeirdGradient(buffer, xOffsetVal, yOffsetVal);
}