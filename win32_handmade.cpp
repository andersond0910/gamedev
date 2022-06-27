#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <cstdint>
#include <cstdio>
#include <algorithm> 
#include "handmade_misc.h"
#include <cmath>
import handmade;
//import platform_layer;
import handmade_hero;
import win32_platform_layer;

global_variable win32_offscreen_buffer global_back_buffer;
global_variable bool Running = true;

LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	HDC deviceContext;
	PAINTSTRUCT paintStruct;

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
            NULL
        );

        if(WindowHandle != NULL)
        {            
            MSG message;
            HDC deviceContext = GetDC(WindowHandle);
            XINPUT_STATE ControllerState;
            bool sound_is_valid = false;
			windows_layer windows_platform;
            
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
            Win32XLoadInput();
            game_input old_input = {}, new_input = {}, temp;
			DWORD our_controller_index;

            int num_controllers = array_count(new_input.controllers);

            DWORD max_controller_count = XUSER_MAX_COUNT > num_controllers ? num_controllers : XUSER_MAX_COUNT;

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
					game_controller_input *old_keyboard_controller = get_game_controller(old_input, 0);
					game_controller_input *new_keyboard_controller = get_game_controller(new_input, 0);
                    
                    game_controller_input zero_controller = {};
					*new_keyboard_controller = zero_controller;
                    new_keyboard_controller->is_connected = true;

                    for(int ButtonIndex = 0; ButtonIndex < array_count(old_keyboard_controller->buttons); ++ButtonIndex)
                    {
                        new_keyboard_controller->buttons[ButtonIndex].ended_down = old_keyboard_controller->buttons[ButtonIndex].ended_down;
                    }

                    QueryPerformanceCounter(&begin_counter);
                    begin_cycle_count = __rdtsc();

					Win32ProcessKeyboardMessages(*new_keyboard_controller, WindowHandle, Running);

                    //get controller input
                    Win32ProcessControllerMessages(old_input, new_input, max_controller_count, ControllerState);

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

                    GameUpdateAndRender(g_memory,new_input, game_display,game_sound_buffer, windows_platform);

                    //sound output test
                    if(sound_is_valid)
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

                    #ifdef HANDMADE_SLOW
                        sprintf(buffer,
                                "Milliseconds/frame: %.02f, FPS: %.02f, million cycles per frame: %.02f",
                                ms_per_frame, 
                                frames_per_second,
                                million_cycles_per_frame);
                    
                        printf("%s\n",buffer);
                    #endif
                    std::swap(new_input,old_input);
                }
                ReleaseDC(WindowHandle,deviceContext);
            }
        }
    }
    return 0;
}
