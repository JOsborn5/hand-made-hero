#if !defined(HANDMADE_H)

/**
 * HANDMADE_INTERNAL:
 * 0 - Build for public release
 * 1 - Build for develop only
 *
 * HANDMADE_SLOW
 * 0 - No slow code allowed
 * 1 - Slow code welcome
 */
#if HANDMADE_INTERNAL
struct debug_read_file_result
{
	uint32_t ContentsSize;
	void* Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char* Filename);
internal void DEBUGPlatformFreeFileMemory(void* BitmapMemory);
internal bool DEBUGPlatformWriteEntireFile(char* Filename, uint32_t MemorySize, void* Memory);
#endif

#if HANDMADE_SLOW
	#define Assert(expression) if (!(expression)) { *(int*)0 = 0; }
#else
	#define Assert(expression)
#endif
#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)
#define ArrayCount(array) (sizeof(array) / (sizeof((array)[0])))

/*
	Services the game provides to the platform layer
*/

inline uint32_t SafeTruncateUInt64(uint64_t ValueToTruncate)
{
	Assert(ValueToTruncate < 0xFFFFFFFF);
	uint32_t TruncatedValue = (uint32_t)ValueToTruncate;
	return TruncatedValue;
}

// Stuff for our RGB display buffer
struct game_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel = 4;
};

struct game_sound_output_buffer
{
	int samplesPerSecond;
	int sampleCount;
	int16_t* samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	int EndedDown;
};

struct game_controller_input
{
	bool IsAnalogue;

	float StickAverageX;
	float StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Start;
			game_button_state Back;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[4];
};

inline game_controller_input* GetGameController(game_input* Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input* Result = &Input->Controllers[ControllerIndex];
	return Result;
}

struct game_state
{
	int toneHz;
	int xOffset;
	int yOffset;
};

struct game_memory
{
	bool IsInitialized;
	uint64_t PermanentStorageSpace;
	uint64_t TransientStorageSpace;
	void* PermanentStorage;
	void* TransientStorage;
};

void GameUpdateAndRender(game_memory* Memory, game_input* GameInput, game_offscreen_buffer* VideoBuffer, game_sound_output_buffer* SoundBuffer);

/*
	Services the platform layer provides to the game
*/

#define HANDMADE_H
#endif