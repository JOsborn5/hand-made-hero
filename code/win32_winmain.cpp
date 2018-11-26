#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define global_variable static

#define Pi32 3.14159265359f

// Macro:
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
// Pointer to our macro:
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#include "handmade.cpp"
#include "win32_handmade.h"

global_variable bool isRunning = false;
global_variable game_offscreen_buffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSoundBuffer;

// Note this changes the buffer so we're passing the buffer as a pointer so this function will update it
internal void Win32_ResizeDIBSection(game_offscreen_buffer* buffer, int width, int height)
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
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
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

internal void Win32_DisplayBufferInWindow(HDC deviceContext, int width, int height, game_offscreen_buffer* buffer)
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
				}
				else if (vKCode == VK_DOWN)
				{
				}
				else if (vKCode == VK_LEFT)
				{
				}
				else if (vKCode == VK_RIGHT)
				{
				}
				else if (vKCode == VK_ESCAPE)
				{
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

void Win32ClearBuffer(win32_sound_output *soundOutput)
{
	VOID *region1;
	DWORD region1Size;
	VOID *region2;
	DWORD region2Size;

	HRESULT gotLock = SUCCEEDED(globalSoundBuffer->Lock(
		0,
		soundOutput->soundBufferSize,
		&region1, &region1Size,
		&region2, &region2Size,
		0
	));
	if(gotLock)
	{
		uint8_t *destSample = (uint8_t *)region1;
		for(DWORD byteIndex = 0;
			byteIndex < region1Size;
			++byteIndex)
		{
			*destSample++ = 0;
		}

		destSample = (uint8_t *)region2;
		for(DWORD byteIndex = 0;
			byteIndex < region2Size;
			++byteIndex)
		{
			*destSample++ = 0;
		}

		globalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

void Win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite, game_sound_output_buffer *sourceBuffer)
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
		int16_t *destSample = (int16_t *)region1;
		int16_t *sourceSample = sourceBuffer->samples;
		for(DWORD sampleIndex = 0;
			sampleIndex < region1SampleCount;
			++sampleIndex)
		{
			*destSample++ = *sourceSample++; // writes the samples from sourceSample to destSample
			*destSample++ = *sourceSample++;
			++(soundOutput->runningSampleIndex);
		}

		DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
		destSample = (int16_t *)region2;
		for(DWORD sampleIndex = 0;
			sampleIndex < region2SampleCount;
			++sampleIndex)
		{
			*destSample++ = *sourceSample++; // writes the samples from sourceSample to destSample
			*destSample++ = *sourceSample++;
			++(soundOutput->runningSampleIndex);
		}

		globalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

void DisplayLastWin32Error()
{
	DWORD errorCode = GetLastError();
	char errorCodeBuffer[256];
	wsprintf(errorCodeBuffer, "VirtualAlloc error code: %d\n", errorCode);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
	LARGE_INTEGER perfCounterFrequencyResult;
	QueryPerformanceFrequency(&perfCounterFrequencyResult);
	int64_t perfCountFrequency = perfCounterFrequencyResult.QuadPart;

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
			isRunning = true;

			win32_sound_output soundOutput = {};
		
			soundOutput.samplesPerSecond = 48000;
			soundOutput.runningSampleIndex = 0; // unsigned because we want this to loop back to zero once it hits its max
			soundOutput.bytesPerSample = sizeof(int16_t) * 2;
			soundOutput.soundBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
			soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

			Win32_InitDirectSound(window, soundOutput.samplesPerSecond, soundOutput.soundBufferSize);
			Win32ClearBuffer(&soundOutput);
			globalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

			LARGE_INTEGER lastCounter;
			QueryPerformanceCounter(&lastCounter);

			int64_t lastCycleCount = __rdtsc();

			bool successfulMemoryAllocation = true;
			int16_t *samples = (int16_t *)VirtualAlloc(0, soundOutput.soundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(samples == NULL)
			{
				successfulMemoryAllocation = false;
				DisplayLastWin32Error();
			}

			game_memory gameMemory = {};
			gameMemory.permanentStorageSpace = Megabytes(64);
			gameMemory.permanentStorage = VirtualAlloc(0, gameMemory.permanentStorageSpace, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(gameMemory.permanentStorage == NULL)
			{
				successfulMemoryAllocation = false;
				DisplayLastWin32Error();
			}

			gameMemory.transientStorageSpace = Gigabytes((uint64_t)4);
			gameMemory.transientStorage = VirtualAlloc(0, gameMemory.transientStorageSpace, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(gameMemory.transientStorage == NULL)
			{
				successfulMemoryAllocation = false;
				DisplayLastWin32Error();
			}

			game_sound_output_buffer soundBuffer = {};
			soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
			soundBuffer.samples = samples;

			while(successfulMemoryAllocation && isRunning)
			{
				MSG message;

				game_input gameInput = {};

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

				// Direct sound output test
				// Lock direct sound buffer
				DWORD byteToLock;
				DWORD targetCursor;
				DWORD bytesToWrite;
				DWORD playCursor;
				DWORD writeCursor;
				bool soundIsValid = false;
				if(SUCCEEDED(globalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
				{
					byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.soundBufferSize;
					targetCursor = ((playCursor +
						(soundOutput.latencySampleCount * soundOutput.bytesPerSample)) %
						soundOutput.soundBufferSize);
					if(byteToLock > targetCursor)	// write cursor is ahead of the play cursor in the circular buffer.
													// So we want to write from the write cursor to the end of the buffer, plus from the start of the buffer to the play cursor.
					{
						bytesToWrite = soundOutput.soundBufferSize - byteToLock;
						bytesToWrite += targetCursor;
					}
					else // play cursor is ahead of the write cursor, so we want to write from write cursor to the play cursor
					{
						bytesToWrite = targetCursor - byteToLock;
					}
					soundIsValid = true;
				}

				soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
				GameUpdateAndRender(&gameMemory, &gameInput, &globalBackBuffer, &soundBuffer);

				if(soundIsValid)
				{
					Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
				}

				win32_window_dimension windowSize = Win32_GetWindowDimension(window);
				HDC deviceContext = GetDC(window);
				Win32_DisplayBufferInWindow(deviceContext, windowSize.width, windowSize.height, &globalBackBuffer);
				ReleaseDC(window, deviceContext);

				int64_t endCycleCount = __rdtsc();
				int64_t cyclesElapsed = endCycleCount - lastCycleCount;
				int32_t megaCyclesPerFrame = cyclesElapsed / (1000 * 1000);

				LARGE_INTEGER endCounter;
				QueryPerformanceCounter(&endCounter);
				int64_t counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
				int32_t msPerFrame = (1000 * counterElapsed) / perfCountFrequency;

				char buffer[256];
				wsprintf(buffer, "ms/frame: %d, mega cycles/frame: %d\n", msPerFrame, megaCyclesPerFrame);
				OutputDebugStringA(buffer);

				lastCounter = endCounter;
				lastCycleCount = endCycleCount;
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
