// stdio.h
include "stddef.fy"

// these are only ever referred to in pointers within stdio so we can unknown them for the time being. (todo: type them properly)
type FILE = unknown 
type fpos_t = unknown
type off_t = unknown

declare const stderr: *FILE
declare const stdout: *FILE
declare const stdin: *FILE

declare fun remove(filename: *char): int
declare fun rename(old_p: *char, new_p: *char): int
declare fun tmpfile(): *FILE
declare fun tmpnam(s: *char): *char
declare fun fclose(stream: *FILE): int
declare fun fflush(stream: *FILE): int
declare fun fopen(filename: *char, mode: *char): *FILE
declare fun freopen(filename: *char, mode: *char, stream: *FILE): *FILE
declare fun setbuf(stream: *FILE, buf: *char): void
declare fun setvbuf(stream: *FILE, buf: *char, mode: int, size: size_t): int
declare fun fprintf(stream: *FILE, format: *char, __VARARG__): int
declare fun fscanf(stream: *FILE, format: *char, __VARARG__): int
declare fun printf(format: *char, __VARARG__): int
declare fun scanf(format: *char, __VARARG__): int
declare fun snprintf(s: *char, n: size_t, format: *char, __VARARG__): int
declare fun sprintf(s: *char, format: *char, __VARARG__): int
declare fun sscanf(s: *char, format: *char, __VARARG__): int
/* // TODO: va_list
 declare fun vfprintf(stream: *FILE, format: *char, arg: va_list): int
 declare fun vfscanf(stream: *FILE, format: *char, arg: va_list): int
 declare fun vprintf(format: *char, arg: va_list): int
 declare fun vscanf(format: *char, arg: va_list): int
 declare fun vsnprintf(s: *char, n: size_t, format: *char, arg: va_list): int
 declare fun vsprintf(s: *char, format: *char, arg: va_list): int
 declare fun vsscanf(s: *char, format: *char, arg: va_list): int
*/ 
declare fun fgetc(stream: *FILE): int
declare fun fgets(s: *char, n: int, stream: *FILE): *char
declare fun fputc(c: int, stream: *FILE): int
declare fun fputs(s: *char, stream: *FILE): int
declare fun getc(stream: *FILE): int
declare fun getchar(): int
declare fun putc(c: int, stream: *FILE): int
declare fun putchar(c: int): int
declare fun puts(s: *char): int
declare fun ungetc(c: int, stream: *FILE): int
declare fun fread(ptr: *void, size: size_t, nmemb: size_t, stream: *FILE): size_t
declare fun fwrite(ptr: *void, size: size_t, nmemb: size_t, stream: *FILE): size_t
declare fun fgetpos(stream: *FILE, pos: *fpos_t): int
declare fun fseek(stream: *FILE, offset: _long_int, whence: int): int
declare fun fsetpos(stream: *FILE, pos: *fpos_t): int
declare fun ftell(stream: *FILE): _long_int
declare fun rewind(stream: *FILE): void
declare fun clearerr(stream: *FILE): void
declare fun feof(stream: *FILE): int
declare fun ferror(stream: *FILE): int
declare fun perror(s: *char): void