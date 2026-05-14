#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "./_getopt.h"

char* optarg = NULL;
int optind = 0, opterr = 1, optopt = 0;

static bool initialized = false;
static ordering_t ordering;
static char* nextchar = NULL;
static int first_nonopt;
static int last_nonopt;

#if (defined(__is_libk))
    // Silence errors in libk
    #define printf_err(...) ((void)0)
#else
    // TODO: this should only print to stderr
    #define printf_err(...) if (opterr) { printf(__VA_ARGS__); }
#endif

static void exchange(char **argv, int bottom, int middle, int top)
{
    while (top > middle && middle > bottom) {
        if ((top - middle) > (middle - bottom)) {
            int len = middle - bottom;
            for (int i = 0; i < len; ++i) {
                char *tmp = argv[bottom + i];
                argv[bottom + i] = argv[top - len + i];
                argv[top - len + i] = tmp;
            }
            top -= len;
        } else {
            int len = top - middle;
            for (int i = 0; i < len; ++i) {
                char *tmp = argv[bottom + i];
                argv[bottom + i] = argv[middle + i];
                argv[middle + i] = tmp;
            }
            bottom += len;
        }
    }
}

static const struct option* match_longopt(
    const char *name,
    size_t namelen,
    const struct option *longopts,
    int *idx,
    bool *ambiguous
) {
    const struct option *found = NULL;
    *idx = -1;
    *ambiguous = false;

    for (int i = 0; longopts && longopts[i].name; ++i) {
        if (strncmp(longopts[i].name, name, namelen) == 0) {
            if (strlen(longopts[i].name) == namelen) {
                *idx = i;
                *ambiguous = false;
                return &longopts[i];
            }
            if (found != NULL) {
                *ambiguous = true;
            } else {
                found = &longopts[i];
                *idx = i;
            }
        }
    }

    return found;
}

static void init(const char *optstring) {
    first_nonopt = last_nonopt = optind;
    nextchar = NULL;

    if (optstring[0] == '-') {
        ordering = RETURN_IN_ORDER;
    }
    else if (optstring[0] == '+') {
        ordering = REQUIRE_ORDER;
    }
    #if !(defined(__is_libk))
    else if (getenv("POSIXLY_CORRECT")) {
        ordering = REQUIRE_ORDER;
    }
    #endif
    else {
        ordering = PERMUTE;
    }

    initialized = 1;
}

