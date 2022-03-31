#pragma once

#ifdef HANDMADE_SLOW
    #define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
    #define Assert(Expression)
#endif

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
typedef uint8_t uint8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float real32;
typedef double real64;

struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

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
    bool is_analog;
    real32 start_x;
    real32 start_y;

    real32 min_x;
    real32 min_y;
    real32 max_x;
    real32 max_y;
    real32 end_x;
    real32 end_y;

    union
    {
        game_button_state buttons[6];
        struct
        {
            game_button_state up;
            game_button_state down;
            game_button_state left;
            game_button_state right;
            game_button_state left_shoulder;
            game_button_state right_shoulder;
        };
    };
};

struct game_input
{
    //TODO - insert clock value here
    game_controller_input controllers[4];
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


/*
    services that the platform layer provides to the game
*/


/*
    services that the game provides to the platform layer
*/

//needs four things - timing, controller/keyboard input, bitmap to output, sound to output
static void GameUpdateAndRender(game_memory*,game_input& input,game_offscreen_buffer&,game_sound_output_buffer&);