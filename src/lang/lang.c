// i0 - Minimal init system
// Copyright (c) 2025 tema5002
// Licensed under the ISC License
#pragma once

#include "en.c"
#include "ru.c"

static void i0_get_lang() {
    const char *env = getenv("LC_ALL");
    if (!env || !*env) env = getenv("LC_MESSAGES");
    if (!env || !*env) env = getenv("LANG");
    if (!env || !*env) goto fallback;

    if (strncmp(env, "ru", 2) == 0) {
        io_lang_use_ru();
        return;
    }
fallback:
    io_lang_use_en();
}
