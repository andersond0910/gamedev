#ifdef HANDMADE_SLOW
    #define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
        #define Assert(Expression)
#endif

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

template<typename T>
internal size_t array_count(T array[])
{
	return sizeof(*array) / sizeof(array[0]);
}