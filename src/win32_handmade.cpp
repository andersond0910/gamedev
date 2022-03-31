#include "win32_handmade.h"
#include<cstdio>
#include<algorithm>
#include<cmath>
#include "handmade.cpp"

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

internal void
Win32ClearSoundBuffer(LPDIRECTSOUNDBUFFER sound_buffer,win32_sound_output &sound_output)
{
    LPVOID region_1, region_2;
    DWORD region_1_size, region_2_size, bytes_to_write;

    if(
        SUCCEEDED(
            sound_buffer->Lock(
                0,sound_output.buffer_size, 
                &region_1,&region_1_size,
                &region_2,&region_2_size,0
            )
        )
    )
    {
        auto dest_sample = reinterpret_cast<uint8*>(region_1);
        
        for(DWORD sample_index = 0; sample_index < region_1_size; ++sample_index)
        {
            *dest_sample++ = 0;//write to left
        }
        
        dest_sample = reinterpret_cast<uint8*>(region_2);
        for(DWORD sample_index = 0; sample_index < region_2_size; ++sample_index)
        {
            *dest_sample++ = 0;//write to left
        }
        
        //unlock the secondary buffer
        sound_buffer->Unlock(region_1,region_1_size,region_2,region_2_size);
    }        
}

void Win32FillSoundBuffer(LPDIRECTSOUNDBUFFER sound_buffer, win32_sound_output &sound_output,
                          game_sound_output_buffer& source, DWORD byte_to_lock, DWORD bytes_to_write)
{
    DWORD write_cursor, play_cursor;
    LPVOID region_1, region_2;
    DWORD region_1_size, region_2_size;
    local_persistent bool sound_is_valid = false;

    if(SUCCEEDED(sound_buffer->GetCurrentPosition(&play_cursor,&write_cursor)))
        sound_is_valid = true;

    if(sound_is_valid)    
    {

        if(SUCCEEDED(sound_buffer->Lock(byte_to_lock,bytes_to_write, &region_1,&region_1_size,&region_2,&region_2_size,0)))
        {
            DWORD region_1_sample_count = region_1_size / sound_output.bytes_per_sample, region_2_sample_count = region_2_size / sound_output.bytes_per_sample;
            auto dest_sample = reinterpret_cast<int16*>(region_1);
            auto source_sample = source.samples;
            
            for(DWORD sample_index = 0; sample_index < region_1_sample_count; ++sample_index)
            {
                *dest_sample++ = *source_sample++;//write to left
                *dest_sample++ = *source_sample++;//write to right
                ++sound_output.running_sample_index; 
            }
            
            dest_sample = reinterpret_cast<int16*>(region_2);
            for(DWORD sample_index = 0; sample_index < region_2_sample_count; ++sample_index)
            {
                *dest_sample++ = *source_sample++;//write to left
                *dest_sample++ = *source_sample++;//write to right
                ++sound_output.running_sample_index; 
            }
            
            //unlock the secondary buffer
            sound_buffer->Unlock(region_1,region_1_size,region_2,region_2_size);
        }        
    }
}

internal void Win32ProcessXInputDigitalButton(game_button_state& old_state, game_button_state& new_state, DWORD button_state, DWORD button_bit)
{
    new_state.ended_down = (button_state & button_bit) == button_bit;
    new_state.half_transition_count = old_state.ended_down != new_state.ended_down ? 1 : 0;
}

