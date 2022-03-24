#include<windows.h>
#include<stdint.h>
#include<Xinput.h>
#include<dsound.h>
#include<stdio.h>
#include<cmath>
#include "handmade.cpp"

#ifndef global_variable
#define global_variable static
#endif

#ifndef local_persistent
#define local_persistent static
#endif

#ifndef internal
#define internal static
#endif

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float real32;
typedef double real64;

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

const real32 Pi32 = 3.14159265359;

struct win32_sound_output
{
    int16 tone_volume = 3000;
    int32 samples_per_second = 48000;
    int32 bytes_per_sample = sizeof(int16)*2;
    int32 Hz = 256;
    uint32 wave_period = samples_per_second/Hz;
    uint32 running_sample_index = 0;
    uint32 buffer_size = samples_per_second * bytes_per_sample;
    real32 tSine = 0;
    int latency_sample_count;
};

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

internal LPDIRECTSOUNDBUFFER
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    auto DirectSoundLibrary = LoadLibraryA("dsound.dll");
    LPDIRECTSOUNDBUFFER SecondaryBuffer = NULL;
    if(DirectSoundLibrary)
    {
        LPDIRECTSOUND direct_sound;
        if(auto direct_sound_create = DirectSoundCreate(NULL,&direct_sound,NULL); SUCCEEDED(direct_sound_create)){
            
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
            WaveFormat.nChannels       = 2;
            WaveFormat.nSamplesPerSec  = SamplesPerSecond;
            WaveFormat.wBitsPerSample  = 16;
            WaveFormat.nBlockAlign     = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            
            if(SUCCEEDED(direct_sound->SetCooperativeLevel(Window,DSSCL_PRIORITY)))
            {
               DSBUFFERDESC BufferDescription = {}; 
               BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
               BufferDescription.dwSize = sizeof(BufferDescription);
               LPDIRECTSOUNDBUFFER PrimaryBuffer;
               auto result = direct_sound->CreateSoundBuffer(&BufferDescription,&PrimaryBuffer,NULL);
               if(SUCCEEDED(result))
               {
                   HRESULT bufferResult = PrimaryBuffer->SetFormat(&WaveFormat);
                   if(SUCCEEDED(bufferResult))
                   {
                       printf("Succeeded in creating primary buffer\n");
                   }
                   else
                   {
                     //TODO(): Diagnostic
                   }
               }
               else
               {
                 //TODO(): Diagnostic
               }
            }
            else
            {

            }

            DSBUFFERDESC BufferDescription = {}; 
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            BufferDescription.dwFlags = 0;
            if(SUCCEEDED(direct_sound->CreateSoundBuffer(&BufferDescription,&SecondaryBuffer,NULL)))
            {
                printf("Succeeded in creating secondary buffer\n");
            }
            else
            {
                //diagnostic
            }
        }
    }
    return SecondaryBuffer;
}

