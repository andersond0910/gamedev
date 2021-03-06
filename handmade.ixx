module;
#include<cstdint>
#include "handmade_misc.h"
export module handmade;

export 
{

    template<typename T >
    constexpr T kilobytes(T num_bytes)
    {
        return num_bytes * 1024;
    }

    template<typename T >
    constexpr T megabytes(T num_bytes)
    {
        return kilobytes(num_bytes) * 1024;
    }

    template<typename T >
    constexpr T gigabytes(T num_bytes)
    {
        return megabytes(num_bytes) * 1024;
    }

    template<typename T >
    constexpr T terabytes(T num_bytes)
    {
        return gigabytes(num_bytes) * 1024;
    }

    struct game_sound_output_buffer
    {
        int sample_count_to_output;
        int samples_per_second;
        int16_t *samples;
    };

    struct game_button_state
    {
        int half_transition_count;
        bool ended_down;
    };

    struct game_controller_input
    {
        bool is_connected = false;
        bool is_analog;
        real32 stick_average_x;
        real32 stick_average_y;

        union
        {
            game_button_state buttons[12];
            struct
            {
                game_button_state move_up;
                game_button_state move_down;
                game_button_state move_left;
                game_button_state move_right;
                
                game_button_state action_up;
                game_button_state action_down;
                game_button_state action_left;
                game_button_state action_right;
                
                game_button_state left_shoulder;
                game_button_state right_shoulder;

                game_button_state back;
                game_button_state start;
            };
        };
    };

    struct game_input
    {
        //TODO - insert clock value here
        game_controller_input controllers[5];
    };

    struct game_memory
    {
        bool is_initialized;
        uint64 permanent_storage_size;
        uint64 transient_storage_size;
        void* permanent_memory;
        void* transient_storage;
    };

    struct game_state
    {
        int blue_offset;
        int green_offset = 0; 
        int tone_hz = 256;
    };

    uint32 safe_truncate_uint_64(uint64 file_size)
    {
        Assert(file_size <= 0xFFFFFFFF);
        auto result = static_cast<uint32>(file_size);
        return result;
    }

    /*
        services that the game provides to the platform layer
    */
    struct game_offscreen_buffer
    {
        void* Memory;
        int Width;
        int Height;
        int Pitch;
    };

    game_controller_input* get_game_controller(game_input& input, int index)
    {
        Assert(index < array_count(input.controllers));
        return &input.controllers[index];
    }
}