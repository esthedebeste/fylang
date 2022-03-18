include "c/string"

type str = *char
fun(str) length()
    strlen(this)