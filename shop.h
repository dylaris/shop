/*
shop.h - v0.2.0 - Dylaris 2025
===================================================

BRIEF:
  A simple cmdline flag parser in C99. Only support short
  option, like '-h'.

NOTICE:
  Not compatible with C++.

USAGE:
  In exactly one source file, define the implementation macro
  before including this header:
  ```
    #define SHOP_IMPLEMENTATION
    #include "shop.h"
  ```
  In other files, just include the header without the macro.

EXAMPLE:
```c
// ./main -hn 1
int main(int argc, char **argv) {
    shop_set("vn:f:h");

    shop_desc('h', NULL, "Show this help message with detailed information about all options");
    shop_desc('v', NULL, "Enable verbose output mode for debugging purposes");
    shop_desc('f', "%s", "Filename (string)");
    shop_desc('n', "%d", "Number (int)");

    shop_track(argc, argv);

    // get the option items
    const char *filename;
    if (shop_sget('f', 0, &filename)) {
        printf("filename: %s\n", filename);
    }

    int number;
    shop_foreach('n', i, &number) {
        printf("Number[%ld]: %d\n", i, number);
    }

    // option callback
    const shop_option_t *opt_ptr;
    if (shop_use('h')) shop_help();
    if ((opt_ptr = shop_use('n'))) { ... };

    shop_free();
    return 0;
}
```

HISTORY:
    v0.2.0 (2026-1-1 by @dylaris): repeat options for multiple values:
                                   ```
                                    ./program -t 1 -t 2     # Supported
                                    ./program -t 1 2        # Not supported
                                   ```
                                   each value needs its own -t.
    v0.1.0 (2025-12-31 by @dylaris): a basic cmdline parser, only support short option and single value

LICENSE:
  See the end of this file for further details.
*/

#ifndef SHOP_H
#define SHOP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef SHOPDEF
#define SHOPDEF
#endif

typedef struct {
    unsigned char name;
    const char *info;
    const char *scan_fmt; // used by sscanf
    bool used;
    bool take_arg;
    const char **items; // option value array
    size_t len;
    size_t cap;
} shop_option_t;

#define SHOP_ASSERT(expr, fmt, ...)                         \
    do {                                                    \
        if (expr) break;                                    \
        fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__); \
        exit(EXIT_FAILURE);                                 \
    } while (0)

// shop_set - set the predifined options
// @opt_str: predifined option string
// Note: 'n:' means option 'n' with argument
//       'h'  means option 'h' without argument
//       so you can define that 'hvn:f:'
SHOPDEF void shop_set(const char *opt_str);

// shop_desc - add the description for the option
// @scan_fmt: scan format string, used in 'sscanf'
// @info: help message
SHOPDEF void shop_desc(unsigned char name, const char *scan_fmt, const char *info);

// shop_track - track the cmdline arguments and update option's state
// @argc: number of argument
// @argv: argument string array
// Note: supports option combination (e.g., -abc).
//       in combined options, only the **last one** may take an argument.
//       example: '-fdata.txt' or '-f data.txt' where 'f' requires an argument.
SHOPDEF void shop_track(int argc, char **argv);

// shop_free - free the memory
SHOPDEF void shop_free(void);

// shop_use - check if the option is used
// @name: option name
// Return: the pointer to option if option is used, otherwise NULL
SHOPDEF const shop_option_t *shop_use(unsigned char name);

// shop_sget - get the option value through scan format (sscanf)
// @name: option name
// @idx: index of option value array
// @dst: the pointer to destination
// Note: support an extra format "%b" for boolean type
//       steal from https://github.com/rxi/ini/blob/master/src/ini.c
// Return: true if get the value, false otherwise
SHOPDEF bool shop_sget(unsigned char name, size_t idx, void *dst);

// shop_len - get the length of option value array
// @name: option name
SHOPDEF size_t shop_len(unsigned char name);

