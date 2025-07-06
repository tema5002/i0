// i0 - Minimal init system
// Copyright (c) 2025 tema5002
// Licensed under the ISC License

#define I0_VERSION "beta.1.5"
#define I0_VERSION_FULL "i0 " I0_VERSION

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <ftw.h>

#include "lang/lang.c"

typedef enum i0_log_type {
    I0_LOG_INFO = 0,
    I0_LOG_GOOD,
    I0_LOG_BAD,
    I0_LOG_TASK_START,
    I0_LOG_TASK_STOP,
    I0_LOG_WARNING,
    I0_LOG_CRITICAL
} i0_log_type;

#define I0_MIN_LOG_LEVEL I0_LOG_INFO
#define I0_MAX_LOG_LEVEL I0_LOG_CRITICAL

static char i0_log_type_char[I0_MAX_LOG_LEVEL - I0_MIN_LOG_LEVEL + 1] = {
    '+',
    '+',
    '-',
    '>',
    '<',
    '!',
    'x'
};

static char* const i0_log_type_color[I0_MAX_LOG_LEVEL - I0_MIN_LOG_LEVEL + 1] = {
    "\033[37m",   // I0_LOG_INFO       -> white
    "\033[32m",   // I0_LOG_GOOD       -> green
    "\033[37m",   // I0_LOG_BAD        -> white
    "\033[32m",   // I0_LOG_TASK_START -> green
    "\033[32m",   // I0_LOG_TASK_STOP  -> green
    "\033[33m",   // I0_LOG_WARNING    -> yellow
    "\033[1;31m", // I0_LOG_CRITICAL   -> bright red
};

// va_list is a dark magic for compiler so i use __VA_ARGS__
#define i0_log(level, ...) do { \
    FILE* _stream = (level) < I0_LOG_WARNING ? stdout : stderr; \
    fputs(i0_log_type_color[level], _stream); \
    fprintf(_stream, "[%c] ", i0_log_type_char[level]); \
    fprintf(_stream, __VA_ARGS__); \
    fputs("\033[0m\n", _stream); \
    fflush(_stream); \
    if ((level) == I0_LOG_CRITICAL) _exit(EXIT_FAILURE); \
} while(0)

static void i0_perror(const char* prefix) {
    i0_log(I0_LOG_CRITICAL, "%s: %s", prefix, strerror(errno));
}

typedef char i0_string[4096];

#define I0_MAIN_SCRIPT "#!/bin/sh\n" \
    "exec %s\n"

#define I0_START_SCRIPT "#!/bin/sh\n" \
    "[ -f pid ] && pid=$(cat pid) && [ -d \"/proc/$pid\" ] && echo \"%s\" && exit\n" \
    "./main &\n" \
    "echo $! > pid\n" \
    "echo \"%s\"\n"

#define I0_STOP_SCRIPT "#!/bin/sh\n" \
    "[ ! -f pid ] && echo \"%s\" && exit\n" \
    "pid=$(cat pid)\n" \
    "[ ! -d \"/proc/$pid\" ] && echo \"%s\" && exit\n" \
    "kill -KILL \"$pid\"\n" \
    "rm -f pid\n" \
    "echo \"%s\"\n"

#define I0_STATUS_SCRIPT "#!/bin/sh\n" \
    "[ -e enabled ] && echo \"%s\"\n" \
    "[ -f description ] && printf \"%s\\n\" \"$(cat description)\"\n" \
    "[ ! -f pid ] && echo \"%s\" && exit\n" \
    "pid=$(cat pid)\n" \
    "[ ! -d \"/proc/$pid\" ] && echo \"%s\" && exit\n" \
    "echo \"%s\"\n" \
    "echo \"%s\"\n"

#define I0_LOCAL_TASKS_DIR "/.config/i0/tasks/"
#define I0_PUBLIC_TASKS_DIR "/etc/i0/tasks/"

// =========================================== //
// work with processes                         //
// =========================================== //

#define safe_execlp(...) do { if (execlp(__VA_ARGS__) == -1) { i0_perror("execlp()"); } } while (0)

#define fork_and_do(pid, thing, another_thing) do { \
    pid = fork(); if (pid < 0) { i0_perror("fork()"); } \
    else if (pid == 0) { thing; _exit(EXIT_SUCCESS); } \
    else { another_thing; } \
} while (0)