template<typename T>
internal size_t array_count(T array[])
{
    return sizeof(*array) / sizeof(array[0]);
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
            bool sound_is_valid = false;
            
            //sound test variables and setup
            win32_sound_output win32_sound_config = {};
            auto direct_sound_buffer = Win32InitDSound(WindowHandle, win32_sound_config.samples_per_second, win32_sound_config.buffer_size);
            win32_sound_config.latency_sample_count = win32_sound_config.samples_per_second / 15;
            int16* samples = reinterpret_cast<int16*>(VirtualAlloc(NULL,win32_sound_config.buffer_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
            
            Win32ClearSoundBuffer(direct_sound_buffer,win32_sound_config);
            direct_sound_buffer->Play(0,0,DSBPLAY_LOOPING);
            
            //timing counters
            LARGE_INTEGER begin_counter, end_counter, performance_frequency;
            uint64 counter_elapsed, perf_count_frequency;
            real32 ms_per_frame, frames_per_second, million_cycles_per_frame;
            QueryPerformanceFrequency(&performance_frequency);
            perf_count_frequency = performance_frequency.QuadPart;
            uint64 begin_cycle_count, end_cycle_count, elapsed_cycles;

            //print buffer
            char buffer[256];

            //controller input
            game_input controller_input[2] = {}, \
                       old_input = controller_input[0], \
                       new_input = controller_input[1];

            int num_controllers = array_count(new_input.controllers);

            int max_controller_count = XUSER_MAX_COUNT > num_controllers ? num_controllers : XUSER_MAX_COUNT;

            //allocate game memory
            #ifndef HANDMADE_INTERNAL
                LPVOID base_address = 0;
            #else
                LPVOID base_address = (LPVOID) terabytes((uint64)2);
            #endif

            game_memory g_memory = {};
            g_memory.permanent_storage_size = megabytes(64);
            g_memory.transient_storage_size = gigabytes((uint64)4);
            
            uint64 total_memory = g_memory.permanent_storage_size + g_memory.transient_storage_size;
            g_memory.permanent_memory = VirtualAlloc(base_address,total_memory,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
            g_memory.transient_storage = reinterpret_cast<uint8*>(&g_memory.permanent_memory + g_memory.permanent_storage_size);
            
            if(samples && g_memory.permanent_memory && g_memory.transient_storage)
            {
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
                            int16 StickX = Pad->sThumbLX;
                            int16 StickY = Pad->sThumbRX;

                            game_controller_input old_controller = old_input.controllers[controllerIndex];
                            game_controller_input new_controller = new_input.controllers[controllerIndex];

                            new_controller.is_analog = true;
                            new_controller.start_x = old_controller.end_x;
                            new_controller.start_y = old_controller.end_y;

                            real32 x = StickX < 0 ? (real32)StickX / 32768.0f : (real32)StickX / 32767.0f;
                            real32 y = StickY < 0 ? (real32)StickY / 32768.0f : (real32)StickY / 32767.0f;

                            new_controller.min_x = new_controller.max_x = new_controller.end_x = x; 
                            new_controller.min_y = new_controller.max_y = new_controller.end_y = y; 

                            Win32ProcessXInputDigitalButton(old_controller.down,
                                                            new_controller.down,
                                                            Pad->wButtons, 
                                                            XINPUT_GAMEPAD_A);

                            Win32ProcessXInputDigitalButton(old_controller.right,
                                                            new_controller.right,
                                                            Pad->wButtons, XINPUT_GAMEPAD_B);

                            Win32ProcessXInputDigitalButton(old_controller.left,
                                                            new_controller.left,
                                                            Pad->wButtons, XINPUT_GAMEPAD_X);

                            Win32ProcessXInputDigitalButton(old_controller.up,
                                                            new_controller.up,
                                                            Pad->wButtons, XINPUT_GAMEPAD_Y);

                            Win32ProcessXInputDigitalButton(old_controller.left_shoulder,
                                                            new_controller.left_shoulder,
                                                            Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);

                            Win32ProcessXInputDigitalButton(old_controller.right_shoulder,
                                                            new_controller.right_shoulder,
                                                            Pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);

                            bool gPadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            bool gPadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);	
                        }
                    }

                    DWORD play_cursor, write_cursor, target_cursor;
                    DWORD bytes_to_write, byte_to_lock;
                    if(SUCCEEDED(direct_sound_buffer->GetCurrentPosition(&play_cursor,&write_cursor)))
                    {
                        byte_to_lock = ((win32_sound_config.running_sample_index*win32_sound_config.bytes_per_sample) % win32_sound_config.buffer_size);

                        target_cursor = ((play_cursor + (win32_sound_config.latency_sample_count*win32_sound_config.bytes_per_sample)) % win32_sound_config.buffer_size);

                        if(byte_to_lock > target_cursor)
                        {
                            bytes_to_write = (win32_sound_config.buffer_size - byte_to_lock);
                            bytes_to_write += target_cursor;
                        }
                        else
                            bytes_to_write = target_cursor - byte_to_lock;
                        sound_is_valid = true;
                    }

                    game_sound_output_buffer game_sound_buffer = {};
                    game_sound_buffer.samples_per_second = win32_sound_config.samples_per_second;
                    game_sound_buffer.sample_count_to_output = bytes_to_write / win32_sound_config.bytes_per_sample;
                    game_sound_buffer.samples = samples; 

                    game_offscreen_buffer game_display = {};
                    game_display.Height = global_back_buffer.Height;
                    game_display.Width = global_back_buffer.Width;
                    game_display.Memory = global_back_buffer.Memory;
                    game_display.Pitch  = global_back_buffer.Pitch;

                    GameUpdateAndRender(g_memory,new_input, game_display,game_sound_buffer);

                    //sound output test
                    Win32FillSoundBuffer(direct_sound_buffer,win32_sound_config, game_sound_buffer, byte_to_lock, bytes_to_write);

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
                    std::swap(new_input,old_input);
                }
                ReleaseDC(WindowHandle,deviceContext);
            }
        }
    }
    return 0;
}
