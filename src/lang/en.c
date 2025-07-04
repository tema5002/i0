// i0 - Minimal init system
// Copyright (c) 2025 tema5002
// Licensed under the ISC License
#pragma once

#include "lang_defs.h"

static void io_lang_use_en() {
    i0_lang[I0_LANG_ERROR_NO_ARGS] = "error: no arguments provided. try '%s help'";
    i0_lang[I0_LANG_ERROR_UNKNOWN_COMMAND] = "error: unknown command";
    i0_lang[I0_LANG_ERROR_BOOT_NO_PERMISSION] = "error: 'boot' must be run as root";
    i0_lang[I0_LANG_ERROR_NO_START_ARG] = "error: nothing to start";
    i0_lang[I0_LANG_ERROR_NO_STOP_ARG] = "error: nothing to stop";
    i0_lang[I0_LANG_ERROR_NO_STATUS_ARG] = "error: nothing to status";
    i0_lang[I0_LANG_ERROR_TASK_NOT_FOUND] = "error: task not found";
    i0_lang[I0_LANG_ERROR_DIRECTORY_NOT_FOUND] = "error: directory does not exist: %s";
    i0_lang[I0_LANG_ERROR_MAIN_NOT_FOUND] = "error: main script not found";
    i0_lang[I0_LANG_ERROR_BUFFER_OVERFLOW] = "error: buffer overflow";
    i0_lang[I0_LANG_ERROR_NO_HOME] = "error: HOME environment variable is not set";

    i0_lang[I0_LANG_IO_Y_UPPERCASE] = "Y";
    i0_lang[I0_LANG_IO_Y_LOWERCASE] = "y";
    i0_lang[I0_LANG_IO_N_UPPERCASE] = "N";
    i0_lang[I0_LANG_IO_N_LOWERCASE] = "n";

    i0_lang[I0_LANG_NEW_LOCAL_OR_SYSTEM] = "Create as system task (needs sudo)?";
    i0_lang[I0_LANG_NEW_NAME] = "Task name: ";
    i0_lang[I0_LANG_NEW_TASK_ALREADY_EXISTS] = "Task already exists at %s, rewrite?";
    i0_lang[I0_LANG_NEW_COMMAND] = "Command to run task: ";
    i0_lang[I0_LANG_NEW_DESCRIPTION] = "Description for task (optional): ",
    i0_lang[I0_LANG_NEW_AUTOSTART] = "Enable autostart?";
    i0_lang[I0_LANG_NEW_START] = "Make start template script?";
    i0_lang[I0_LANG_NEW_STATUS] = "Make status template script?";
    i0_lang[I0_LANG_NEW_STOP] = "Make stop template script?";
    i0_lang[I0_LANG_NEW_PATH_TOO_LONG] = "Path name too long";
    i0_lang[I0_LANG_NEW_TASK_CREATED] = "successfully created task at %s";

    i0_lang[I0_LANG_STATUS_STARTED] = "started %s";
    i0_lang[I0_LANG_STATUS_STOPPED] = "stopped %s";

    i0_lang[I0_LANG_STATUS_DESCRIPTION] = "description";
    i0_lang[I0_LANG_STATUS_ENABLED] = "enabled";
    i0_lang[I0_LANG_STATUS_RUNNING] = "running: PID %s";
    i0_lang[I0_LANG_STATUS_NOT_RUNNING] = "not running";
    i0_lang[I0_LANG_STATUS_STARTED_AT] = "started at: %s";
    i0_lang[I0_LANG_STATUS_ALREADY_STOPPED] = "already stopped";
    i0_lang[I0_LANG_STATUS_ALREADY_RUNNING] = "already running with PID %s";

    i0_lang[I0_LANG_BOOT_START] = "starting boot sequence";
    i0_lang[I0_LANG_BOOT_END] = "boot sequence ended";
}