#define fork_and_do_no_pid(thing, another_thing) do { \
    pid_t pid; fork_and_do(pid, thing, another_thing); \
} while (0)

static int is_process_alive(const pid_t pid) {
    return kill(pid, 0) == 0 || errno == EPERM;
}

// =========================================== //
// string work                                 //
// =========================================== //

static void i0_string_append(char *dest, size_t dest_size, const char *suff, size_t suff_size) {
    if (dest_size + suff_size + 1 > sizeof(i0_string)) {
        i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_BUFFER_OVERFLOW]);
    }
    memcpy(dest + dest_size, suff, suff_size);
}

static int str_eq(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

#define conststrlen(s) (sizeof(s) - 1)

// =========================================== //
// path work                                   //
// =========================================== //

static void i0_get_local_tasks_dir(i0_string path) {
    const char* home = getenv("HOME");
    if (home == NULL) i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_NO_HOME]);

    size_t len = strlen(home);
    i0_string_append(path, 0, home, len);

    // you are a weird person if '/' is your home
    if (len < 2 || path[len - 1] == '/') len--;
    i0_string_append(path, len, I0_LOCAL_TASKS_DIR, conststrlen(I0_LOCAL_TASKS_DIR) + 1);
}

static void i0_get_system_tasks_dir(i0_string path) {
    i0_string_append(path, 0, I0_PUBLIC_TASKS_DIR, conststrlen(I0_PUBLIC_TASKS_DIR) + 1);
}

static void i0_get_tasks_dir(i0_string path) {
    if (geteuid() == 0) {
        i0_get_system_tasks_dir(path);
    }
    else {
        i0_get_local_tasks_dir(path);
    }
}

// =========================================== //
// directory work                              //
// =========================================== //

static int dir_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void safe_mkdir(i0_string path) {
    if (!dir_exists(path)) {
        if (mkdir(path, 0755) != 0) {
            i0_perror("mkdir()");
        }
    }
}

static void mkdir_p(i0_string path) {
    for (char* p = path + 1;; p++) {
        if (*p == '/' || *p == '\0') {
            const char c = *p;
            *p = '\0';
            safe_mkdir(path);
            if (c == '\0') break;
            *p = c;
        }
    }
}

static int unlink_cb(const char* fpath, const struct stat* _1, int _2, struct FTW* _3) {
    (void)_1; (void)_2; (void)_3;
    const int ret = remove(fpath);
    if (ret) i0_perror(fpath);
    return ret;
}

static int rmdir_rf(const char *path) {
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

// =========================================== //
// work with files                             //
// =========================================== //

static FILE* safe_fopen(const char* path, const char* mode) {
    FILE* f = fopen(path, mode);
    if (f == NULL) {
        i0_perror("fopen()");
    }
    return f;
}

#define rwxr_xr_x (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

static void safe_fwrite(const void* ptr, const size_t n, FILE* file) {
    if (fwrite(ptr, 1, n, file) != n) {
        i0_perror("fwrite()");
    }
}

static void open_write(const char* path, const char* content, const size_t size) {
    FILE* f = safe_fopen(path, "w");
    safe_fwrite(content, size, f);
    fclose(f);
    chmod(path, rwxr_xr_x);
}

static void open_write_int(const char* path, const int i) {
    FILE* f = safe_fopen(path, "w");
    fprintf(f, "%d\n", i);
    fclose(f);
}

static int file_exists(const char* path) {
    return access(path, X_OK) == 0;
}

// =========================================== //
// work with input                             //
// =========================================== //

static void safe_fgets(char* ptr, const int n, FILE* file) {
    if (!fgets(ptr, n, file)) {
        i0_perror("fgets()");
    }
}

static void i0_clear_stdin(char* buf) {
    const size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] != '\n') {
        while (getchar() != '\n' && !feof(stdin)) ;
    }
    else if (len > 0) {
        buf[len - 1] = '\0';
    }
}

typedef enum { i0_no, i0_yes } i0_yesno;

