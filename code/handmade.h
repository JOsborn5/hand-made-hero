#if !defined(HANDMADE_H)

/*
	Services the game provides to the platform layer
*/
void GameUpdateAndRender();

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

/*
	Services the platform layer provides to the game
*/

#define HANDMADE_H
#endif