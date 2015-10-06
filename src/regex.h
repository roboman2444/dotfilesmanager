/**
 * Copyright 2015 Ted Meyer
 *
 * See LICENSE for details
 */

#ifndef _REGEX_H_
#define _REGEX_H_

#include "bool.h"

struct state {
    struct state *next[256];
    char name;
    bool is_final;
};

struct state *make_state_machine(char *string);
bool matches(const char *str, struct state *regex);

#endif
