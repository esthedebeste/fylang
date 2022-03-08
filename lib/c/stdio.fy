// stdio.h
include "stddef.fy"

// these are only ever referred to in pointers within stdio so we can unknown them for the time being. (todo: type them properly)
type FILE = unknown 
type fpos_t = unknown
type off_t = unknown

declare const stderr: *FILE
declare const stdout: *FILE
declare const stdin: *FILE

declare fun clearerr(arg: *FILE): void
declare fun ctermid(arg: *char): *char
declare fun cuserid(arg: *char): *char
declare fun fclose(arg: *FILE): int
declare fun fdopen(arg: int, arg: *char): *FILE
declare fun feof(arg: *FILE): int
declare fun ferror(arg: *FILE): int
declare fun fflush(arg: *FILE): int
declare fun fgetc(arg: *FILE): int
declare fun fgetpos(arg: *FILE, arg: *fpos_t): int
declare fun fgets(arg: *char, arg: int, arg: *FILE): *char
declare fun fileno(arg: *FILE): int
declare fun flockfile(arg: *FILE): void
declare fun fopen(arg: *char, arg: *char): *FILE
declare fun fputc(arg: char, arg: *FILE): int
declare fun fputs(arg: *char, arg: *FILE): int
declare fun fread(arg: *void, arg: size_t, arg: size_t, arg: *FILE): size_t
declare fun freopen(arg: *char, arg: *char, arg: *FILE): *FILE
declare fun fseek(arg: *FILE, arg: long, arg: int): int
declare fun fseeko(arg: *FILE, arg: off_t, arg: int): int
declare fun fsetpos(arg: *FILE, arg: *fpos_t): int
declare fun ftell(arg: *FILE): long
declare fun ftello(arg: *FILE): off_t
declare fun ftrylockfile(arg: *FILE): int
declare fun funlockfile(arg: *FILE): void
declare fun fwrite(arg: *void, arg: size_t, arg: size_t, arg: *FILE): size_t
declare fun getc(arg: *FILE): int
declare fun getchar(arg: void): int
declare fun getc_unlocked(arg: *FILE): int
declare fun getchar_unlocked(arg: void): int
declare fun getopt(arg: int, arg: **char, arg: char): int
declare fun gets(arg: *char): *char
declare fun getw(arg: *FILE): int
declare fun pclose(arg: *FILE): int
declare fun perror(arg: *char): void
declare fun popen(arg: *char, arg: *char): *FILE
declare fun putc(arg: int, arg: *FILE): int
declare fun putchar(arg: char): int
declare fun putc_unlocked(arg: int, arg: *FILE): int
declare fun putchar_unlocked(arg: char): int
declare fun puts(arg: *char): int
declare fun putw(arg: int, arg: *FILE): int
declare fun remove(arg: *char): int
declare fun rename(arg: *char, arg: *char): int
declare fun rewind(arg: *FILE): void
declare fun setbuf(arg: *FILE, arg: *char): void
declare fun setvbuf(arg: *FILE, arg: *char, arg: int, arg: size_t): int
declare fun tempnam(arg: *char, arg: *char): *char
declare fun tmpfile(arg: void): *FILE
declare fun tmpnam(arg: *char): *char
declare fun ungetc(arg: int, arg: *FILE): int
// TODO: vararg
/*
 declare fun vfprintf(arg: *FILE, arg: *char, arg: va_list): int
 declare fun vprintf(arg: *char, arg: va_list): int
 declare fun vsnprintf(arg: *char, arg: size_t, arg: *char, arg: va_list): int
 declare fun vsprintf(arg: *char, arg: *char, arg: va_list): int
 declare fun fprintf(arg: *FILE, arg: *char, arg: ...): int
 declare fun fscanf(arg: *FILE, arg: *char, arg: ...): int
 declare fun printf(arg: *char, arg: ...): int
 declare fun scanf(arg: *char, arg: ...): int
 declare fun snprintf(arg: *char, arg: size_t, arg: *char, arg: ...): int
 declare fun sprintf(arg: *char, arg: *char, arg: ...): int
 declare fun sscanf(arg: *char, arg: *char, arg: int ...): int
*/