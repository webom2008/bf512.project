/* Define _Environ needed by getenv() */
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
char *_null_str = "";
char **_Environ = &_null_str;
