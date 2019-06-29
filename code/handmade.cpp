#include "handmade.h"

static void RenderWeirdGradient(game_offscreen_buffer* Buffer, int xOffsetVal, int yOffsetVal)
{
	uint8_t* Row = (uint8_t*)Buffer->Memory;
	for(int Y = 0; Y < Buffer->Height; Y++)
	{
		uint8_t* Pixel = (uint8_t*)Row;
		for(int X = 0; X < Buffer->Width; X++)
		{
			/*
				pixel in memory: BB GG RR xx
				little endian architecture
				0x xxBBGGRR
			*/
			
			//BLUE
			*Pixel = (uint8_t)(X + xOffsetVal); // using * because we're writing to the memory location stored in the pixel variable
			Pixel++; // not using * because we're moving the pointer (working out where the memory location is for the next pixel)

			//GREEN
			*Pixel = (uint8_t)(Y - yOffsetVal);
			Pixel++;

			//RED
			*Pixel = (uint8_t)(Buffer->Width - X + xOffsetVal);
			Pixel++;

			// PADDING BYTE
			*Pixel = 0;
			Pixel++;
			
		}
		Row += Buffer->Pitch;
	}
}


static void GameOutputSound(game_sound_output_buffer* SoundOutput, int ToneHz)
{
	static float tSine;
	int16_t ToneVolume = 3000;
	int WavePeriod = SoundOutput->samplesPerSecond / ToneHz;

	int16_t* SampleOut = SoundOutput->samples;
	for(int SampleIndex = 0;
		SampleIndex < SoundOutput->sampleCount;
		++SampleIndex)
	{
		float SineValue = sinf(tSine);
		int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue; // writes the sampleValue to the buffer
		*SampleOut++ = SampleValue;

		tSine += 2.0f * Pi32 * 1.0f/(float)WavePeriod;
	}
}

void GameUpdateAndRender(game_memory* Memory, game_input* GameInput, game_offscreen_buffer* VideoBuffer, game_sound_output_buffer* SoundBuffer)
{
	Assert(sizeof(game_state) <= (Memory->PermanentStorageSpace));

	game_state* GameState = (game_state*)Memory->PermanentStorage;

	if(!Memory->IsInitialized)
	{
		char* Filename = __FILE__;
		debug_read_file_result ReadFileResult = DEBUGPlatformReadEntireFile(Filename);
		if (ReadFileResult.Contents)
		{
			DEBUGPlatformWriteEntireFile("temp.out", ReadFileResult.ContentsSize, ReadFileResult.Contents);
			DEBUGPlatformFreeFileMemory(ReadFileResult.Contents);
		}
		
		GameState->toneHz = 256;
		GameState->xOffset = 0;
		GameState->yOffset = 0;
		Memory->IsInitialized = true;
	}

	game_controller_input* input0 = GetGameController(GameInput, 0);

	if(input0->IsAnalogue)
	{
		GameState->toneHz = 256 + (int)(128.0f * input0->StickAverageX);
		GameState->yOffset = (int)(4.0f * input0->StickAverageY);
	}
	else
	{
		if (input0->ActionRight.EndedDown)
		{
			if (GameState->toneHz < 44000)
			{
				GameState->toneHz += 2;
			}
		}
		else if (input0->ActionLeft.EndedDown)
		{
			if (GameState->toneHz > 5)
			{
				GameState->toneHz -= 2;
			}
		}

		if (input0->MoveUp.EndedDown)
		{
			GameState->yOffset -= 1;	
		}
		else if (input0->MoveDown.EndedDown)
		{
			GameState->yOffset += 1;	
		}
		else if (input0->MoveRight.EndedDown)
		{
			GameState->xOffset += 1;	
		}
		else if (input0->MoveLeft.EndedDown)
		{
			GameState->xOffset -= 1;	
		}
	}

	GameOutputSound(SoundBuffer, GameState->toneHz);
	RenderWeirdGradient(VideoBuffer, GameState->xOffset, GameState->yOffset);
}