// shop_foreach - iterate the option values
// @name: option name
// @idx: index name
// @dst: pointer to destination
#define shop_foreach(name, idx, dst) for (size_t idx = 0; shop_sget(name, idx, dst); idx++)

// shop_verbose - print all options' state as a table
SHOPDEF void shop_verbose(void);

// shop_help - print the help message
// Note: '*' before the option means it require a parameter
SHOPDEF void shop_help(void);

#endif // SHOP_H

#ifdef SHOP_IMPLEMENTATION

static unsigned char shop__map[255] = {0};
static struct {
    shop_option_t *items;
    size_t len;
    size_t cap;
} shop__options = {0};

#define shop__push(vec, item)                                                       \
    do {                                                                            \
        if ((vec)->len + 1 > (vec)->cap) {                                          \
            (vec)->cap = (vec)->cap < 8 ? 8 : 2*(vec)->cap;                         \
            (vec)->items = realloc((vec)->items, (vec)->cap*sizeof(*(vec)->items)); \
            SHOP_ASSERT((vec)->items, "out of memory");                             \
        }                                                                           \
        (vec)->items[(vec)->len++] = (item);                                        \
    } while (0)

SHOPDEF void shop_set(const char *opt_str) {
    size_t len = strlen(opt_str);
    char input[len+1];

    memcpy(input, opt_str, len);
    input[len] = '\0';

    char *p = input;
    const char *token;
    while ((token = strtok(p, ": "))) {
        // printf("token: '%s'\n", token);
        for (size_t i = 0; i < strlen(token); i++) {
            shop_option_t opt = { .name = token[i], .take_arg = false };
            if (i == strlen(token) - 1) opt.take_arg = true;
            shop__push(&shop__options, opt);
            shop__map[opt.name] = (unsigned char) shop__options.len;
        }
        p = NULL;
    }

    // check if the last option requires argument
    if (opt_str[len-1] != ':') {
        shop_option_t *last_option = &shop__options.items[shop__options.len-1];
        last_option->take_arg = false;
    }
}

SHOPDEF void shop_free(void) {
    for (size_t i = 0; i < shop__options.len; i++) {
        const shop_option_t *opt_ptr = &shop__options.items[i];
        if (opt_ptr->items) free(opt_ptr->items);
    }
    if (shop__options.items) free(shop__options.items);
    memset(&shop__options, 0, sizeof(shop__options));
    memset(&shop__map, 0, sizeof(shop__map));
}

static shop_option_t *shop__find(unsigned char name) {
    unsigned char idx = shop__map[(unsigned int) name];
    if (idx == 0) return NULL;
    return &shop__options.items[idx - 1];
}

SHOPDEF void shop_desc(unsigned char name, const char *scan_fmt, const char *info) {
    shop_option_t *opt_ptr = shop__find(name);
    opt_ptr->scan_fmt = scan_fmt;
    opt_ptr->info = info;
}

SHOPDEF const shop_option_t *shop_use(unsigned char name) {
    const shop_option_t *opt_ptr = shop__find(name);
    if (opt_ptr && opt_ptr->used) return opt_ptr;
    return NULL;
}

SHOPDEF void shop_track(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        // skip non-option arg
        if (arg[0] != '-') continue;

        // handle current option (may combined, -rhp)
        bool has_param_in_next_arg = false;
        for (int j = 1; arg[j] != '\0'; j++) {
            char name = arg[j];
            shop_option_t *opt = shop__find(name);
            SHOP_ASSERT(opt, "unknown option: '-%c'", name);
            opt->used = true;

            // check if the option param is in next cmdline arg
            // -f data.txt or -fdata.txt
            if (opt->take_arg) {
                if (arg[j+1] != '\0') shop__push(opt, arg + j + 1);
                else has_param_in_next_arg = true;
                break;
            }
        }

        // handle next option argument if it is
        if (has_param_in_next_arg) {
            i++; // skip current option
            SHOP_ASSERT(i < argc, "option '%s' require argument but not supply", arg);
            // find the option which need this argument
            for (int j = 1; arg[j] != '\0'; j++) {
                shop_option_t *opt = shop__find(arg[j]);
                if (opt && opt->take_arg) {
                    shop__push(opt, argv[i]);
                    break;
                }
            }
        }
    }
}

