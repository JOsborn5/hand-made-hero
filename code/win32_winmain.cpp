#include <windows.h>
#include <stdint.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define global_variable static

#define Pi32 3.14159265359f

// Macro:
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
// Pointer to our macro:
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// Stuff for our RGB display buffer
struct win32_offscreen_buffer
{
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel = 4;
};

struct win32_window_dimension
{
	int width;
	int height;
};

struct win32_sound_output
{
	int samplesPerSecond;
	int *toneHz;
	int toneVolume;
	uint32_t runningSampleIndex; // unsigned because we want this to loop back to zero once it hits its max
	int wavePeriod;
	int bytesPerSample;
	int soundBufferSize;
	float tSine;
};

global_variable bool isRunning = false;
global_variable win32_offscreen_buffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSoundBuffer;
global_variable int xOffset = 0;
global_variable int yOffset = 0;
global_variable int pitch = 256;

internal void RenderWeirdGradient(win32_offscreen_buffer *buffer, int xOffsetVal, int yOffsetVal)
{
	uint8_t *row = (uint8_t *) buffer->memory;
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

// Note this changes the buffer so we're passing the buffer as a pointer so this function will update it
internal void Win32_ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
	if(buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;

	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = (buffer->width * buffer->height) * buffer->bytesPerPixel;
	buffer->pitch = buffer->width * buffer->bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32_InitDirectSound(HWND window, int32_t samplesPerSecond, int32_t bufferSize)
{
	// Load the library
	HMODULE DirectSoundLibrary = LoadLibraryA("dsound.dll");

	if(DirectSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)
			GetProcAddress(DirectSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.wBitsPerSample = 16;
			waveFormat.cbSize = 0;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

			if(SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
			{
				// clear the buffer memory space to zero
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER primaryBuffer;
				if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					HRESULT error = primaryBuffer->SetFormat(&waveFormat);
					if(SUCCEEDED(error))
					{
						OutputDebugStringA("Primary buffer format was set.\n");
						// we have finally set the format!
					}
				}
			}

			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			
			HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &globalSoundBuffer, 0);
			if(SUCCEEDED(error))
			{
				OutputDebugStringA("Secondary sound buffer created successfully.\n");
			}

			// Create a secondary buffer (2 sec duration that we write to)
			bufferDescription.dwBufferBytes = bufferSize;

			// Start playing
		}
		else
		{
			// disagnostics
		}
	}
}

win32_window_dimension Win32_GetWindowDimension(HWND window)
{
	win32_window_dimension windowDim;

	RECT clientRect;
	GetClientRect(window, &clientRect);
	
	windowDim.width = clientRect.right - clientRect.left;
	windowDim.height = clientRect.bottom - clientRect.top;

	return windowDim;
}

