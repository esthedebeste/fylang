// string.h
include "stddef.fy"

declare fun memccpy(arg: *void, arg: *void, arg: int, arg: size_t): *void
declare fun memchr(arg: *void, arg: int, arg: size_t): *void
declare fun memcmp(arg: *void, arg: *void, arg: size_t): int
declare fun memcpy(arg: *void, arg: *void, arg: size_t): *void
declare fun memmove(arg: *void, arg: *void, arg: size_t): *void
declare fun memset(arg: *void, arg: int, arg: size_t): *void
declare fun strcat(arg: *char, arg: *char): *char
declare fun strchr(arg: *char, arg: int): *char
declare fun strcmp(arg: *char, arg: *char): int
declare fun strcoll(arg: *char, arg: *char): int
declare fun strcpy(arg: *char, arg: *char): *char
declare fun strcspn(arg: *char, arg: *char): size_t
declare fun strdup(arg: *char): *char
declare fun strerror(arg: int): *char
declare fun strlen(arg: *char): size_t
declare fun strncat(arg: *char, arg: *char, arg: size_t): *char
declare fun strncmp(arg: *char, arg: *char, arg: size_t): int
declare fun strncpy(arg: *char, arg: *char, arg: size_t): *char
declare fun strpbrk(arg: *char, arg: *char): *char
declare fun strrchr(arg: *char, arg: int): *char
declare fun strspn(arg: *char, arg: *char): size_t
declare fun strstr(arg: *char, arg: *char): *char
declare fun strtok(arg: *char, arg: *char): *char
declare fun strtok_r(arg: *char, arg: *char, arg: *char): *char
declare fun strxfrm(arg: *char, arg: *char, arg: size_t): size_t
