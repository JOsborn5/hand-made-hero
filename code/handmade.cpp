#include "handmade.h"

static void RenderWeirdGradient(game_offscreen_buffer *buffer, int xOffsetVal, int yOffsetVal)
{
	uint8_t *row = (uint8_t *)buffer->memory;
	for(int Y = 0; Y < buffer->height; Y++)
	{
		uint8_t *pixel = (uint8_t *)row;
		for(int X = 0; X < buffer->width; X++)
		{
			/*
				pixel in memory: BB GG RR xx
				little endian architecture
				0x xxBBGGRR
			*/
			
			//BLUE
			*pixel = (uint8_t)(X + xOffsetVal); // using * because we're writing to the memory location stored in the pixel variable
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


static void GameOutputSound(game_sound_output_buffer *SoundOutput)
{
	static float tSine;
	int16_t toneVolume = 3000;
	int ToneHz = 256;
	int WavePeriod = SoundOutput->samplesPerSecond / ToneHz;

	int16_t *sampleOut = SoundOutput->samples;
	for(int sampleIndex = 0;
		sampleIndex < SoundOutput->sampleCount;
		++sampleIndex)
	{
		float sineValue = sinf(tSine);
		int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		*sampleOut++ = sampleValue; // writes the sampleValue to the buffer
		*sampleOut++ = sampleValue;

		tSine += 2.0f * Pi32 * 1.0f/(float)WavePeriod;
	}
}

void GameUpdateAndRender(game_memory *memory, game_input *GameInput, game_offscreen_buffer *VideoBuffer, game_sound_output_buffer *SoundBuffer)
{
	Assert(sizeof(game_state) <= (memory->permanentStorageSpace));

	game_state *GameState = (game_state *)memory->permanentStorage;

	if(!memory->isInitialized)
	{
		GameState->toneHz = 256;
		GameState->xOffset = 0;
		GameState->yOffset = 0;
		memory->isInitialized = true;
	}

	game_controller_input *input0 = &GameInput->controllers[0];

	if(input0->isAnalogue)
	{

	}
	else
	{
		GameState->toneHz = 256 + (int)(128.0f * input0->endX);
		GameState->yOffset = (int)(4.0f * input0->endY);
	}

	GameOutputSound(SoundBuffer);
	RenderWeirdGradient(VideoBuffer, GameState->xOffset, GameState->yOffset);
}