#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#if _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif // _WIN32

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


#define da_append_many(da, new_items, new_items_count)                                      \
    do {                                                                                    \
        if ((da)->count + new_items_count > (da)->capacity) {                               \
            if ((da)->capacity == 0) {                                                      \
                (da)->capacity = DA_INIT_CAP;                                               \
            }                                                                               \
            while((da)->count + new_items_count > (da)->capacity) {                         \
                (da)->capacity *= 2;                                                        \
            }                                                                               \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items));        \
            assert((da)->items != NULL && "Buy more RAM lol");                              \
        }                                                                                   \
        memcpy((da)->items + (da)->count, new_items, new_items_count*sizeof(*(da)->items)); \
        (da)->count += new_items_count;                                                     \
    } while(0)                                                                              \

typedef struct{
    char *items;
    size_t count;
    size_t capacity;
} String_Builder;

#define sb_append_buf da_append_many
#define sb_append_cstr(sb, cstr)   \
    do {                           \
        const char *s = (cstr);    \
        size_t n = strlen(s);      \
        da_append_many(sb, s, n);  \
    } while(0)                     \

#define sb_append_null(sb) da_append_many(sb, "", 1)


typedef struct{
    const char **items;
    size_t count;
    size_t capacity;
} Cmd;

#define da_foreach(type, item, da)  \
    for (type *item = (da)->items; item < (da)->items + (da)->count; item++) 


void shell_escape_cstr(String_Builder *sb, const char *cstr)
{
    if (!strchr(cstr, ' ')) {
        sb_append_cstr(sb, cstr);
    } else {
        da_append(sb, '\'');
        sb_append_cstr(sb, cstr);
        da_append(sb, '\'');
    }
}

void cmd_render(Cmd cmd, String_Builder *render)
{
    da_foreach(const char *, arg, &cmd) {
        shell_escape_cstr(render, *arg);
        sb_append_cstr(render, " ");
    }
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
typedef HANDLE Proc;
#define INVALID_PROC NULL
#else
typedef int Proc;
#define INVALID_PROC -1
#endif // _WIN32

Proc cmd_run_async(Cmd cmd)
{
    String_Builder sb = {0};
    cmd_render(cmd, &sb);
    sb_append_null(&sb);
    fprintf(stderr, "[CMD] %s\n", sb.items);

    pid_t cpid = fork();
    if (cpid < 0)
    {
        fprintf(stderr, "[ERROR] Could not fork child proc: %s: %s\n", sb.items, strerror(errno));
        return INVALID_PROC;
    }

    if (cpid == 0) 
    {
        if (execvp(*cmd.items, (char * const*) cmd.items) < 0) {
            fprintf(stderr, "[ERROR] Could not exec child proc: %s", strerror(errno));
            exit(1);
        }
        assert(0 && "unreachable");
    }

    return cpid;
}

bool proc_wait(Proc p)
{
    for(;;) {
        int wstatus = 0;
        if (waitpid(p, &wstatus, 0) < 0) {
           fprintf(stderr, "[ERROR] could not wait on command(pid %d): %s", p, strerror(errno));
           return false;
        }
        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                fprintf(stderr, "[ERROR] command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }
        if (WIFSIGNALED(wstatus)) {
            fprintf(stderr, "[ERROR] command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
            return false;
        }
    }
    return true;
}

bool cmd_run_sync(Cmd cmd)
{
    Proc p = cmd_run_async(cmd);
    if (p == INVALID_PROC) return false;

    return proc_wait(p);
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
    cmd_append(cmd, "-ggdb");
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
    cmd_append(&cmd, "foo bar hello.c");

    //if (cmd_run(cmd) != 0) return 1;
    if (!cmd_run_async(cmd)) return 1;
    return 0;
}