static i0_yesno i0_prompt_yesno(const i0_yesno def, const char* format, ...) {
    char line[8];
    for (;;) {
        const char* y = def == i0_yes ? i0_lang[I0_LANG_IO_Y_UPPERCASE] : i0_lang[I0_LANG_IO_Y_LOWERCASE];
        const char* n = def == i0_no ? i0_lang[I0_LANG_IO_N_UPPERCASE] : i0_lang[I0_LANG_IO_N_LOWERCASE];

        va_list args;
        va_start(args, format);
        vfprintf(stdout, format, args);
        printf(" [%s/%s]: ", y, n);
        va_end(args);

        safe_fgets(line, sizeof(line), stdin);
        i0_clear_stdin(line);

        if (line[0] == '\0') return def;

        if (str_eq(line, "y")) return i0_yes;
        if (str_eq(line, "Y")) return i0_yes;
        if (str_eq(line, "n")) return i0_no;
        if (str_eq(line, "N")) return i0_no;

        if (str_eq(line, i0_lang[I0_LANG_IO_Y_LOWERCASE])) return i0_yes;
        if (str_eq(line, i0_lang[I0_LANG_IO_Y_UPPERCASE])) return i0_yes;
        if (str_eq(line, i0_lang[I0_LANG_IO_N_LOWERCASE])) return i0_no;
        if (str_eq(line, i0_lang[I0_LANG_IO_N_UPPERCASE])) return i0_no;
    }
}

static void i0_read_prompt(i0_string buf, char* prompt, const int required) {
    do {
        printf("%s", prompt);
        safe_fgets(buf, sizeof(i0_string), stdin);
        i0_clear_stdin(buf);
    } while (buf[0] == '\0' && required);
}

// =========================================== //
// actual i0 functionality                     //
// =========================================== //

static void i0_task_find(const char* name, i0_string path) {
    i0_get_tasks_dir(path);
    i0_string_append(path, strlen(path), name, strlen(name) + 1);

    if (!dir_exists(path)) {
        i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_TASK_NOT_FOUND]);
    }
}

