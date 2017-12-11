#if !defined(HANDMADE_H)

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

void GameUpdateAndRender(game_offscreen_buffer* videoBuffer, game_sound_output_buffer* soundBuffer);

/*
	Services the platform layer provides to the game
*/

#define HANDMADE_H
#endif