int __getopt_internal(
    int argc,
    char *argv[],
    const char *optstring,
    const struct option *longopts,
    int *longindex,
    bool long_only
) {
    optarg = NULL;

    if (argc <= 0 || argv == NULL || optstring == NULL) {
        return -1;
    }

    if (optind == 0) {
        optind = 1;
    }

    if (!initialized || (optind == 1 && nextchar == NULL)) {
        init(optstring);
    }

    if (*optstring == '+' || *optstring == '-') {
        ++optstring;
    }

    while (1) {
        if (nextchar == NULL || *nextchar == '\0') {
            nextchar = NULL;

            if (ordering == PERMUTE) {
                if (last_nonopt > optind) {
                    last_nonopt = optind;
                }

                if (first_nonopt > optind) {
                    first_nonopt = optind;
                }

                if (first_nonopt != last_nonopt && last_nonopt != optind) {
                    exchange(argv, first_nonopt, last_nonopt, optind);
                }
                else if (last_nonopt != optind) {
                    first_nonopt = optind;
                }

                while (optind < argc &&
                       (argv[optind][0] != '-' || argv[optind][1] == '\0')) {
                    optind++;
                }

                last_nonopt = optind;
            }

            if (optind >= argc) {
                return -1;
            }

            if (strcmp(argv[optind], "--") == 0) {
                optind++;
                if (ordering == PERMUTE && first_nonopt != last_nonopt) {
                    exchange(argv, first_nonopt, last_nonopt, optind - 1);
                }
                return -1;
            }

            if (argv[optind][0] != '-' || argv[optind][1] == '\0') {
                if (ordering == REQUIRE_ORDER) {
                    return -1;
                }
                if (ordering == RETURN_IN_ORDER) {
                    optarg = argv[optind++];
                    return 1;
                }
                continue;
            }

            if (longopts) {
                bool try_long = false;

                if (argv[optind][1] == '-') {
                    try_long = true;
                    nextchar = argv[optind] + 2;
                } else if (long_only &&
                           (argv[optind][2] != '\0' ||
                            strchr(optstring, argv[optind][1]) == NULL)) {
                    try_long = true;
                    nextchar = argv[optind] + 1;
                }

                if (try_long) {
                    char *eq = strchr(nextchar, '=');
                    size_t namelen = eq
                        ? (size_t)(eq - nextchar)
                        : strlen(nextchar);
                    int idx = -1;
                    bool ambiguous = false;
                    const struct option *o = match_longopt(
                        nextchar,
                        namelen,
                        longopts,
                        &idx,
                        &ambiguous
                    );

                    if (ambiguous) {
                        printf_err(
                            "%s: option '%s' is ambiguous\n",
                            argv[0],
                            argv[optind]
                        );
                        optind++;
                        optopt = 0;
                        nextchar = NULL;
                        return '?';
                    }

                    if (o) {
                        if (longindex) {
                            *longindex = idx;
                        }

                        if (o->has_arg == no_argument) {
                            if (eq) {
                                printf_err(
                                    "%s: option '--%s' doesn't allow an argument\n",
                                    argv[0],
                                    o->name
                                );
                                optind++;
                                nextchar = NULL;
                                optopt = o->val;
                                return '?';
                            }
                            optarg = NULL;
                        } else if (o->has_arg == required_argument) {
                            if (eq) {
                                optarg = eq + 1;
                            } else if (optind + 1 < argc) {
                                optarg = argv[++optind];
                            } else {
                                if (optstring[0] != ':') {
                                    printf_err(
                                        "%s: option '--%s' requires an argument\n",
                                        argv[0],
                                        o->name
                                    );
                                }
                                optind++;
                                nextchar = NULL;
                                optopt = o->val;
                                return (optstring[0] == ':') ? ':' : '?';
                            }
                        } else { // optional_argument
                            optarg = eq ? eq + 1 : NULL;
                        }

                        optind++;
                        nextchar = NULL;

                        if (o->flag) {
                            *o->flag = o->val;
                            return 0;
                        }
                        return o->val;
                    }

                    if (argv[optind][1] == '-') {
                        printf_err(
                            "%s: unrecognized option '%s'\n",
                            argv[0],
                            argv[optind]
                        );
                        optind++;
                        nextchar = NULL;
                        optopt = 0;
                        return '?';
                    }

                    nextchar = argv[optind] + 1;
                } else {
                    nextchar = argv[optind] + 1;
                }
            } else {
                nextchar = argv[optind] + 1;
            }
        }

        char c = *nextchar++;
        const char *spec = strchr(optstring, c);

        if (*nextchar == '\0') {
            optind++;
        }

        if (!spec || c == ':') {
            if (optstring[0] != ':') {
                printf_err( "%s: invalid option -- '%c'\n", argv[0], c);
            }
            optopt = c;
            return '?';
        }

        if (spec[1] == ':') {
            if (spec[2] == ':') { // optional arg
                if (*nextchar != '\0') {
                    optarg = nextchar;
                    optind++;
                } else {
                    optarg = NULL;
                }
                nextchar = NULL;
            } else { // required arg
                if (*nextchar != '\0') {
                    optarg = nextchar;
                    optind++;
                } else if (optind < argc) {
                    optarg = argv[optind++];
                } else {
                    if (optstring[0] != ':') {
                        printf_err(
                            "%s: option requires an argument -- '%c'\n",
                            argv[0],
                            c
                        );
                    }
                    optopt = c;
                    nextchar = NULL;
                    return (optstring[0] == ':') ? ':' : '?';
                }
                nextchar = NULL;
            }
        }

        return c;
    }
}
