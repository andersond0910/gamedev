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

export 
{
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

                    if(error_flag && memory_size == lpNumberOfBytesWritten)
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
}

//Stubs for XInput State Windows functions.  This allows use program to not have to use xbox controllers when it isn't available.  Input can 
//be taken from the keyboard