static void i0_task_new() {
    i0_string task_name;
    i0_read_prompt(task_name, i0_lang[I0_LANG_NEW_NAME], 1);

    i0_string task_path;
    i0_get_tasks_dir(task_path);

    size_t task_path_size = strlen(task_path);
    const size_t task_name_size = strlen(task_name);
    if (task_name_size + task_path_size > sizeof(task_path)) {
        i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_NEW_PATH_TOO_LONG]);
    }

    i0_string_append(task_path, task_path_size, task_name, task_name_size);

    int rewrite = 0;

    if (dir_exists(task_path)) {
        rewrite = i0_prompt_yesno(i0_no, i0_lang[I0_LANG_NEW_TASK_ALREADY_EXISTS], task_path);
        if (!rewrite) return;
    }

    i0_string task_command;
    i0_read_prompt(
        task_command,
        i0_lang[I0_LANG_NEW_COMMAND],
        1
    );

    i0_string description;
    i0_read_prompt(
        description,
        i0_lang[I0_LANG_NEW_DESCRIPTION],
        0
    );

    const i0_yesno autostart = i0_prompt_yesno(i0_no, "%s", i0_lang[I0_LANG_NEW_AUTOSTART]);

    const i0_yesno make_start = i0_prompt_yesno(i0_no, "%s", i0_lang[I0_LANG_NEW_START]);
    const i0_yesno make_stop = i0_prompt_yesno(i0_no, "%s", i0_lang[I0_LANG_NEW_STOP]);
    const i0_yesno make_status = i0_prompt_yesno(i0_no, "%s", i0_lang[I0_LANG_NEW_STATUS]);

    if (rewrite) rmdir_rf(task_path);
    mkdir_p(task_path);
    task_path_size = strlen(task_path);

    {
        i0_string_append(task_path, task_path_size, "/main", conststrlen("/main") + 1);
        i0_string main_script;
        snprintf(
            main_script,
            sizeof(main_script),
            I0_MAIN_SCRIPT,
            task_command
        );
        open_write(task_path, main_script, strlen(main_script));
    }

    if (make_start) {
        char _already_buf[256];
        snprintf(
            _already_buf, sizeof(_already_buf),
            i0_lang[I0_LANG_STATUS_ALREADY_RUNNING],
            "$pid"
        );

        char already_buf[256];
        snprintf(
            already_buf, sizeof(already_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_WARNING],
            i0_log_type_char[I0_LOG_WARNING],
            _already_buf
        );

        char _started_buf[256];
        snprintf(_started_buf, sizeof(_started_buf),
            i0_lang[I0_LANG_STATUS_STARTED],
            task_name
        );

        char started_buf[256];
        snprintf(
            started_buf, sizeof(started_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_TASK_START],
            i0_log_type_char[I0_LOG_TASK_START],
            _started_buf
        );

        i0_string_append(task_path, task_path_size, "/start", conststrlen("/start") + 1);
        i0_string start_script;
        snprintf(
            start_script,
            sizeof(start_script),
            I0_START_SCRIPT,
            already_buf,
            started_buf
        );
        open_write(task_path, start_script, strlen(start_script));
    }

    if (make_status) {
        char enabled_buf[256];
        snprintf(
            enabled_buf, sizeof(enabled_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_GOOD],
            i0_log_type_char[I0_LOG_GOOD],
            i0_lang[I0_LANG_STATUS_ENABLED]
        );

        char description_buf[256];
        snprintf(
            description_buf, sizeof(description_buf),
            "%s[%c] %s: %%s\033[0m",
            i0_log_type_color[I0_LOG_INFO],
            i0_log_type_char[I0_LOG_INFO],
            i0_lang[I0_LANG_STATUS_DESCRIPTION]
        );

        char not_running_buf[256];
        snprintf(
            not_running_buf, sizeof(not_running_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_BAD],
            i0_log_type_char[I0_LOG_BAD],
            i0_lang[I0_LANG_STATUS_NOT_RUNNING]
        );

        char _running_buf[256];
        snprintf(
            _running_buf, sizeof(_running_buf),
            i0_lang[I0_LANG_STATUS_RUNNING],
            "$pid"
        );

        char running_buf[256];
        snprintf(
            running_buf, sizeof(running_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_GOOD],
            i0_log_type_char[I0_LOG_GOOD],
            _running_buf
        );

        char _started_at_buf[256];
        snprintf(
            _started_at_buf, sizeof(_started_at_buf),
            i0_lang[I0_LANG_STATUS_STARTED_AT],
            "$(date -r pid '+%Y-%m-%d %H:%M:%S')"
        );

        char started_at_buf[256];
        snprintf(
            started_at_buf, sizeof(started_at_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_GOOD],
            i0_log_type_char[I0_LOG_GOOD],
            _started_at_buf
        );

        i0_string_append(task_path, task_path_size, "/status", conststrlen("/status") + 1);
        i0_string status_script;
        snprintf(
            status_script,
            sizeof(status_script),
            I0_STATUS_SCRIPT,
            enabled_buf,
            description_buf,
            not_running_buf,
            not_running_buf,
            running_buf,
            started_at_buf
        );
        open_write(task_path, status_script, strlen(status_script));
    }

    if (make_stop) {
        char already_buf[256];
        snprintf(
            already_buf, sizeof(already_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_WARNING],
            i0_log_type_char[I0_LOG_WARNING],
            i0_lang[I0_LANG_STATUS_ALREADY_STOPPED]
        );

        char _stopped_buf[256];
        snprintf(_stopped_buf, sizeof(_stopped_buf),
            i0_lang[I0_LANG_STATUS_STOPPED],
            task_name
        );

        char stopped_buf[256];
        snprintf(
            stopped_buf, sizeof(stopped_buf),
            "%s[%c] %s\033[0m",
            i0_log_type_color[I0_LOG_TASK_STOP],
            i0_log_type_char[I0_LOG_TASK_STOP],
            _stopped_buf
        );

        i0_string_append(task_path, task_path_size, "/stop", conststrlen("/stop") + 1);
        i0_string stop_script;
        snprintf(
            stop_script,
            sizeof(stop_script),
            I0_STOP_SCRIPT,
            already_buf,
            already_buf,
            stopped_buf
        );
        open_write(task_path, stop_script, strlen(stop_script));
    }

    if (description[0] != '\0') {
        const size_t len = strlen(description);
        description[len] = '\n'; // posix standard thing idk
        i0_string_append(task_path, task_path_size, "/description", conststrlen("/description") + 1);
        open_write(task_path, description, len + 1);
    }

    if (autostart) {
        i0_string_append(task_path, task_path_size, "/enabled", conststrlen("/enabled") + 1);
        FILE* f = safe_fopen(task_path, "w");
        fclose(f);
    }

    task_path[task_path_size] = '\0';
    i0_log(I0_LOG_GOOD, i0_lang[I0_LANG_NEW_TASK_CREATED], task_path);
}

