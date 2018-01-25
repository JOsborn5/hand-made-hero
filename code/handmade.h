#if !defined(HANDMADE_H)

#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define ArrayCount(array) (sizeof(array) / (sizeof((array)[0])))

/*
	Services the game provides to the platform layer
*/

// Stuff for our RGB display buffer
struct game_offscreen_buffer
{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel = 4;
};

struct game_sound_output_buffer
{
	int samplesPerSecond;
	int sampleCount;
	int16_t* samples;
};

struct game_button_state
{
	int halfTransitionCount;
	int endedDown;
};

struct game_controller_input
{
	bool isAnalogue;

	float startX;
	float startY;

	float minX;
	float minY;

	float maxX;
	float maxY;

	float endX;
	float endY;

	game_button_state up;
	game_button_state down;
	game_button_state left;
	game_button_state right;
};

struct game_input
{
	game_controller_input controllers[4];
};

struct game_state
{
	int toneHz;
	int xOffset;
	int yOffset;
};

struct game_memory
{
	bool isInitialized;
	uint64_t permanentStorageSpace;
	void *permanentStorage;
};

void GameUpdateAndRender(game_memory *memory, game_input* gameInput, game_offscreen_buffer* videoBuffer, game_sound_output_buffer* soundBuffer);

/*
	Services the platform layer provides to the game
*/

#define HANDMADE_H
#endif