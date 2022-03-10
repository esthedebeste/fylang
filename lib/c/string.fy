// string.h
include "stddef.fy"

declare fun memchr(s: *void, c: *void, n: size_t): *void
declare fun memcmp(s1: *void, s2: *void, n: size_t): int
declare fun memcpy(dest: *void, src: *void, n: size_t): *void
declare fun memmove(dest: *void, src: *void, n: size_t): *void
declare fun memset(ptr: *void, c: int, n: size_t): *void
declare fun strcat(dest: *char, src: *char): *char
declare fun strchr(str: *char, c: int): *char
declare fun strcmp(str1: *char, str2: *char): int
declare fun strcoll(str1: *char, str2: *char): int
declare fun strcpy(dest: *char, src: *char): *char
declare fun strcspn(str: *char, reject: *char): size_t
declare fun strdup(str: *char): *char
declare fun strerror(err: int): *char
declare fun strlen(str: *char): size_t
declare fun strncat(dest: *char, src: *char, n: size_t): *char
declare fun strncmp(str1: *char, str2: *char, n: size_t): int
declare fun strncpy(dest: *char, src: *char, n: size_t): *char
declare fun strpbrk(str: *char, accept: *char): *char
declare fun strrchr(str: *char, c: int): *char
declare fun strspn(str: *char, accept: *char): size_t
declare fun strstr(str: *char, finding: *char): *char
declare fun strtok(str: *char, delimiters: *char): *char
declare fun strxfrm(dest: *char, str: *char, n: size_t): size_t