static void i0_task_start(const char* task) {
    FILE* f = fopen("./pid", "r");
    if (f) {
        pid_t pid;
        if (fscanf(f, "%d", &pid) == 1 && is_process_alive(pid)) {
            char pidbuf[16];
            snprintf(pidbuf, 16, "%d", pid);
            i0_log(I0_LOG_WARNING, i0_lang[I0_LANG_STATUS_ALREADY_RUNNING], pidbuf);
            fclose(f);
            return;
        }
        fclose(f);
    }

    pid_t pid;
    fork_and_do(pid, safe_execlp("./main", task, NULL), open_write_int("./pid", pid));

    i0_log(I0_LOG_TASK_START, i0_lang[I0_LANG_STATUS_STARTED], task);
}

static void i0_task_start_script(const char* task, const char* path) {
    if (chdir(path) != 0) {
        i0_perror("chdir");
    }

    if (file_exists("./start")) {
        safe_execlp("./start", task, NULL);
        return;
    }

    if (file_exists("./main")) {
        i0_task_start(task);
        return;
    }

    i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_MAIN_NOT_FOUND]);
}

static void i0_task_stop(const char* task) {
    FILE* f = fopen("./pid", "r");
    if (!f) {
        i0_log(I0_LOG_WARNING, "%s", i0_lang[I0_LANG_STATUS_ALREADY_STOPPED]);
        return;
    }

    pid_t pid;
    if (fscanf(f, "%d", &pid) != 1) {
        fclose(f);
        i0_log(I0_LOG_WARNING, "%s", i0_lang[I0_LANG_STATUS_ALREADY_STOPPED]);
        return;
    }
    fclose(f);

    if (!is_process_alive(pid)) {
        unlink("./pid");
        i0_log(I0_LOG_WARNING, "%s", i0_lang[I0_LANG_STATUS_ALREADY_STOPPED]);
        return;
    }

    if (kill(pid, SIGKILL) != 0) {
        if (errno == ESRCH) {
            unlink("pid");
            i0_log(I0_LOG_WARNING, "%s", i0_lang[I0_LANG_STATUS_ALREADY_STOPPED]);
        }
        else {
            i0_perror("kill()");
        }
        return;
    }

    unlink("./pid");
    i0_log(I0_LOG_TASK_STOP, i0_lang[I0_LANG_STATUS_STOPPED], task);
}

static void i0_task_stop_script(const char* task, const char* path) {
    if (chdir(path) != 0) {
        i0_perror("chdir");
    }

    if (file_exists("./stop")) {
        safe_execlp("./stop", task, NULL);
        return;
    }

    i0_task_stop(task);
}

static void i0_task_status() {
    if (file_exists("./enabled")) i0_log(I0_LOG_GOOD, "%s", i0_lang[I0_LANG_STATUS_ENABLED]);

    FILE* f = fopen("./description", "r");
    if (f) {
        char buffer[512] = {0};

        // posix standard suggests each file to have \n in the end
        // and that completely fucks up i0_log output
        fread(buffer, 1, sizeof(buffer) - 1, f);

        const size_t desc_len = strlen(buffer);
        if (desc_len > 0 && buffer[desc_len - 1] == '\n') {
            buffer[desc_len - 1] = '\0';
        }

        i0_log(I0_LOG_INFO, "%s: %s", i0_lang[I0_LANG_STATUS_DESCRIPTION], buffer);

        fclose(f);
    }

    f = fopen("./pid", "r");
    if (f) {
        pid_t pid;
        if (fscanf(f, "%d", &pid) == 1 && is_process_alive(pid)) {
            char pidbuf[16];
            snprintf(pidbuf, 16, "%d", pid);
            i0_log(I0_LOG_GOOD, i0_lang[I0_LANG_STATUS_RUNNING], pidbuf);

            struct stat st;

            if (stat("./pid", &st) != 0) {
                i0_perror("stat()");
            }

            struct tm* tm = localtime(&st.st_mtime);
            if (!tm) {
                i0_perror("localtime()");
            }

            char time_buf[64];
            if (!strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm)) {
                i0_perror("strftime()");
            }

            i0_log(I0_LOG_GOOD, i0_lang[I0_LANG_STATUS_STARTED_AT], time_buf);
        }
        else goto not_running;
        fclose(f);
    }
    else goto not_running;
    return;
    not_running:
        i0_log(I0_LOG_BAD, "%s", i0_lang[I0_LANG_STATUS_NOT_RUNNING]);
}

