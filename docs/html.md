---
title = HTML
---
# HTML
----------

This header provides an easy (although debatably sane) way to generate html in c.

There are three types of tags:
* One and done tags, like `<img>` or `<hr>` which only have an opening tag
* Basic tags which follow this format: `<tag>content</tag>`
* Tags where the content is probably other tags

You can open and close any tags using `tagBeg` and `tagEnd`, you can also set attributes like this:

```c
tagBeg("div", .class="content", .id="main");
```

Finally, you can use the `htmlRaw` macro to write raw html.

Example code:
```c
str_t generate_page(arena_t *arena, page_t *data) {
    str_t out = STR_EMPTY;

    htmlBeg(arena, &out);
        headBeg();
            title(data->title);
            htmlRaw(<script src="script.js"></script>)
            style(data->css);
        headEnd();
        bodyBeg();
            divBeg(.id="main");
                h1("Hello World!");
                hr();
                p("Some content blah blah");
                img(.src="image.png");
            divEnd();
        bodyEnd();
    htmlEnd();

    return out;
}
```