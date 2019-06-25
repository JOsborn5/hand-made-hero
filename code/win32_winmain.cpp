#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define global_variable static

#define Pi32 3.14159265359f

#include "handmade.cpp"
#include "win32_handmade.h"

// Macro:
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
// Pointer to our macro:
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal debug_read_file_result DEBUGPlatformReadEntireFile(char* FileName)
{
	debug_read_file_result ReadFileResult = {};
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32_t FileSize32 = SafeTruncateUInt64(FileSize.QuadPart); // assert we're not trying to open a file larger than 4GB, since ReadFile will break for 4GB+ file
			ReadFileResult.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(ReadFileResult.Contents)
			{
				DWORD BytesToRead;
				if(ReadFile(FileHandle, ReadFileResult.Contents, FileSize32, &BytesToRead, 0) && FileSize32 == BytesToRead)
				{
					ReadFileResult.ContentsSize = BytesToRead;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(ReadFileResult.Contents);
					ReadFileResult.Contents = 0;
				}
			}
		}

		CloseHandle(FileHandle);
	}

	return ReadFileResult;
}

internal void DEBUGPlatformFreeFileMemory(void* Memory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool DEBUGPlatformWriteEntireFile(char* FileName, uint32_t MemorySize, void* Memory)
{
	bool Result = false;
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			Result = (MemorySize == BytesWritten);
		}

		CloseHandle(FileHandle);
	}

	return (Result);
}

global_variable bool IsRunning = false;
global_variable game_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

// Note this changes the buffer so we're passing the buffer as a pointer so this function will update it
internal void Win32_ResizeDIBSection(game_offscreen_buffer* Buffer, int Width, int Height)
{
	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32_InitDirectSound(HWND window, int32_t samplesPerSecond, int32_t bufferSize)
{
	// Load the library
	HMODULE DirectSoundLibrary = LoadLibraryA("dsound.dll");

	if(DirectSoundLibrary)
	{
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DirectSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.cbSize = 0;
			WaveFormat.nSamplesPerSec = samplesPerSecond;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

			if(SUCCEEDED(DirectSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
			{
				// clear the buffer memory space to zero
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if(SUCCEEDED(Error))
					{
						OutputDebugStringA("Primary buffer format was set.\n");
						// we have finally set the format!
					}
				}
			}

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = bufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSoundBuffer, 0);
			if(SUCCEEDED(Error))
			{
				OutputDebugStringA("Secondary sound buffer created successfully.\n");
			}

			// Create a secondary buffer (2 sec duration that we write to)
			BufferDescription.dwBufferBytes = bufferSize;

			// Start playing
		}
		else
		{
			// disagnostics
		}
	}
}

win32_window_dimension Win32_GetWindowDimension(HWND Window)
{
	win32_window_dimension WindowDim;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	
	WindowDim.width = ClientRect.right - ClientRect.left;
	WindowDim.height = ClientRect.bottom - ClientRect.top;

	return WindowDim;
}

