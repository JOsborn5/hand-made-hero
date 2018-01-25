#if !defined(WIN32_HANDMADE_H)
struct win32_window_dimension
{
	int width;
	int height;
};

struct win32_sound_output
{
	int samplesPerSecond;
	uint32_t runningSampleIndex; // unsigned because we want this to loop back to zero once it hits its max
	int bytesPerSample;
	int soundBufferSize;
	int latencySampleCount;
};
#define HANDMADE_H
#endif
