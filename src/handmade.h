#pragma once

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
    game_controller_input controllers[4];
};

/*
    services that the platform layer provides to the game
*/


/*
    services that the game provides to the platform layer
*/

//needs four things - timing, controller/keyboard input, bitmap to output, sound to output
static void GameUpdateAndRender(game_input& input,game_offscreen_buffer&,game_sound_output_buffer&);