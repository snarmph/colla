---
title = Format
---
# Format
----------

Small formatting utility, it has 2 functions (and the `va_list` alternatives):

* `fmtPrint`: equivalent to 
* `fmtBuffer`

It uses [stb_sprintf](https://github.com/nothings/stb/blob/master/stb_sprintf.h) under the hood, but it also has support for printing buffers using `%v` (structures that are a pair of pointer/size)  

This means it can print strings and [string views](/str).

In 

Usage example:

```c
int main() {
    strview_t v = strv("world");
    char buffer[1024] = {0};

    fmtPrint("Hello %v!", v);
    fmtBuffer(buffer, sizeof(buffer), "Hello %v!", v);
}
```