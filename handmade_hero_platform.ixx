export module platform_layer;

export struct game_file_data
{
    void* memory;
    int file_size;
};

export class platform_layer 
{
    public:
        virtual game_file_data debug_platform_read_entire_file(char* filename) = 0;
        virtual void debug_platform_free_file_memory(void *bitmap_memory) = 0;
        virtual bool debug_platform_write_entire_file(char* filename, int memory_size, void *memory) = 0;
};