internal void Win32_DisplayBufferInWindow(HDC DeviceContext, int Width, int Height, game_offscreen_buffer* Buffer)
{
	StretchDIBits(DeviceContext,
		0, 0, Width, Height,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory,
		&Buffer->Info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK Win32_MainWindowCallback(HWND window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = -1;

	switch(Message)
	{
		case WM_SIZE:
		{

		} break;
		case WM_DESTROY:
		{
			IsRunning = false;
		} break;
		case WM_CLOSE:
		{
			IsRunning = false;
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Assert(!"NOOOOOOOO");
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC DeviceContext = BeginPaint(window, &paint);
			win32_window_dimension WindowSize = Win32_GetWindowDimension(window);
			Win32_DisplayBufferInWindow(DeviceContext, WindowSize.width, WindowSize.height, &GlobalBackBuffer);
			EndPaint(window, &paint);
		} break;
		default:
		{
			// use windows default callback handler
			Result = DefWindowProc(window, Message, wParam, lParam);
		} break;
	}

	return Result;
}

internal void Win32ClearBuffer(win32_sound_output* SoundOutput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	HRESULT gotLock = SUCCEEDED(GlobalSoundBuffer->Lock(
		0,
		SoundOutput->soundBufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0
	));
	if(gotLock)
	{
		uint8_t* DestSample = (uint8_t*)Region1;
		for(DWORD ByteIndex = 0;
			ByteIndex < Region1Size;
			++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (uint8_t*)Region2;
		for(DWORD ByteIndex = 0;
			ByteIndex < Region2Size;
			++ByteIndex)
		{
			*DestSample++ = 0;
		}

		GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer* SourceBuffer)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	HRESULT gotLock = SUCCEEDED(GlobalSoundBuffer->Lock(
		ByteToLock,
		BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0
	));
	if(gotLock)
	{
		// TODO: Assert Region1Size is valid
		DWORD Region1SampleCount = Region1Size / SoundOutput->bytesPerSample;
		int16_t* DestSample = (int16_t*)Region1;
		int16_t* SourceSample = SourceBuffer->samples;
		for(DWORD SampleIndex = 0;
			SampleIndex < Region1SampleCount;
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++; // writes the samples from SourceSample to DestSample
			*DestSample++ = *SourceSample++;
			++(SoundOutput->runningSampleIndex);
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->bytesPerSample;
		DestSample = (int16_t*)Region2;
		for(DWORD SampleIndex = 0;
			SampleIndex < Region2SampleCount;
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++; // writes the samples from SourceSample to DestSample
			*DestSample++ = *SourceSample++;
			++(SoundOutput->runningSampleIndex);
		}

		GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}


internal void Win32ProcessKeyboardMessage(game_button_state* State, bool IsDown)
{
	Assert(State->EndedDown != (int)IsDown);
	State->EndedDown = IsDown;
	++State->HalfTransitionCount;
}

internal void Win32ProcessPendingMessages(game_controller_input* KeyboardController)
{
	// flush the queue of Messages from windows in this loop
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		if(Message.message == WM_QUIT)
		{
			IsRunning = false;
		}

		switch(Message.message)
		{
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32_t VKCode = (uint32_t)Message.wParam;
				bool WasDown = ((Message.lParam & (1 << 30)) != 0); // Bit #30 of the LParam tells us what the previous key was
				bool IsDown = ((Message.lParam & (1 << 31)) == 0); // Bit #31 of the LParam tells us what the current key is
				if(WasDown != IsDown)
				{
					if(VKCode == VK_UP)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Up, IsDown);
					}
					else if (VKCode == VK_DOWN)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Down, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Left, IsDown);
					}
					else if (VKCode == VK_RIGHT || VKCode == 'd')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Right, IsDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
					}
					else if (VKCode == VK_SPACE)
					{

					}
				}

				bool AltKeyDown = ((Message.lParam & (1 << 29)) != 0);
				if((VKCode == VK_F4) && AltKeyDown)
				{
					IsRunning = false;
				}
			} break;
			default: {
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			} break;

		}
	}
}

