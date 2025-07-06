// i0 - Minimal init system
// Copyright (c) 2025 tema5002
// Licensed under the ISC License
#pragma once

#include "lang_defs.h"

static void io_lang_use_ru() {
    i0_lang[I0_LANG_ERROR_NO_ARGS] = "ошибка: аргументы не указаны. попробуйте '%s help'";
    i0_lang[I0_LANG_ERROR_UNKNOWN_COMMAND] = "ошибка: неизвестная команда";
    i0_lang[I0_LANG_ERROR_NO_START_ARG] = "ошибка: нечего запускать";
    i0_lang[I0_LANG_ERROR_NO_STOP_ARG] = "ошибка: нечего завершать";
    i0_lang[I0_LANG_ERROR_NO_STATUS_ARG] = "ошибка: нечего проверять";
    i0_lang[I0_LANG_ERROR_TASK_NOT_FOUND] = "ошибка: задача не найдена";
    i0_lang[I0_LANG_ERROR_DIRECTORY_NOT_FOUND] = "ошибка: директория не найдена";
    i0_lang[I0_LANG_ERROR_MAIN_NOT_FOUND] = "ошибка: main скрипт не найден";

    i0_lang[I0_LANG_IO_Y_UPPERCASE] = "Д";
    i0_lang[I0_LANG_IO_Y_LOWERCASE] = "д";
    i0_lang[I0_LANG_IO_N_UPPERCASE] = "Н";
    i0_lang[I0_LANG_IO_N_LOWERCASE] = "н";

    i0_lang[I0_LANG_NEW_LOCAL_OR_SYSTEM] = "Создать как системную задачу (нужен sudo)?";
    i0_lang[I0_LANG_NEW_NAME] = "Имя задачи: ";
    i0_lang[I0_LANG_NEW_TASK_ALREADY_EXISTS] = "Задача уже существует: %s, перезаписать?";
    i0_lang[I0_LANG_NEW_COMMAND] = "Команда запуска задачи: ";
    i0_lang[I0_LANG_NEW_DESCRIPTION] = "Описание для задачи (необязательно): ",
    i0_lang[I0_LANG_NEW_AUTOSTART] = "Включить автозапуск?";
    i0_lang[I0_LANG_NEW_START] = "Сделать шаблон start?";
    i0_lang[I0_LANG_NEW_STATUS] = "Сделать шаблон status?";
    i0_lang[I0_LANG_NEW_STOP] = "Сделать шаблон stop?";
    i0_lang[I0_LANG_NEW_PATH_TOO_LONG] = "Слишком длинный путь";
    i0_lang[I0_LANG_NEW_TASK_CREATED] = "задача успешно создана по пути %s";

    i0_lang[I0_LANG_STATUS_STARTED] = "запущена задача %s";
    i0_lang[I0_LANG_STATUS_STOPPED] = "остановлена задача %s";

    i0_lang[I0_LANG_STATUS_DESCRIPTION] = "описание";
    i0_lang[I0_LANG_STATUS_ENABLED] = "включено";
    i0_lang[I0_LANG_STATUS_RUNNING] = "запущено";
    i0_lang[I0_LANG_STATUS_NOT_RUNNING] = "не запущено";
    i0_lang[I0_LANG_STATUS_STARTED_AT] = "запущено в";
    i0_lang[I0_LANG_STATUS_ALREADY_STOPPED] = "уже остановлено";
    i0_lang[I0_LANG_STATUS_ALREADY_RUNNING] = "уже запущено с PID %s";
}