static void i0_task_status_script(const char* task, const char* path) {
    if (chdir(path) != 0) {
        i0_perror("chdir");
    }

    if (file_exists("./status")) {
        safe_execlp("./status", task, NULL);
        return;
    }

    i0_task_status();
}

static void i0_boot_scan(i0_string path) {
    DIR* d = opendir(path);
    if (!d) {
        if (errno == ENOENT) return;
        i0_perror("opendir()");
    }

    struct dirent* dir;
    const size_t path_len = strlen(path);
    while ((dir = readdir(d)) != NULL) {
        const char* filename = dir->d_name;

        if (str_eq(dir->d_name, ".") || str_eq(dir->d_name, "..")) {
            continue;
        }

        const size_t filename_len = strlen(filename);

        i0_string_append(path, path_len, filename, filename_len + 1);
        if (dir_exists(path)) {
            const size_t path_filename_len = path_len + filename_len;
            i0_string_append(path, path_filename_len, "/enabled", conststrlen("/enabled") + 1);

            if (file_exists(path)) {
                fork_and_do_no_pid(i0_task_start_script(dir->d_name, path), ;);
            }
        }
    }
    closedir(d);
}

static void i0_boot() {
    i0_log(I0_LOG_INFO, "%s", i0_lang[I0_LANG_BOOT_START]);

    i0_string path;

    i0_get_tasks_dir(path);
    i0_boot_scan(path);

    i0_log(I0_LOG_INFO, "%s", i0_lang[I0_LANG_BOOT_END]);
}

#define i0_task_find_and_do(task, thing) do { \
    i0_string path; \
    i0_task_find(task, path); \
    fork_and_do_no_pid(thing(task, path), ;); \
} while (0)

#define i0_task_find_and_do_no_fork(task, thing) do { \
    i0_string path; \
    i0_task_find(task, path); \
    thing(task, path); \
} while (0)

int main(const int argc, const char* argv[]) {
    i0_get_lang();

    if (argc < 2) {
        i0_log(I0_LOG_CRITICAL, i0_lang[I0_LANG_ERROR_NO_ARGS], argv[0]);
    }

    if (str_eq(argv[1], "version")) {
        puts(I0_VERSION_FULL);
        return EXIT_SUCCESS;
    }

    if (str_eq(argv[1], "help")) {
        safe_execlp("man", "man", "i0", NULL);
    }

    if (str_eq(argv[1], "boot")) {
        if (geteuid() != 0) {
            i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_BOOT_NO_PERMISSION]);
        }
        i0_boot();
        return EXIT_SUCCESS;
    }

    if (str_eq(argv[1], "new")) {
        i0_task_new();
        return EXIT_SUCCESS;
    }

    if (str_eq(argv[1], "start")) {
        if (argc < 3) {
            i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_NO_START_ARG]);
        }
        i0_task_find_and_do(argv[2], i0_task_start_script);
        return EXIT_SUCCESS;
    }

    if (str_eq(argv[1], "stop")) {
        if (argc < 3) {
            i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_NO_STOP_ARG]);
        }
        i0_task_find_and_do(argv[2], i0_task_stop_script);
        return EXIT_SUCCESS;
    }

    if (str_eq(argv[1], "status")) {
        if (argc < 3) {
            i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_NO_STATUS_ARG]);
        }
        i0_task_find_and_do_no_fork(argv[2], i0_task_status_script);
        return EXIT_SUCCESS;
    }

    i0_log(I0_LOG_CRITICAL, "%s", i0_lang[I0_LANG_ERROR_UNKNOWN_COMMAND]);
}
