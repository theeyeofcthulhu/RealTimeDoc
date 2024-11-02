#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <fmt/core.h>

#include <raylib.h>

#include "format.hpp"

class TmpFile {
public:
    TmpFile();
    ~TmpFile();

    int fd() const { return p_fd; }
    std::string_view fn() const { return tmp_fn; }
    bool has_changed();
private:
    static constexpr std::string_view templ = "/tmp/RealTimeDoc-XXXXXX";

    char* tmp_fn;
    int p_fd;

    struct timespec last_timestamp = { 0, 0 };
};

TmpFile::TmpFile()
{
    tmp_fn = strdup(templ.data());

    if ((p_fd = mkstemp(tmp_fn)) == -1) {
        perror("mkstemp");
        exit(1);
    }
}

TmpFile::~TmpFile()
{
    fmt::println("Deleting {}", tmp_fn);
    if (remove(tmp_fn) == -1) {
        perror(fmt::format("Removing {}", tmp_fn).c_str());
    }

    free(tmp_fn);
}

bool TmpFile::has_changed()
{
    struct stat s;
    if (fstat(p_fd, &s) == -1) {
        perror("fstat");
        return false;
    }

    bool has_changed = (s.st_ctim.tv_sec > last_timestamp.tv_sec)
                       || (s.st_ctim.tv_nsec > last_timestamp.tv_nsec);

    if (has_changed)
        last_timestamp = s.st_ctim;

    return has_changed;
}

bool child_has_exited()
{
    siginfo_t exit_info;
    int result = waitid(P_ALL, -1, &exit_info, WEXITED | WNOHANG);

    if (result == -1){
        perror("waitid");
        exit(1);
    }

    return exit_info.si_signo == SIGCHLD;
}

void raylib_loop(TmpFile &tf)
{
    TextFormatter form;

    int width = 800;
    int height = 450;

    int font_size = 32;
    int load_font_size = font_size;
    const int font_reloading_distance = 12;

    InitWindow(width, height, "RealTimeDoc");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(30);

    float vertical = 0;

    Camera2D camera = { {20.0f, 20.0f},
                        {0, vertical},
                        0.0f,
                        1.0f };

    Font font = LoadFontEx("resources/NotoSans-Regular.ttf", load_font_size, 0, 0xff);

    // Loads all UTF-8 chars we need from the string
    int codepoint_count;
    int *cp = LoadCodepoints(codepoints, &codepoint_count);

    Font math_font = LoadFontEx("resources/NotoSansMath-Regular.ttf", load_font_size, cp, codepoint_count);

    double last_file_update = 0.0f;
    while (true)
    {
        if (child_has_exited()) {
            fmt::println("Vim has exited.");
            break;
        }

        bool should_update = false;

        if (IsKeyDown(KEY_RIGHT_BRACKET) || IsKeyDown(KEY_LEFT_BRACKET)) {
            should_update = true;
            font_size += IsKeyDown(KEY_LEFT_BRACKET) ? -2 : 2;

            // Reload fonts if displayed size is too different from loaded size
            // TODO: think about dictionary for already loaded sizes (big memory footprint after loading)
            if (abs(load_font_size - font_size) >= font_reloading_distance) {
                load_font_size = font_size;
                fmt::println("Reloading fonts with size {}", load_font_size);

                UnloadFont(font);
                font = LoadFontEx("resources/NotoSans-Regular.ttf",
                        load_font_size, 0, 0xff);

                UnloadFont(math_font);
                math_font = LoadFontEx("resources/NotoSansMath-Regular.ttf",
                        load_font_size, cp, codepoint_count);
            }
        }

        if (GetMouseWheelMove() != 0.0f) {
            should_update = true;

            vertical += ((float)GetMouseWheelMove() * 20.0f);
            camera.target = { 0, vertical };
        }

        if (IsWindowResized()) {
            should_update = true;

            width = GetScreenWidth();
            height = GetScreenHeight();
        }

        // Only check file every half second
        if (GetTime() - last_file_update > 0.5f) {
            last_file_update = GetTime();

            if (tf.has_changed()) {
                should_update = true;

                std::ifstream f(tf.fn().data());
                if (!f.is_open()) {
                    perror(tf.fn().data());
                    continue;
                }

                form.read(f);
            }
        }

        BeginDrawing();
        {
            if (should_update) {
                ClearBackground(WHITE);

                BeginMode2D(camera);
                {
                    form.render(font, math_font, font_size);
                }
                EndMode2D();
            }
        }
        EndDrawing();

        // Somebody might want to read the log live
        // with tail -f
        fflush(stdout);
    }

    UnloadFont(font);
    UnloadFont(math_font);
    UnloadCodepoints(cp);

    CloseWindow();        // Close window and OpenGL context
}

int main()
{
    TmpFile tf;
    fmt::println("Temporary file: {}", tf.fn());

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    } else if (pid > 0) {
        // Vim needs TTY, so we write to a seperate log file
        if (freopen("rtd-log.txt", "a+", stdout) == NULL) {
            perror("freopen(\"rtd-log.txt\")");
        }

        std::time_t secs = std::time(nullptr);
        fmt::println("BEGIN {}", std::asctime(std::localtime(&secs)));

        // Exits when Vim exits
        raylib_loop(tf);

        fmt::println("END {}", std::asctime(std::localtime(&secs)));

        if (freopen("/dev/tty", "w", stdout) == NULL) {
            perror("freopen(\"/dev/tty\")");
        }
    } else {
        int result = execl("/usr/bin/vim", 
              "/usr/bin/vim", "-S", "rtd-vimrc.vim", tf.fn().data(),
              (char *)0);

        if (result == -1) {
            perror("execl");
            exit(1);
        }
    }

    return 0;
}
