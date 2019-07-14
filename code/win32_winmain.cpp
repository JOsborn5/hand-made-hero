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
global_variable int64_t GlobalPerfCountFrequency;

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
					if(VKCode == 'W')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
					}
					else if(VKCode == 'S')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
					}
					else if(VKCode == 'A')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
					}
					else if(VKCode == 'D')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
					}
					else if(VKCode == 'Q')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if(VKCode == 'E')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
					}
					else if(VKCode == VK_UP)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
					}
					else if (VKCode == VK_DOWN)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
					}
					else if (VKCode == VK_RIGHT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
					}
					else if (VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
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

inline LARGE_INTEGER Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return Result;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	uint64_t CounterElapsed = End.QuadPart - Start.QuadPart;
	float SecondsElapsedForWork = ((float)CounterElapsed / (float)GlobalPerfCountFrequency);
	return SecondsElapsedForWork;
}

internal void Win32DebugDrawVertical(game_offscreen_buffer* VideoBuffer, int X, int Top, int Bottom, uint32_t Color)
{
	uint8_t* Pixel = (uint8_t*)VideoBuffer->Memory + (X * VideoBuffer->BytesPerPixel + (Top * VideoBuffer->Pitch));
	for(int Step = Top; Step < Bottom; Step++)
	{
		*(uint32_t*)Pixel = Color;
		Pixel += VideoBuffer->Pitch;
	}
}

internal void Win32DebugSyncDisplay(game_offscreen_buffer* VideoBuffer, int PlayCursorArrayCount, DWORD* LastPlayCursor, win32_sound_output* SoundOutput, float SecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;
	int Top = PadY;
	int Bottom = VideoBuffer->Height - PadY;

	float C = (float)(VideoBuffer->Width - (2 * PadX)) / (float)SoundOutput->soundBufferSize;
	for(int PlayCursorIndex = 0; PlayCursorIndex < PlayCursorArrayCount; PlayCursorIndex++)
	{
		DWORD ThisPlayCursor = LastPlayCursor[PlayCursorIndex];
		Assert(ThisPlayCursor < (DWORD)SoundOutput->soundBufferSize);
		float XReal32 = (C * ThisPlayCursor);
		int X = PadX + (int)XReal32;
		Win32DebugDrawVertical(VideoBuffer, X, Top, Bottom, 0xFFFFFFFF);
	}
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	GlobalPerfCountFrequency = PerfCounterFrequencyResult.QuadPart;

	// Set the Windows schedular granularity to 1ms to help our Sleep() function call be granular
	UINT DesiredSchedulerMS = 1;
	MMRESULT setSchedularGranularityResult = timeBeginPeriod(DesiredSchedulerMS);
	bool SleepIsGranular = (setSchedularGranularityResult == TIMERR_NOERROR);

	WNDCLASSA WindowClass = {};
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32_MainWindowCallback;
	WindowClass.hInstance = instance;
	WindowClass.lpszClassName = "Fucking A!";

	const int MonitorRefreshHz = 60;
	const int GameUpdateHz = 30;
	// int GameUpdateHz = MonitorRefreshHz / 2;
	float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;

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

			LARGE_INTEGER LastCounter = Win32GetWallClock();

			int DebugLastPlayCursorIndex = 0;
			DWORD DebugLastPlayCursor[GameUpdateHz / 2] = {0};

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
				game_controller_input* NewKeyboardController = GetGameController(NewController, 0);
				game_controller_input* OldKeyboardController = GetGameController(OldController, 0);
				*NewKeyboardController = {};
				for(int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
				{
					NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
				}

				Win32ProcessPendingMessages(NewKeyboardController);

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

				float WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
				float SecondsElapsedForFrame = WorkSecondsElapsed;
				if(SecondsElapsedForFrame < TargetSecondsPerFrame)
				{
					if(SleepIsGranular)
					{
						DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
						if (SleepMS > 0)
						{
							Sleep(SleepMS);
						}
					}

					while(SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
					}
				}
				else
				{
					// TODO MISSED FRAME RATE
				}

				LARGE_INTEGER EndCounter = Win32GetWallClock();
				float MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
				uint64_t CounterElapsed = LastCounter.QuadPart - EndCounter.QuadPart;
				LastCounter = EndCounter;

				win32_window_dimension WindowSize = Win32_GetWindowDimension(window);
				HDC DeviceContext = GetDC(window);
#if HANDMADE_INTERNAL
				Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugLastPlayCursor), DebugLastPlayCursor, &SoundOutput, TargetSecondsPerFrame);
#endif
				Win32_DisplayBufferInWindow(DeviceContext, WindowSize.width, WindowSize.height, &GlobalBackBuffer);

				ReleaseDC(window, DeviceContext);

#if HANDMADE_INTERNAL
				{
					DWORD DebugPlayCursor;
					DWORD DebugWriteCursor;
					GlobalSoundBuffer->GetCurrentPosition(&DebugPlayCursor, &DebugWriteCursor);
					DebugLastPlayCursor[DebugLastPlayCursorIndex++] = DebugPlayCursor;
					if(DebugLastPlayCursorIndex > ArrayCount(DebugLastPlayCursor))
					{
						DebugLastPlayCursorIndex = 0;
					}
				}
#endif

				// After processing all inputs, point the old controller at the new controller
				// so 'new' state becomes 'old' state for the start of the next loop
				game_input* Temp = NewController;
				NewController = OldController;
				OldController = Temp;

				int64_t EndCycleCount = __rdtsc();
				int64_t CyclesElapsed = EndCycleCount - LastCycleCount;
				LastCycleCount = EndCycleCount;

				double FPS = (double)GlobalPerfCountFrequency / (double)CounterElapsed;
				double MCPF = (double)CyclesElapsed / (1000.0f * 1000.0f);

				char Buffer[256];
				sprintf_s(Buffer, "%.02f ms/f,  %.02f f/s,  %.02f MC/f\n", MSPerFrame, FPS, MCPF);
				OutputDebugStringA(Buffer);
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
