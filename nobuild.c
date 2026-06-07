#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#define DA_INIT_CAP 256
#define da_append(da, item)                                                           \
    do {                                                                              \
        if ((da)->count >= (da)->capacity) {                                          \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;    \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items));  \
            assert((da)->items != NULL && "Buy more RAM lol");                        \
        }                                                                             \
        (da)->items[(da)->count++] = (item);                                          \
    } while(0)                                                                        \


typedef struct{
    char *items;
    size_t count;
    size_t capacity;
} String_Builder;

typedef struct{
    const char **items;
    size_t count;
    size_t capacity;
} Cmd;

void cmd_render(Cmd cmd, String_Builder *render)
{

}


void cmd_append_null(Cmd *cmd, ...)
{
    va_list args;
    va_start(args, cmd); 

    const char *arg = va_arg(args, const char*);
    while(arg != NULL) {
        da_append(cmd, arg);
        arg = va_arg(args, const char*);
    }

    va_end(args);
    (void)cmd;
}


#ifdef _WIN32
typedef HANDLE Pid;
#else
typedef int Pid;
#endif // _WIN32

Pid cmd_run_sync(Cmd cmd)
{
    pid_t cpid = fork();
    if (cpid < 0)
    {
	PANIC("Could not fork child process: %s: %s",
		cmd_show(cmd), strerror(errno));
    }

    if (cpid == 0) 
    {
	Cstr_Array args = cstr_array_append(cmd.line, NULL);

	if (fdin)
	{
	    if (dup2(*fdin, STDIN_FILENO) < 0) 
	    {
		PANIC("Could not setup stdin for child process: %s", strerror(errno));
	    }
	}

	if (fdout) 
	{
	    if (dup2(*fdout, STDOUT_FILENO) < 0)
	    {
		PANIC("Could not setup stdout for child process: %s", strerror(errno));
	    }
	}

	if (execvp(args.elems[0], (char * const*) args.elems) < 0)
	{
	    PANIC("Could not exec child process: %s: %s", cmd_show(cmd), strerror(errno));
	}
    }

    return cpid;
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
    cmd_append(&cmd, "-o", "./build/musializer");
    musializer_src(&cmd);
    link_libraries(&cmd);

    if (cmd_run(cmd) != 0) return 1;
    return 0;
}