void DisplayLastWin32Error()
{
	DWORD ErrorCode = GetLastError();
	char ErrorCodeBuffer[256];
	wsprintf(ErrorCodeBuffer, "VirtualAlloc error code: %d\n", ErrorCode);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	int64_t PerfCountFrequency = PerfCounterFrequencyResult.QuadPart;

	WNDCLASSA WindowClass = {};
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32_MainWindowCallback;
	WindowClass.hInstance = instance;
	WindowClass.lpszClassName = "Fucking A!";

	Win32_ResizeDIBSection(&GlobalBackBuffer, 1200, 700);

	if(RegisterClass(&WindowClass))
	{
		HWND window = CreateWindowEx(
								0,
								WindowClass.lpszClassName,
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
			IsRunning = true;

			win32_sound_output SoundOutput = {};
		
			SoundOutput.samplesPerSecond = 48000;
			SoundOutput.runningSampleIndex = 0; // unsigned because we want this to loop back to zero once it hits its max
			SoundOutput.bytesPerSample = sizeof(int16_t) * 2;
			SoundOutput.soundBufferSize = SoundOutput.samplesPerSecond * SoundOutput.bytesPerSample;
			SoundOutput.latencySampleCount = SoundOutput.samplesPerSecond / 15;

			Win32_InitDirectSound(window, SoundOutput.samplesPerSecond, SoundOutput.soundBufferSize);
			Win32ClearBuffer(&SoundOutput);
			GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			int64_t LastCycleCount = __rdtsc();

			bool SuccessfulMemoryAllocation = true;
			int16_t* samples = (int16_t*)VirtualAlloc(0, SoundOutput.soundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(samples == NULL)
			{
				SuccessfulMemoryAllocation = false;
				DisplayLastWin32Error();
			}

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes((uint64_t)2);
#else
			LPVOID BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSpace = Megabytes(64);
			GameMemory.TransientStorageSpace = Gigabytes((uint64_t)4);

			uint64_t TotalStorageSpace = GameMemory.PermanentStorageSpace + GameMemory.TransientStorageSpace;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalStorageSpace, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(GameMemory.PermanentStorage == NULL)
			{
				SuccessfulMemoryAllocation = false;
				DisplayLastWin32Error();
			}

			GameMemory.TransientStorage = (uint8_t*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSpace;

			game_sound_output_buffer soundBuffer = {};
			soundBuffer.samplesPerSecond = SoundOutput.samplesPerSecond;
			soundBuffer.samples = samples;

			game_input Input[2] = {};
			game_input* NewController = &Input[0];
			game_input* OldController = &Input[1];

			while(SuccessfulMemoryAllocation && IsRunning)
			{
				game_controller_input* NewKeyboardController = &NewController->Controllers[0];
				game_controller_input* OldKeyboardController = &OldController->Controllers[0];
				game_controller_input ZeroController = {};
				*NewKeyboardController = ZeroController;
				for(int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
				{
					NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
				}

				Win32ProcessPendingMessages(NewKeyboardController);

				// After processing all inputs, point the old controller at the new controller
				// so 'new' state becomes 'old' state for the start of the next loop
				*OldController = *NewController;

				// Direct sound output test
				// Lock direct sound buffer
				DWORD ByteToLock = 0;
				DWORD TargetCursor = 0;
				DWORD BytesToWrite = 0;
				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0;
				bool soundIsValid = false;
				if(SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					ByteToLock = (SoundOutput.runningSampleIndex * SoundOutput.bytesPerSample) % SoundOutput.soundBufferSize;
					TargetCursor = ((PlayCursor +
						(SoundOutput.latencySampleCount * SoundOutput.bytesPerSample)) %
						SoundOutput.soundBufferSize);
					if(ByteToLock > TargetCursor)	// write cursor is ahead of the play cursor in the circular buffer.
													// So we want to write from the write cursor to the end of the buffer, plus from the start of the buffer to the play cursor.
					{
						BytesToWrite = SoundOutput.soundBufferSize - ByteToLock;
						BytesToWrite += TargetCursor;
					}
					else // play cursor is ahead of the write cursor, so we want to write from write cursor to the play cursor
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					soundIsValid = true;
				}

				soundBuffer.sampleCount = BytesToWrite / SoundOutput.bytesPerSample;
				GameUpdateAndRender(&GameMemory, NewController, &GlobalBackBuffer, &soundBuffer);

				if(soundIsValid)
				{
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &soundBuffer);
				}

				win32_window_dimension WindowSize = Win32_GetWindowDimension(window);
				HDC DeviceContext = GetDC(window);
				Win32_DisplayBufferInWindow(DeviceContext, WindowSize.width, WindowSize.height, &GlobalBackBuffer);
				ReleaseDC(window, DeviceContext);

				int64_t EndCycleCount = __rdtsc();
				int64_t CyclesElapsed = EndCycleCount - LastCycleCount;
				int32_t MegaCyclesPerFrame = (int32_t)(CyclesElapsed / (1000 * 1000));

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);
				int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				int32_t msPerFrame = (int32_t)((1000 * CounterElapsed) / PerfCountFrequency);

				char Buffer[256];
				wsprintf(Buffer, "ms/frame: %d, mega cycles/frame: %d\n", msPerFrame, MegaCyclesPerFrame);
				OutputDebugStringA(Buffer);

				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;
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
