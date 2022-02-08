#define _UNICODE
#define UNICODE

#include<windows.h>
#include<stdint.h>
#include<Xinput.h>

#ifndef global_variable
#define global_variable static
#endif

#ifndef local_persistent
#define local_persistent static
#endif

#ifndef internal
#define internal static
#endif

struct win32_offscreen_buffer{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int BytesPerPixel = 4;
	int Pitch;
	int MemorySize;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

internal win32_window_dimension get_window_dimension(HWND Window)
{
	win32_window_dimension wd = {};
	RECT clientRect;
	GetClientRect(Window,&clientRect);
	wd.Width = clientRect.right - clientRect.left;
	wd.Height = clientRect.bottom - clientRect.top;
	return wd;
}

global_variable win32_offscreen_buffer global_back_buffer;
global_variable bool Running = true;

using uint8 = uint8_t;
using uint32 = uint32_t;

//Stubs for XInput State Windows functions.  This allows use program to not have to use xbox controllers when it isn't available.  Input can 
//be taken from the keyboard

#define X_INPUT_GET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32XLoadInput()
{
    auto XInputLdLbry = LoadLibrary(L"x_input1_4.dll");
    if(!XInputLdLbry)
        XInputLdLbry =  LoadLibrary(L"x_input1_3.dll");

    XInputGetState = reinterpret_cast<x_input_get_state*>(GetProcAddress(XInputLdLbry,"XInputGetState"));
    XInputSetState = reinterpret_cast<x_input_set_state*>(GetProcAddress(XInputLdLbry,"XInputSetState"));
    
}

internal void
RenderWeirdGradient(win32_offscreen_buffer buffer, int BlueOffset, int GreenOffset)
{
	auto Row = reinterpret_cast<uint8*>(buffer.Memory);
	for(int y=0;y<buffer.Height;++y)
	{
		auto Pixel = reinterpret_cast<uint32*>(Row);
		for(int x = 0; x < buffer.Width; ++x)
		{
			//Pixel in memory:  xx RR GG BB
			uint32 Blue = x+BlueOffset, Green = y+GreenOffset;
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += buffer.Pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer& buffer,int width, int height)
{
    //TODO Bulletproof this
    //Maybe won't free first, free after, then free first if that fails
    if(buffer.Memory){
        VirtualFree(buffer.Memory,0,MEM_RELEASE);
    }

    buffer.Width = width;
    buffer.Height = height;

    buffer.Info.bmiHeader.biSize = sizeof(buffer.Info.bmiHeader);
    buffer.Info.bmiHeader.biWidth = buffer.Width;
    buffer.Info.bmiHeader.biHeight = -buffer.Height;
    buffer.Info.bmiHeader.biPlanes = 1;
    buffer.Info.bmiHeader.biBitCount = 32;
    buffer.Info.bmiHeader.biCompression = BI_RGB;

    buffer.MemorySize = (buffer.Width*buffer.Height)*buffer.BytesPerPixel;
    buffer.Memory = VirtualAlloc(NULL,buffer.MemorySize,MEM_COMMIT,PAGE_READWRITE);
    buffer.Pitch = width*buffer.BytesPerPixel;
}

internal void Win32CopyBufferToWindow(HDC deviceContext,int WindowWidth,int WindowHeight,
									  win32_offscreen_buffer buffer)
{
    StretchDIBits
    (
        deviceContext,
        0,
        0,
		WindowWidth,
        WindowHeight,
        0,
        0,
        buffer.Width,
        buffer.Height,
        buffer.Memory,
        &buffer.Info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK WindowProc(HWND Window,
                            UINT Message,
                            WPARAM wParam,
                            LPARAM lParam)
{
    LRESULT result = 0;
    HDC deviceContext;
    PAINTSTRUCT paintStruct;
    const long keyDownBit = (1 << 30);
    const long keyUpBit = (1 << 31);
    switch(Message)
    {
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        break;
        case WM_CLOSE:
        {
            Running = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 vkCode = wParam;
            bool wasDown = ((lParam & keyDownBit) != 0);
            bool isDown = ((lParam & keyUpBit) != 0);
            if(wasDown != isDown)
            {
                switch(vkCode)
                {
                    case 'W':
                    {}break;
                    case 'A':
                    {}break;
                    case 'S':
                    {}break;
                    case 'D':
                    {}break;
                    case 'Q':
                    {}break;
                    case 'E':
                    {}break;
                    case VK_UP:
                    {

                    }
                    break;
                    case VK_DOWN:
                    {

                    }
                    break;
                    case VK_LEFT:
                    {

                    }
                    break;
                    case VK_RIGHT:
                    {

                    }
                    break;
                    case VK_ESCAPE:
                    {

                    }
                    break;
                }
            }
            auto altKeyDownBit = (1 << 29);
            bool altKeyDown = ((lParam & altKeyDownBit) != 0);
            if(vkCode == VK_F4 && altKeyDown)
                Running = false;

        }break;
        case WM_PAINT:
        {
            deviceContext= BeginPaint(Window,  &paintStruct);
            win32_window_dimension wd = get_window_dimension(Window);
            Win32CopyBufferToWindow(deviceContext,wd.Width, wd.Height,global_back_buffer);

            EndPaint(Window,&paintStruct);
        }break;
        default:
        {
            OutputDebugStringA("default\n");
            result = DefWindowProc(Window,Message,wParam,lParam);
        } break;
    }

    return result;
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(global_back_buffer,1280,720);

    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = &WindowProc;
    WindowClass.hInstance = hInstance;
    LPCWSTR className = L"HandmadeHeroWindowClass";
    WindowClass.lpszClassName = className;
    LPCWSTR windowName = L"Handmade Hero";

    if(RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(
            0,
            className,
            windowName,
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0
        );

        int xOffset = 0, yOffset = 0;
        if(WindowHandle != NULL)
        {
            MSG Message;
            while(Running)
            {
                while(PeekMessage(&Message,WindowHandle,0,0,PM_REMOVE))
                {
					if(Message.message == WM_QUIT)
						Running = false;

					TranslateMessage(&Message);
					DispatchMessage(&Message);
                }

                //get controller input
                for(DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT;++controllerIndex)
                {
                    XINPUT_STATE ControllerState;
                    if(XInputGetState(controllerIndex,&ControllerState) == ERROR_SUCCESS)
                    {
                        PXINPUT_GAMEPAD Pad = &ControllerState.Gamepad;
                        bool dPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool dPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool dPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool dPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool gPadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool gPadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);	
                        bool gPadLftShldr = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);	
                        bool gPadRghtShldr = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);	
                        bool gPadButtonA = (Pad->wButtons & XINPUT_GAMEPAD_A); 
                        bool gPadButtonB = (Pad->wButtons & XINPUT_GAMEPAD_B);	
                        bool gPadButtonX = (Pad->wButtons & XINPUT_GAMEPAD_X);	
                        bool dPadButtonY = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16_t StickX = Pad->sThumbLX;
                        int16_t StickY = Pad->sThumbRX;
                    }
                }

				RenderWeirdGradient(global_back_buffer,xOffset++,yOffset++);
				HDC deviceContext = GetDC(WindowHandle);

				win32_window_dimension wd = get_window_dimension(WindowHandle);
				Win32CopyBufferToWindow(deviceContext, wd.Width,wd.Height,
										global_back_buffer);
				ReleaseDC(WindowHandle,deviceContext);
            }
        }
    }
    return 0;
}