internal void
Win32XLoadInput()
{
    auto XInputLdLbry = LoadLibraryA("x_input1_4.dll");
    if(!XInputLdLbry)
        XInputLdLbry =  LoadLibraryA("x_input1_3.dll");

    XInputGetState = reinterpret_cast<x_input_get_state*>(GetProcAddress(XInputLdLbry,"XInputGetState"));
    XInputSetState = reinterpret_cast<x_input_set_state*>(GetProcAddress(XInputLdLbry,"XInputSetState"));
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
    buffer.Memory = VirtualAlloc(NULL,buffer.MemorySize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
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

void Win32WriteSineWaveToBuffer(LPDIRECTSOUNDBUFFER sound_buffer, win32_sound_output &sound_output)
{
    DWORD write_cursor, play_cursor;
    LPVOID region_1, region_2;
    DWORD region_1_size, region_2_size, bytes_to_write;

    if(SUCCEEDED(sound_buffer->GetCurrentPosition(&play_cursor,&write_cursor)))    
    {
        DWORD byte_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.buffer_size;
        DWORD target_cursor = (play_cursor + (sound_output.latency_sample_count*sound_output.bytes_per_sample)) % sound_output.buffer_size;
        if(byte_to_lock == target_cursor){}
        if(byte_to_lock > target_cursor)
        {
            bytes_to_write = sound_output.buffer_size - byte_to_lock;
            bytes_to_write += target_cursor;
        }
        else
        {
            bytes_to_write = target_cursor - byte_to_lock;
        }

        if(SUCCEEDED(sound_buffer->Lock(write_cursor,bytes_to_write, &region_1,&region_1_size,&region_2,&region_2_size,0)))
        {
            DWORD region_1_sample_count = region_1_size / sound_output.bytes_per_sample, region_2_sample_count = region_2_size / sound_output.bytes_per_sample;
            auto sample = reinterpret_cast<int16*>(region_1);
            
            for(DWORD sample_index = 0; sample_index < region_1_sample_count; ++sample_index)
            {
                real32 sine_value = sin(sound_output.tSine);
                int16 sample_value = (int16)(sine_value * sound_output.tone_volume);
                *sample++ = sample_value;//write to left
                *sample++ = sample_value;//write to right
                sound_output.tSine += (2.0f * Pi32* 1.0f) / (real32)sound_output.wave_period;
                ++sound_output.running_sample_index; 
            }
            
            sample = reinterpret_cast<int16*>(region_2);
            for(DWORD sample_index = 0; sample_index < region_2_sample_count; ++sample_index)
            {
                real32 sine_value = sin(sound_output.tSine);
                int16 sample_value = (int16)(sine_value * sound_output.tone_volume);
                *sample++ = sample_value;//write to left
                *sample++ = sample_value;//write to right
                sound_output.tSine += (2.0f * Pi32) / (real32)sound_output.wave_period;
                ++sound_output.running_sample_index; 
            }
            
            //unlock the secondary buffer
            sound_buffer->Unlock(region_1,region_1_size,region_2,region_2_size);
        }        
    }
}


int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    WNDCLASSA window_class = {};

	Win32ResizeDIBSection(global_back_buffer,1280,720);

    window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = &WindowProc;
    window_class.hInstance = hInstance;
    LPCSTR className = "HandmadeHeroWindowClass", window_name="Handmade Hero";
    window_class.lpszClassName = className;

    if(RegisterClassA(&window_class))
    {
        HWND WindowHandle = CreateWindowA(
            className,
            window_name,
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

        if(WindowHandle != NULL)
        {
            int xOffset = 0, yOffset = 0;
            
            MSG message;
            HDC deviceContext = GetDC(WindowHandle);
            XINPUT_STATE ControllerState;
            
            //sound test variables and setup
            win32_sound_output sound_output = {};
            auto sound_buffer = Win32InitDSound(WindowHandle, sound_output.samples_per_second, sound_output.buffer_size);
            sound_output.latency_sample_count = sound_output.samples_per_second / 15;
            
            Win32WriteSineWaveToBuffer(sound_buffer, sound_output);
            sound_buffer->Play(0,0,DSBPLAY_LOOPING);

            //timing counters
            LARGE_INTEGER begin_counter, end_counter, performance_frequency;
            uint64 counter_elapsed, perf_count_frequency;
            real32 ms_per_frame, frames_per_second, million_cycles_per_frame;
            QueryPerformanceFrequency(&performance_frequency);
            perf_count_frequency = performance_frequency.QuadPart;
            uint64 begin_cycle_count, end_cycle_count, elapsed_cycles;

            //print buffer
            char buffer[256];

            while(Running)
            {
                QueryPerformanceCounter(&begin_counter);
                begin_cycle_count = __rdtsc();
                while(PeekMessage(&message,WindowHandle,0,0,PM_REMOVE))
                {
					if(message.message == WM_QUIT)
						Running = false;

					TranslateMessage(&message);
					DispatchMessage(&message);
                }

                //get controller input
                for(DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT;++controllerIndex)
                {
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

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbRX;

                        //joke match sound to xbox controller stick
                        sound_output.Hz = 512 + (int32)(256.0f*((real32)StickY / 30000.0f));
                        sound_output.wave_period = sound_output.samples_per_second/sound_output.Hz;
                    }
                }

                game_offscreen_buffer off_buffer = {};
                off_buffer.Height = global_back_buffer.Height;
                off_buffer.Width = global_back_buffer.Width;
                off_buffer.Memory = global_back_buffer.Memory;
                off_buffer.Pitch  = global_back_buffer.Pitch;

				GameUpdateAndRender(off_buffer, xOffset, yOffset);
                
                //sound output test
                //Win32WriteSquareWaveToBuffer(sound_buffer,buffer_size, running_sample_index);
                Win32WriteSineWaveToBuffer(sound_buffer,sound_output);

                win32_window_dimension wd = get_window_dimension(WindowHandle);
                Win32CopyBufferToWindow(deviceContext, wd.Width,wd.Height,global_back_buffer);

                //get frame timings
				end_cycle_count = __rdtsc();
                elapsed_cycles = end_cycle_count - begin_cycle_count;
                QueryPerformanceCounter(&end_counter);

                counter_elapsed = end_counter.QuadPart - begin_counter.QuadPart;
                ms_per_frame = static_cast<real32>((1000*counter_elapsed) / (perf_count_frequency*1.0f));
                frames_per_second = static_cast<real32>(perf_count_frequency / (1.0f * counter_elapsed));
                million_cycles_per_frame = static_cast<real32>(elapsed_cycles / 1'000'000.0f);

/*
                sprintf(buffer,
                        "Milliseconds/frame: %.02f, FPS: %.02f, million cycles per frame: %.02f",
                        ms_per_frame, 
                        frames_per_second,
                        million_cycles_per_frame);
            
                printf("%s\n",buffer);*/
            }
			ReleaseDC(WindowHandle,deviceContext);
        }
    }
    return 0;
}
