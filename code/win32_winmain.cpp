#include <windows.h>
#include <stdint.h>

#define internal static
#define global_variable static

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

global_variable bool isRunning = false;
global_variable win32_offscreen_buffer globalBackBuffer;




internal void
RenderWeirdGradient(win32_offscreen_buffer buffer, int xOffset, int yOffset)
{
	uint8_t *row = (uint8_t *) buffer.memory;
	for(int Y = 0; Y < buffer.height; Y++)
	{
		uint8_t *pixel = (uint8_t *)row;
		for(int X = 0; X < buffer.width; X++)
		{
			/*
				pixel in memory: BB GG RR xx
				littel endian architecture
				0x xxBBGGRR
			*/
			
			//BLUE
			*pixel = (uint8_t)(X + xOffset); // using * because we're writing to the memory location stored in the pixel variabe
			pixel++; // not using * because we're moving the pointer (working out where the memory location is for the next pixel)

			//GREEN
			*pixel = (uint8_t)(Y - yOffset);
			pixel++;

			//RED
			*pixel = (uint8_t)(buffer.width - X + xOffset);
			pixel++;

			// PADDING BYTE
			*pixel = 0;
			pixel++;
			
		}
		row += buffer.pitch;
	}
}


internal void
// Note this changes the buffer so we're passing the buffer as a pointer so this function will update it
Win32_ResizeDIBSection(win32_offscreen_buffer *buffer, win32_window_dimension window)
{
	if(buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = window.width;
	buffer->height = window.height;

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

win32_window_dimension
Win32_GetWindowDimension(HWND window)
{
	win32_window_dimension windowDim;

	RECT clientRect;
	GetClientRect(window, &clientRect);
	
	windowDim.width = clientRect.right - clientRect.left;
	windowDim.height = clientRect.bottom - clientRect.top;

	return windowDim;
}

internal void
Win32_DisplayBufferInWindow(HDC deviceContext, win32_window_dimension window, win32_offscreen_buffer buffer)
{
	StretchDIBits(deviceContext,
		0, 0, window.width, window.height,
		0, 0, buffer.width, buffer.height,
		buffer.memory,
		&buffer.info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK
Win32_MainWindowCallback(HWND window,
						UINT message,
						WPARAM wParam,
						LPARAM lParam)
{
	LRESULT result = -1;

	switch(message)
	{
		case WM_SIZE:
		{
			win32_window_dimension windowSize = Win32_GetWindowDimension(window);
			Win32_ResizeDIBSection(&globalBackBuffer, windowSize);
		} break;
		case WM_DESTROY:
		{
			isRunning = false;
		} break;
		case WM_CLOSE:
		{
			isRunning = false;
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(window, &paint);
			win32_window_dimension windowSize = Win32_GetWindowDimension(window);
			Win32_DisplayBufferInWindow(deviceContext, windowSize, globalBackBuffer);
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

int CALLBACK
WinMain(HINSTANCE instance,
		HINSTANCE prevInstance,
		LPSTR commandLine,
		int showCode)
{
	WNDCLASS windowClass = {};
	windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	windowClass.lpfnWndProc = Win32_MainWindowCallback;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = "Fucking A!";

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
			int xOffset = 0;
			int yOffset = 0;
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

				RenderWeirdGradient(globalBackBuffer, xOffset, yOffset);

				win32_window_dimension windowSize = Win32_GetWindowDimension(window);
				HDC deviceContext = GetDC(window);
				Win32_DisplayBufferInWindow(deviceContext, windowSize, globalBackBuffer);
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
