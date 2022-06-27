module;
#include <windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <cstdint>
#include <cstdio>
#include "handmade_misc.h"
import platform_layer;
import handmade;
export module win32_platform_layer;

#define X_INPUT_GET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


export 
{
    void Win32XLoadInput()
    {
        auto XInputLdLbry = LoadLibraryA("XInput1_4.dll");
        if(!XInputLdLbry)
            XInputLdLbry =  LoadLibraryA("XInput1_3.dll");

        XInputGetState = reinterpret_cast<x_input_get_state*>(GetProcAddress(XInputLdLbry,"XInputGetState"));
        XInputSetState = reinterpret_cast<x_input_set_state*>(GetProcAddress(XInputLdLbry,"XInputSetState"));
    }
    
    class windows_layer : public platform_layer 
    {
        public:
            game_file_data debug_platform_read_entire_file(char* filename)
            {
                LPCSTR                lpFileName = filename;
                DWORD                 dwDesiredAccess = GENERIC_READ;
                DWORD                 dwShareMode = FILE_SHARE_READ;
                LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;
                DWORD                 dwCreationDisposition = OPEN_EXISTING;
                DWORD                 dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
                HANDLE                hTemplateFile = NULL;
                void* result = NULL;
                game_file_data file_data = {};
                DWORD bytes_read;

                auto file_handle = CreateFileA(
                    lpFileName,
                    dwDesiredAccess,
                    dwShareMode,
                    lpSecurityAttributes,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    hTemplateFile
                );

                if(file_handle != INVALID_HANDLE_VALUE)
                {
                    LARGE_INTEGER file_size;
                    if(GetFileSizeEx(file_handle,&file_size))
                    {
                        uint32 file_size_32 = safe_truncate_uint_64(file_size.QuadPart);
                        result = VirtualAlloc(NULL, file_size_32, MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
                        if(result)
                        {
                            if(ReadFile(file_handle, result, file_size_32, &bytes_read, NULL) &&
                                file_size_32 == bytes_read)
                            {
                                    file_data.memory = result;
                                    file_data.file_size = bytes_read;
                            }
                            else
                            {
                                //add logging
                                debug_platform_free_file_memory(result);
                                result = NULL;
                            }
                        }
                        else
                        {
                            //add logging
                        }
                        CloseHandle(file_handle);
                    }
                    else
                    {
                        //add logging
                    }
                }
                else
                {
                    //add logging
                }

                return file_data;
            }

            void debug_platform_free_file_memory(void *memory)
            {
                if(memory)
                {
                    VirtualFree(memory, 0, MEM_RELEASE);
                }
            }

            bool debug_platform_write_entire_file(char* filename, int memory_size, void *memory)
            {
                LPCSTR                lpFileName = filename;
                DWORD                 dwDesiredAccess = GENERIC_WRITE;
                DWORD                 dwShareMode = 0;
                LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;
                DWORD                 dwCreationDisposition = CREATE_ALWAYS;
                DWORD                 dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
                HANDLE                hTemplateFile = NULL;
                bool result = false;

                auto file_handle = CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

                if(file_handle != INVALID_HANDLE_VALUE)
                {
                    LPCVOID      lpBuffer = memory;
                    DWORD        nNumberOfBytesToWrite = memory_size;
                    DWORD        lpNumberOfBytesWritten;
                    LPOVERLAPPED lpOverlapped = NULL;

                    auto error_flag = WriteFile (file_handle, lpBuffer, nNumberOfBytesToWrite, &lpNumberOfBytesWritten, lpOverlapped);

                    if(error_flag && nNumberOfBytesToWrite == lpNumberOfBytesWritten)
                    {
                        result = true;
                    }
                    else
                    {
                        //log error
                    }
                    CloseHandle(file_handle);
                }
                else
                {
                    //handle error
                }

                return result;
            }
    };

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

    struct win32_sound_output
    {
        int32 samples_per_second = 48000;
        int32 bytes_per_sample = sizeof(int16_t)*2;
        uint32 running_sample_index = 0;
        uint32 buffer_size = samples_per_second * bytes_per_sample;
        real32 tSine = 0;
        int latency_sample_count;
    };

    const real32 Pi32 = 3.14159265359;

    win32_window_dimension get_window_dimension(HWND Window)
    {
        win32_window_dimension wd = {};
        RECT clientRect;
        GetClientRect(Window,&clientRect);
        wd.Width = clientRect.right - clientRect.left;
        wd.Height = clientRect.bottom - clientRect.top;
        return wd;
    }

    LPDIRECTSOUNDBUFFER Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
    {
        auto DirectSoundLibrary = LoadLibraryA("dsound.dll");
        LPDIRECTSOUNDBUFFER SecondaryBuffer = NULL;
        if(DirectSoundLibrary)
        {
            LPDIRECTSOUND direct_sound;
            if(auto direct_sound_create = DirectSoundCreate(NULL,&direct_sound,NULL); SUCCEEDED(direct_sound_create))
            {
                
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

    void Win32ResizeDIBSection(win32_offscreen_buffer& buffer,int width, int height)
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

    void Win32CopyBufferToWindow(HDC deviceContext,int WindowWidth,int WindowHeight, win32_offscreen_buffer buffer)
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

    void Win32ClearSoundBuffer(LPDIRECTSOUNDBUFFER sound_buffer,win32_sound_output &sound_output)
    {
        LPVOID region_1, region_2;
        DWORD region_1_size, region_2_size;

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

    void Win32ProcessXInputDigitalButton(game_button_state& old_state, game_button_state& new_state, DWORD button_state, DWORD button_bit)
    {
        new_state.ended_down = (button_state & button_bit) == button_bit;
        new_state.half_transition_count = old_state.ended_down != new_state.ended_down ? 1 : 0;
    }

    void Win32ProcessKeyboardInput(game_button_state& new_state, bool is_down)
    {
        new_state.ended_down = is_down;
        ++new_state.half_transition_count;
    }

    void Win32ProcessKeyboardMessages(game_controller_input& keyboard_controller, HWND WindowHandle, bool &Running)
    {
        MSG message;
        const long keyDownBit = (1 << 30);
        const long keyUpBit = (1 << 31);

        while(PeekMessage(&message,WindowHandle,0,0,PM_REMOVE))
        {
            if(message.message == WM_QUIT)
                Running = false;
            
            switch(message.message)
            {
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP:
                {
                    uint32 vkCode = static_cast<uint32>(message.wParam);
                    bool wasDown = ((message.lParam & keyDownBit) != 0);
                    bool isDown = ((message.lParam & keyUpBit) == 0);
                    if(wasDown != isDown)
                    {
                        switch(vkCode)
                        {
                            case 'W':
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.move_up, isDown);
                            }break;
                            case 'A':
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.move_left, isDown);
                            }break;
                            case 'S':
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.move_right, isDown);
                            }break;
                            case 'D':
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.move_down, isDown);
                            }break;
                            case 'Q':
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.left_shoulder, isDown);
                            }break;
                            case 'E':
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.right_shoulder, isDown);
                            }break;
                            case VK_UP:
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.action_up, isDown);
                            }
                            break;
                            case VK_DOWN:
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.action_down, isDown);
                            }
                            break;
                            case VK_LEFT:
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.action_left, isDown);
                            }
                            break;
                            case VK_RIGHT:
                            {
                                Win32ProcessKeyboardInput(keyboard_controller.action_right, isDown);
                            }
                            break;
                            case VK_ESCAPE:
                            {
                                Running = false;
                            }
                            break;
                        }
                    }
                    auto altKeyDownBit = (1 << 29);
                    bool altKeyDown = ((message.lParam & altKeyDownBit) != 0);
                    if(vkCode == VK_F4 && altKeyDown)
                        Running = false;

                }break;
                default:
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
            }
        }
    }

    real32 Win32ProcessXInputStickValue(SHORT value, SHORT dead_zone_threshold)
    {
        real32 result = 0;

        if(value < -dead_zone_threshold)
            result = static_cast<real32>(value) / 32768.0f;
        else if(value > dead_zone_threshold)
            result = static_cast<real32>(value) / 32767.0f;

        return result;
    }

    void Win32ProcessControllerMessages(game_input& old_input, game_input& new_input, DWORD max_controller_count, XINPUT_STATE& controller_state)
    {
        for(DWORD controllerIndex = 0; controllerIndex < max_controller_count;++controllerIndex)
        {
            DWORD our_controller_index = controllerIndex + 1;
            game_controller_input old_controller = *get_game_controller(old_input, our_controller_index);
            game_controller_input new_controller = *get_game_controller(new_input, our_controller_index);

            if(XInputGetState(controllerIndex,&controller_state) == ERROR_SUCCESS)
            {
                PXINPUT_GAMEPAD Pad = &controller_state.Gamepad;
                bool dPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                bool dPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                bool dPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                bool dPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                int16 StickX = Pad->sThumbLX;
                int16 LStickY = Pad->sThumbLY;
                int16 StickY = Pad->sThumbRX;


                new_controller.is_analog = true;
                new_controller.is_connected = true;

                real32 x = 0; 
                real32 y = 0;
                real32 threshold = 0.5f;

                new_controller.stick_average_x = Win32ProcessXInputStickValue(
                    StickX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                new_controller.stick_average_y = Win32ProcessXInputStickValue(
                    LStickY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                if(dPadUp)
                    new_controller.stick_average_y = 1.0f;
                if(dPadDown)
                    new_controller.stick_average_y -= 1.0f;
                if(dPadLeft)
                    new_controller.stick_average_x -= 1.0f;
                if(dPadRight)
                    new_controller.stick_average_y += 1.0f;
                
                

                Win32ProcessXInputDigitalButton(old_controller.move_left,
                                                new_controller.move_left,
                                                (new_controller.stick_average_x < -threshold) ? 1 : 0, 
                                                1);

                Win32ProcessXInputDigitalButton(old_controller.move_right,
                                                new_controller.move_right,
                                                (new_controller.stick_average_x > threshold) ? 1 : 0, 
                                                1);

                Win32ProcessXInputDigitalButton(old_controller.move_down,
                                                new_controller.move_down,
                                                (new_controller.stick_average_y < -threshold) ? 1 : 0, 
                                                1);


                Win32ProcessXInputDigitalButton(old_controller.move_up,
                                                new_controller.move_up,
                                                (new_controller.stick_average_y > threshold) ? 1 : 0, 
                                                1);

                Win32ProcessXInputDigitalButton(old_controller.action_down,
                                                new_controller.action_down,
                                                Pad->wButtons, 
                                                XINPUT_GAMEPAD_A);

                Win32ProcessXInputDigitalButton(old_controller.action_right,
                                                new_controller.action_right,
                                                Pad->wButtons, XINPUT_GAMEPAD_B);

                Win32ProcessXInputDigitalButton(old_controller.action_left,
                                                new_controller.action_left,
                                                Pad->wButtons, XINPUT_GAMEPAD_X);

                Win32ProcessXInputDigitalButton(old_controller.action_up,
                                                new_controller.action_up,
                                                Pad->wButtons, XINPUT_GAMEPAD_Y);

                Win32ProcessXInputDigitalButton(old_controller.left_shoulder,
                                                new_controller.left_shoulder,
                                                Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);

                Win32ProcessXInputDigitalButton(old_controller.right_shoulder,
                                                new_controller.right_shoulder,
                                                Pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);


                Win32ProcessXInputDigitalButton(old_controller.back,
                                                new_controller.back,
                                                Pad->wButtons, XINPUT_GAMEPAD_BACK);

                Win32ProcessXInputDigitalButton(old_controller.start,
                                                new_controller.start,
                                                Pad->wButtons, XINPUT_GAMEPAD_START);                                

                bool gPadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
                bool gPadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);	
            }
        }
    }
}



//Stubs for XInput State Windows functions.  This allows use program to not have to use xbox controllers when it isn't available.  Input can 
//be taken from the keyboard


