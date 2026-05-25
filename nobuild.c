#include <stdio.h>
#include <stdbool.h>

typedef struct{
    const char **args;
    size_t count;
    size_t capacity;
} Cmd;

#define da_append(da, item)                                                           \
    do {                                                                              \
        if ((da)->count >= (da)->capacity) {                                          \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;    \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items));  \
            assert((da)->items != NULL && "Buy more RAM lol");                        \
        }                                                                             \
        (da)->items[(da)->count++] = (item);                                          \
    } while(0)                                                                        \



void cmd_append_null(Cmd *cmd, ...)
{
    (void)cmd;
}

int cmd_run(Cmd cmd)
{
    return -1;
}

#define cmd_append(cmd, ...) cmd_append_null(cmd, __VA_ARGS__, NULL);

void platform_compiler(Cmd *cmd)
{
#ifdef _WIN32
    cmd_append(cmd, "x86_64-w64-mingw32-gcc");
#else
    cmd_append(cmd, "clang");
#endif // _WIN32
}

void cflags(Cmd *cmd)
{
    cmd_append(cmd, "-Wall");
    cmd_append(cmd, "-Wextra");
    cmd_append(cmd, "-gdb");
#ifdef _WIN32
    cmd_append(cmd, "-I./raylib-4.5.0_win64_mingw-w64/include");
#else
    cmd_append(cmd, "-I/home/streamer/opt/raylib/include");
#endif // _WIN32
}

void musializer_src(Cmd* cmd)
{
    cmd_append(cmd, "./src/musializer.c");
    cmd_append(cmd, "./src/plug.c");
#ifdef _WIN32
    cmd_append(cmd, "./src/ffmpeg_windows.c");
#else
    cmd_append(cmd, "./src/ffmpeg_linux.c");
#endif // _WIN32
    cmd_append(cmd, "./src/spearate_translation_unit_for_miniauio.h");
}

void link_libraries(Cmd *cmd)
{
#ifdef _WIN32
    cmd_append(cmd, "-L./raylib-4.5.0_win64_mingw-w64/lib/");
    cmd_append(cmd, "-lraylib", "-lwinmm", "-lgdi32", "-static");
#else
    cmd_append(cmd, "-L/home/streamer/opt/raylib/lib/");
    cmd_append(cmd, "-lraylib", "-lm", "-ldl", "-lpthread");
#endif // _WIN32
}

int main(void) 
{
    Cmd cmd = {0};
    platform_compiler(&cmd);
    cflags(&cmd);
    cmd_append(&cmd, "-o", "musializer");
    musializer_src(&cmd);
    link_libraries(&cmd);

    if (cmd_run(cmd) != 0) return 1;
	return 0;
}