SHOPDEF void shop_verbose(void) {
    const int DESC_WIDTH = 20;
    const int ARG_WIDTH = 10;

    printf("\n%-6s  %-*s  %-6s  %-10s  %-*s\n",
           "Option", DESC_WIDTH, "Description", "Used", "Type", ARG_WIDTH, "Argument");
    printf("%-6s  %-*s  %-6s  %-10s  %-*s\n",
           "------", DESC_WIDTH, "-----------", "----", "----", ARG_WIDTH, "--------");

    for (size_t i = 0; i < shop__options.len; i++) {
        const shop_option_t *opt_ptr = &shop__options.items[i];

        char short_desc[DESC_WIDTH + 4];
        const char *desc = opt_ptr->info ? opt_ptr->info : "";
        if (strlen(desc) > (size_t) DESC_WIDTH) {
            strncpy(short_desc, desc, DESC_WIDTH - 3);
            short_desc[DESC_WIDTH - 3] = '\0';
            strcat(short_desc, "...");
        } else {
            strcpy(short_desc, desc);
        }
        printf("-%c      %-*s  %-6s  %-10s  ",
               opt_ptr->name,
               DESC_WIDTH, short_desc,
               opt_ptr->used ? "yes" : "no",
               opt_ptr->take_arg ? "with-arg" : "flag");

        char short_arg[ARG_WIDTH + 4];
        const char *sep = "";
        for (size_t j = 0; j < opt_ptr->len; j++) {
            const char *arg = opt_ptr->items[j];
            if (strlen(arg) > (size_t) ARG_WIDTH) {
                strncpy(short_arg, arg, ARG_WIDTH - 3);
                short_arg[ARG_WIDTH - 3] = '\0';
                strcat(short_arg, "...");
            } else {
                strcpy(short_arg, arg);
            }
            printf("%s%-*s", sep, (int) strlen(short_arg), short_arg);
            sep = ",";
        }

        printf("\n");
    }
}

SHOPDEF void shop_help(void) {
    for (size_t i = 0; i < shop__options.len; i++) {
        const shop_option_t *opt_ptr = &shop__options.items[i];
        printf("%c -%c    %s\n", opt_ptr->take_arg ? '*' : ' ', opt_ptr->name, opt_ptr->info);
    }
}

SHOPDEF size_t shop_len(unsigned char name) {
    const shop_option_t *opt_ptr = shop__find(name);
    return opt_ptr->len;
}

SHOPDEF bool shop_sget(unsigned char name, size_t idx, void *dst) {
    const shop_option_t *opt_ptr = shop__find(name);
    if (!opt_ptr || !opt_ptr->used || !opt_ptr->take_arg
     || !opt_ptr->scan_fmt || opt_ptr->scan_fmt[0] == '\0'
     || idx >= opt_ptr->len) {
        return false;
    }

    const char *value = opt_ptr->items[idx];
    const char *scan_fmt = opt_ptr->scan_fmt;

    if (strcmp(scan_fmt, "%s") == 0) {
        *((const char**) dst) = value;
    } else if (strcmp(scan_fmt, "%b") == 0) {
        bool res = (strcmp(value, "true") == 0 ||
                    strcmp(value, "yes") == 0 ||
                    strcmp(value, "1") == 0 ||
                    strcmp(value, "on") == 0);
        memcpy(dst, &res, sizeof(bool));
    } else {
        if (sscanf(value, scan_fmt, dst) != 1) return false;
    }

    return true;
}

#endif // SHOP_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under MIT license.
------------------------------------------------------------------------------
Copyright (c) 2025 Dylaris
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, COOKING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
