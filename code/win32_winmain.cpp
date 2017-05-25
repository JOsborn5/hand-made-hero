#include <windows.h>
#include <stdint.h>

#define internal static
#define global_variable static

global_variable bool isRunning = false;

// Stuff for our RGB display buffer
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable int bytesPerPixel = 4;

internal void
RenderWeirdGradient(int xOffset, int yOffset)
{
	int width = bitmapWidth;
	int height = bitmapHeight;
	int bitmapMemorySize = (width * height) * bytesPerPixel;
	bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	int pitch = width * bytesPerPixel;
	uint8_t *row = (uint8_t *) bitmapMemory;
	for(int Y = 0; Y < bitmapHeight; Y++)
	{
		uint8_t *pixel = (uint8_t *)row;
		for(int X = 0; X < bitmapWidth; X++)
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
			*pixel = (uint8_t)(bitmapWidth - X + xOffset);
			pixel++;

			// PADDING BYTE
			*pixel = 0;
			pixel++;
			
		}
		row += pitch;
	}
}


internal void
Win32_ResizeDIBSection(int width, int height)
{
	if(bitmapMemory)
	{
		VirtualFree(bitmapMemory, 0, MEM_RELEASE);
	}

	bitmapWidth = width;
	bitmapHeight = height;

	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = bitmapWidth;
	bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
}

internal void
Win32_updateWindow(HDC deviceContext, RECT *clientRect)
{
	int windowWidth = clientRect->right - clientRect->left;
	int windowHeight = clientRect->bottom - clientRect->top;
	StretchDIBits(deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, bitmapWidth, bitmapHeight,
		bitmapMemory,
		&bitmapInfo,
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
			RECT clientRect;
			GetClientRect(window, &clientRect);
			int width = clientRect.right - clientRect.left;
			int height = clientRect.bottom - clientRect.top;
			Win32_ResizeDIBSection(width, height);
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
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;

			Win32_updateWindow(deviceContext, &paint.rcPaint);

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

				RenderWeirdGradient(xOffset, yOffset);

				RECT clientRect;
				GetClientRect(window, &clientRect);
				PAINTSTRUCT paint;
				HDC deviceContext = GetDC(window);
				Win32_updateWindow(deviceContext, &clientRect);
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