internal void Win32_DisplayBufferInWindow(HDC deviceContext, int width, int height, win32_offscreen_buffer *buffer)
{
	StretchDIBits(deviceContext,
		0, 0, width, height,
		0, 0, buffer->width, buffer->height,
		buffer->memory,
		&buffer->info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK Win32_MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = -1;

	switch(message)
	{
		case WM_SIZE:
		{

		} break;
		case WM_DESTROY:
		{
			isRunning = false;
		} break;
		case WM_CLOSE:
		{
			isRunning = false;
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32_t vKCode = wParam;
			bool wasDown = ((lParam & (1 << 30)) != 0); // Bit #30 of the LParam tells us what the previous key was
			bool isDown = ((lParam & (1 << 30)) == 0); // Bit #31 of the LParam tells us what the current key is
			if(wasDown != isDown)
			{
				if(vKCode == VK_UP)
				{
					yOffset -= 10;
					pitch += 20;
				}
				else if (vKCode == VK_DOWN)
				{
					yOffset += 10;
					pitch -= 20;
				}
				else if (vKCode == VK_LEFT)
				{
					xOffset -= 10;
				}
				else if (vKCode == VK_RIGHT)
				{
					xOffset += 10;
				}
				else if (vKCode == VK_ESCAPE)
				{
					xOffset = 0;
					yOffset = 0;
					pitch = 256;
				}
				else if (vKCode == VK_SPACE)
				{

				}
			}

			bool altKeyDown = ((lParam & (1 << 29)) != 0);
			if((vKCode == VK_F4) && altKeyDown)
			{
				isRunning = false;
			}
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(window, &paint);
			win32_window_dimension windowSize = Win32_GetWindowDimension(window);
			Win32_DisplayBufferInWindow(deviceContext, windowSize.width, windowSize.height, &globalBackBuffer);
			EndPaint(window, &paint);
		} break;
		default:
		{
			// use windows default callback handler
			result = DefWindowProc(window, message, wParam, lParam);
		} break;
	}

	return result;
}

void Win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
	VOID *region1;
	DWORD region1Size;
	VOID *region2;
	DWORD region2Size;

	HRESULT gotLock = SUCCEEDED(globalSoundBuffer->Lock(
		byteToLock,
		bytesToWrite,
		&region1, &region1Size,
		&region2, &region2Size,
		0
	));
	if(gotLock)
	{
		// TODO: Assert region1Size is valid
		DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
		int16_t *sampleOut = (int16_t *)region1;
		for(DWORD sampleIndex = 0;
			sampleIndex < region1SampleCount;
			++sampleIndex)
		{
			float sineValue = sinf(soundOutput->tSine);
			int16_t sampleValue = (int16_t)(sineValue * soundOutput->toneVolume);
			*sampleOut++ = sampleValue; // writes the sampleValue to the buffer
			*sampleOut++ = sampleValue;
			soundOutput->wavePeriod = soundOutput->samplesPerSecond / *soundOutput->toneHz;
			soundOutput->tSine += 2.0f * Pi32 * 1.0f/(float)soundOutput->wavePeriod;
			++soundOutput->runningSampleIndex;
		}

		DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
		sampleOut = (int16_t *)region2;
		for(DWORD sampleIndex = 0;
			sampleIndex < region2SampleCount;
			++sampleIndex)
		{
			float sineValue = sinf(soundOutput->tSine);
			int16_t sampleValue = (int16_t)(sineValue * soundOutput->toneVolume);
			*sampleOut++ = sampleValue; // writes the sampleValue to the buffer
			*sampleOut++ = sampleValue;
			soundOutput->wavePeriod = soundOutput->samplesPerSecond / *soundOutput->toneHz;
			soundOutput->tSine += 2.0f * Pi32 * 1.0f/(float)soundOutput->wavePeriod;
			++soundOutput->runningSampleIndex;
		}

		globalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
	WNDCLASSA windowClass = {};
	windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	windowClass.lpfnWndProc = Win32_MainWindowCallback;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = "Fucking A!";

	Win32_ResizeDIBSection(&globalBackBuffer, 1200, 700);

	if(RegisterClass(&windowClass))
	{
		HWND window = CreateWindowEx(
								0,
								windowClass.lpszClassName,
								"Fucking AAA!",
								WS_OVERLAPPEDWINDOW|WS_VISIBLE,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								0,
								0,
								instance,
								0);
		if(window)
		{
			MSG message;
			isRunning = true;

			win32_sound_output soundOutput = {};
		
			soundOutput.samplesPerSecond = 48000;
			soundOutput.toneHz = &pitch;
			soundOutput.toneVolume = 3000;
			soundOutput.runningSampleIndex = 0; // unsigned because we want this to loop back to zero once it hits its max
			soundOutput.wavePeriod = soundOutput.samplesPerSecond / *soundOutput.toneHz;
			soundOutput.bytesPerSample = sizeof(int16_t) * 2;
			soundOutput.soundBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;

			Win32_InitDirectSound(window, soundOutput.samplesPerSecond, soundOutput.soundBufferSize);
			Win32FillSoundBuffer(&soundOutput, 0, soundOutput.soundBufferSize);
			globalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

			while(isRunning)
			{
				// flush the queue of messages from windows in this loop
				while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if(message.message == WM_QUIT)
					{
						isRunning = false;
					}
					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				RenderWeirdGradient(&globalBackBuffer, xOffset, yOffset);

				// Direct sound output test
				// Lock direct sound buffer
				DWORD playCursor;
				DWORD writeCursor;
				if(SUCCEEDED(globalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
				{
					DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.soundBufferSize;
					DWORD bytesToWrite;
					if(byteToLock == playCursor)	// If nothing has been played.
					{
						bytesToWrite = 0;
					}
					else if(byteToLock > playCursor)	// write cursor is ahead of the play cursor in the circular buffer.
														// So we want to write from the write cursor to the end of the buffer, plsu from the start of the buffer to the play cursor.
					{
						bytesToWrite = soundOutput.soundBufferSize - byteToLock;
						bytesToWrite += playCursor;
					}
					else // play cursor is ahead of the write cursor, so we want to write from write cursor to the play cursor
					{
						bytesToWrite = playCursor - byteToLock;
					}

					Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
				}

				win32_window_dimension windowSize = Win32_GetWindowDimension(window);
				HDC deviceContext = GetDC(window);
				Win32_DisplayBufferInWindow(deviceContext, windowSize.width, windowSize.height, &globalBackBuffer);
				ReleaseDC(window, deviceContext);

				xOffset += 1;
				yOffset += 1;
			}
		}
		else
		{
			// Handle unable to create window
		}
	}
	else
	{
		// Handle windows window registration failure
	}

	return (0);
}
