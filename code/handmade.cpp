#include "handmade.h"

static void RenderWeirdGradient(game_offscreen_buffer* buffer, int xOffsetVal, int yOffsetVal)
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


static void GameOutputSound(game_sound_output_buffer* soundOutput)
{
	static float tSine;
	int16_t toneVolume = 3000;
	int toneHz = 256;
	int wavePeriod = soundOutput->samplesPerSecond / toneHz;

	int16_t *sampleOut = soundOutput->samples;
	for(int sampleIndex = 0;
		sampleIndex < soundOutput->sampleCount;
		++sampleIndex)
	{
		float sineValue = sinf(tSine);
		int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		*sampleOut++ = sampleValue; // writes the sampleValue to the buffer
		*sampleOut++ = sampleValue;

		tSine += 2.0f * Pi32 * 1.0f/(float)wavePeriod;
	}
}

void GameUpdateAndRender(game_memory *memory, game_input* gameInput, game_offscreen_buffer* videoBuffer, game_sound_output_buffer* soundBuffer)
{
	game_state *gameState = (game_state *)memory->permanentStorage;

	if(!memory->isInitialized)
	{
		gameState->toneHz = 256;
		gameState->xOffset = 0;
		gameState->yOffset = 0;
		memory->isInitialized = true;
	}

	game_controller_input *input0 = &gameInput->controllers[0];

	if(input0->isAnalogue)
	{

	}
	else
	{
		gameState->toneHz = 256 + (int)(128.0f * input0->endX);
		gameState->yOffset = (int)(4.0f * input0->endY);
	}

	GameOutputSound(soundBuffer);
	RenderWeirdGradient(videoBuffer, gameState->xOffset, gameState->yOffset);
}