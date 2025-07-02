// i0 - Minimal init system
// Copyright (c) 2025 tema5002
// Licensed under the ISC License
#pragma once

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

static char* i0_log_type_color[I0_MAX_LOG_LEVEL - I0_MIN_LOG_LEVEL + 1] = {
    "\033[37m",   // I0_LOG_INFO       -> white
    "\033[32m",   // I0_LOG_GOOD       -> green
    "\033[37m",   // I0_LOG_BAD        -> white
    "\033[32m",   // I0_LOG_TASK_START -> green
    "\033[32m",   // I0_LOG_TASK_STOP  -> green
    "\033[33m",   // I0_LOG_WARNING    -> yellow
    "\033[1;31m", // I0_LOG_CRITICAL   -> bright red
};

static void i0_log(const i0_log_type log_level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    FILE* stream = log_level < I0_LOG_WARNING ? stdout : stderr;
    fprintf(stream, "%s", i0_log_type_color[log_level]);
    fprintf(stream, "[%c] ", i0_log_type_char[log_level]);
    vfprintf(stream, format, args);
    fprintf(stream, "\033[0m");
    fprintf(stream, "\n");
    fflush(stream);

    if (log_level == I0_LOG_CRITICAL) exit(EXIT_FAILURE);

    va_end(args);
}

#define i0_perror(prefix) i0_log(I0_LOG_CRITICAL, "%s: %s", (prefix), strerror(errno))
