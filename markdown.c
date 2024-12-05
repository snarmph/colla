#include "markdown.h"

#include "arena.h"
#include "str.h"
#include "strstream.h"
#include "file.h"
#include "ini.h"
#include "tracelog.h"

#ifndef MD_LIST_MAX_DEPTH
    #define MD_LIST_MAX_DEPTH 8
#endif

typedef struct {
    struct {
        int indent;
        int count;
        bool list_is_ordered[MD_LIST_MAX_DEPTH];
    } list;
    struct {
        bool is_in_block;
        strview_t lang;
    } code;
    bool is_bold;
    bool is_italic;
    bool is_in_paragraph;
    strview_t raw_line;
    md_options_t *options;
    md_parser_t *curparser;
} markdown_ctx_t;

static void markdown__parse_config(arena_t *arena, instream_t *in, ini_t *out);
static int markdown__count_chars(strview_t *line, char c);
static void markdown__parse_line(markdown_ctx_t *md, strview_t line, outstream_t *out, bool add_newline, bool is_line_start);
static strview_t markdown__parse_header(markdown_ctx_t *md, strview_t line, outstream_t *out);
static strview_t markdown__parse_ulist_or_line(markdown_ctx_t *md, strview_t line, outstream_t *out);
static strview_t markdown__parse_olist(markdown_ctx_t *md, strview_t line, outstream_t *out);
static strview_t markdown__parse_code_block(markdown_ctx_t *md, strview_t line, outstream_t *out);
static bool markdown__try_parse_url(instream_t *in, strview_t *out_url, strview_t *out_text);
static void markdown__empty_line(markdown_ctx_t *md, outstream_t *out);
static void markdown__close_list(markdown_ctx_t *md, outstream_t *out);
static void markdown__escape(strview_t view, outstream_t *out);

str_t markdown(arena_t *arena, arena_t scratch, strview_t filename, md_options_t *options) {
    str_t text = fileReadWholeStr(&scratch, filename);
    return markdownStr(arena, strv(text), options);
}

str_t markdownStr(arena_t *arena, strview_t markdown_str, md_options_t *options) {
    instream_t in = istrInitLen(markdown_str.buf, markdown_str.len);

    markdown__parse_config(arena, &in, options ? options->out_config : NULL);

    outstream_t out = ostrInit(arena);

    markdown_ctx_t md = {
        .list = {
            .indent = -1,
        },
        .options = options,
    };

    while (!istrIsFinished(in)) {
        md.raw_line = istrGetLine(&in);
        markdown__parse_line(&md, strvTrimLeft(md.raw_line), &out, true, true);
    }

    markdown__empty_line(&md, &out);

    return ostrAsStr(&out);
}

// == PRIVATE FUNCTIONS ==================================================

static void markdown__parse_config(arena_t *arena, instream_t *in, ini_t *out) {
    strview_t first_line = strvTrim(istrGetLine(in));
    if (!strvEquals(first_line, strv("---"))) {
        istrRewind(in);
        return;
    }
    
    strview_t ini_data = strvInitLen(in->cur, 0);
    usize data_beg = istrTell(*in);
    while (!istrIsFinished(*in)) {
        strview_t line = istrGetViewEither(in, strv("\r\n"));
        if (strvEquals(strvTrim(line), strv("---"))) {
            break;
        }
        istrSkipWhitespace(in);
    }
    usize data_end = istrTell(*in);
    ini_data.len = data_end - data_beg - 3;

    if (out) {
        // allocate the string as ini_t only as a copy
        str_t ini_str = str(arena, ini_data);
        *out = iniParseStr(arena, strv(ini_str), NULL);
    }
}

static int markdown__count_chars(strview_t *line, char c) {
    strview_t temp = *line;
    int n = 0;
    while (strvFront(temp) == c) {
        n++;
        temp = strvRemovePrefix(temp, 1);
    }

    *line = temp;
    return n;
}

static strview_t markdown__parse_header(markdown_ctx_t* md, strview_t line, outstream_t *out) {
    int n = markdown__count_chars(&line, '#');
    line = strvTrimLeft(line); 
    
    ostrPrintf(out, "<h%d>", n);
    markdown__parse_line(md, line, out, false, false);
    ostrPrintf(out, "</h%d>", n);

    return STRV_EMPTY;
}

static strview_t markdown__parse_ulist_or_line(markdown_ctx_t *md, strview_t line, outstream_t *out) {
    // check if there is anything before this character, if there is
    // it means we're in the middle of a line and we should ignore
    strview_t prev = strvSub(md->raw_line, 0, line.buf - md->raw_line.buf);
    int space_count;
    for (space_count = 0; space_count < prev.len; ++space_count) {
        if (prev.buf[space_count] != ' ') break;
    }

    if (space_count < prev.len) {
        return line;
    }

    // if its only * or -, this is a list
    if (line.len > 1 && line.buf[1] == ' ') {
        strview_t raw_line = md->raw_line;
        int cur_indent = markdown__count_chars(&raw_line, ' ');
        // start of list
        if (md->list.indent < cur_indent) {
            if (md->list.count >= MD_LIST_MAX_DEPTH) {
                fatal("markdown: too many list levels, max is %d, define MD_LIST_MAX_DEPTH to an higher number", MD_LIST_MAX_DEPTH);
            }
            md->list.list_is_ordered[md->list.count++] = false;
            ostrPuts(out, strv("<ul>\n"));
        }
        else if (md->list.indent > cur_indent) {
            markdown__close_list(md, out);
        }

        md->list.indent = cur_indent;
        ostrPuts(out, strv("<li>"));
        markdown__parse_line(md, strvRemovePrefix(line, 2), out, false, false);
        ostrPuts(out, strv("</li>"));
        goto read_whole_line;
    }

    // check if it is an <hr>
    char hr_char = strvFront(line);
    strview_t hr = strvTrim(line);
    bool is_hr = true;
    for (usize i = 0; i < hr.len; ++i) {
        if (hr.buf[i] != hr_char) {
            is_hr = false;
            break;
        }
    }

    if (is_hr) {
        ostrPuts(out, strv("<hr>"));
        goto read_whole_line;
    }
    else {
        strview_t to_print = line;
        int n = markdown__count_chars(&line, strvFront(line));
        to_print = strvSub(to_print, 0, n);
        line = strvSub(line, n, SIZE_MAX);
        ostrPuts(out, to_print);
    }

    return line;
read_whole_line:
    return STRV_EMPTY;
}

static strview_t markdown__parse_olist(markdown_ctx_t *md, strview_t line, outstream_t *out) {
    instream_t in = istrInitLen(line.buf, line.len);

    int32 number = 0;
    if (!istrGetI32(&in, &number)) {
        return line;
    }

    if (istrPeek(&in) != '.') {
        return line;
    }

    istrSkip(&in, 1);

    if (istrPeek(&in) != ' ') {
        return line;
    }

    istrSkip(&in, 1);

    strview_t raw_line = md->raw_line;
    int cur_indent = markdown__count_chars(&raw_line, ' ');
    // start of list
    if (md->list.indent < cur_indent) {
        if (md->list.count >= MD_LIST_MAX_DEPTH) {
            fatal("markdown: too many list levels, max is %d, define MD_LIST_MAX_DEPTH to an higher number", MD_LIST_MAX_DEPTH);
        }
        md->list.list_is_ordered[md->list.count++] = true;
        ostrPuts(out, strv("<ol>\n"));
    }
    else if (md->list.indent > cur_indent) {
        markdown__close_list(md, out);
    }

    md->list.indent = cur_indent;
    ostrPuts(out, strv("<li>"));
    markdown__parse_line(md, strvRemovePrefix(line, istrTell(in)), out, false, false);
    ostrPuts(out, strv("</li>"));

    return STRV_EMPTY;
}

static strview_t markdown__parse_code_block(markdown_ctx_t *md, strview_t line, outstream_t *out) {
    strview_t line_copy = line;
    int ticks = markdown__count_chars(&line_copy, '`');

    if (ticks != 3) {
        goto finish;
    }

    if (md->code.is_in_block) {
        md->code.is_in_block = false;
        if (md->curparser) {
            md_parser_t *p = md->curparser;
            if (p->finish) {
                p->finish(p->userdata);
            }
        }
        ostrPuts(out, strv("</ol></code></pre>\n"));
        line = line_copy;
        goto finish;
    }

    instream_t in = istrInitLen(line_copy.buf, line_copy.len);
    strview_t lang = istrGetLine(&in);

    if (!strvIsEmpty(lang)) {
        md->curparser = NULL;
        md_options_t *opt = md->options;
        if (opt) {
            for (int i = 0; i < opt->parsers_count; ++i) {
                if (strvEquals(lang, opt->parsers->lang)) {
                    md->curparser = &opt->parsers[i];
                    break;
                }
            }
        }

        if (!md->curparser) {
            warn("markdown: no parser found for code block in language (%v)", lang);
        }
        else {
            md_parser_t *p = md->curparser;
            if (p->init) {
                p->init(p->userdata);
            }
        }
    }

    ostrPuts(out, strv("<pre><code><ol>"));
    md->code.is_in_block = true;

    return STRV_EMPTY;
finish:
    return line;
}

static bool markdown__try_parse_url(instream_t *in, strview_t *out_url, strview_t *out_text) {
    istrSkip(in, 1); // skip [

    strview_t text = istrGetView(in, ']');
    istrSkip(in, 1); // skip ]

    strview_t url = STRV_EMPTY;
    if (istrPeek(in) == '(') {
        istrSkip(in, 1); // skip (
        url = istrGetView(in, ')');
        istrSkip(in, 1); // skip )
    }

    bool success = !strvIsEmpty(url);

    if (success) {
        *out_url = url;
        *out_text = text;
    }

    return success;
}

static void markdown__parse_line(markdown_ctx_t *md, strview_t line, outstream_t *out, bool add_newline, bool is_line_start) {
    if (md->code.is_in_block && strvFront(line) != '`') {
        md_parser_t *p = md->curparser;
        if (p && p->callback) {
            p->callback(md->raw_line, out, p->userdata);
        }
        else {
            ostrPrintf(out, "%v\n", md->raw_line);
        }
        return;
    }

    if (strvIsEmpty(line)) {
        markdown__empty_line(md, out);
        return;
    }

    switch (strvFront(line)) {
        // header
        case '#':
            line = markdown__parse_header(md, line, out);
            break;
        // unordered list or <hr>
        case '-': case '*': case '_': 
            line = markdown__parse_ulist_or_line(md, line, out);
            break;
        // ordered list
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
            line = markdown__parse_olist(md, line, out);
            break;
        // code block
        case '`': 
            line = markdown__parse_code_block(md, line, out);
            break;
        default:
            break;
    }

    if (!strvIsEmpty(line) && is_line_start && !md->is_in_paragraph) {
        md->is_in_paragraph = true;
        ostrPuts(out, strv("<p>\n"));
    }

    for (usize i = 0; i < line.len; ++i) {
        switch (line.buf[i]) {
            // escape next character
            case '\\': 
                if (++i < line.len) {
                    ostrPutc(out, line.buf[i]);
                }
                break;
            // bold or italic
            case '*': 
            {
                strview_t sub = strvSub(line, i, SIZE_MAX);
                int n = markdown__count_chars(&sub, '*');
                
                bool is_both   = n >= 3;
                bool is_italic = n == 1 || is_both;
                bool is_bold   = n == 2 || is_both;

                if (is_italic) {
                    ostrPrintf(out, "<%s>", md->is_italic ? "/i" : "i");
                    md->is_italic = !md->is_italic;
                }
                if (is_bold) {
                    ostrPrintf(out, "<%s>", md->is_bold ? "/b" : "b");
                    md->is_bold = !md->is_bold;
                }
                if (is_both) {
                    for (int k = 3; k < n; ++k) {
                        ostrPutc(out, '*');
                    }
                }
                i += n - 1;
                break;
            }
            // url
            case '[':
            {
                instream_t in = istrInitLen(line.buf + i, line.len - i);
                strview_t url = STRV_EMPTY;
                strview_t text = STRV_EMPTY;
                if (markdown__try_parse_url(&in, &url, &text)) {
                    ostrPrintf(out, "<a href=\"%v\">%v</a>", url, strvIsEmpty(text) ? url : text);
                    i += istrTell(in) - 1;
                }
                else{
                    ostrPutc(out, line.buf[i]);
                }
                break;
            }
            // image
            case '!': 
            {
                instream_t in = istrInitLen(line.buf + i, line.len - i);
                strview_t url = STRV_EMPTY;
                strview_t text = STRV_EMPTY;

                istrSkip(&in, 1); // skip !

                if (markdown__try_parse_url(&in, &url, &text)) {
                    ostrPrintf(out, "<img src=\"%v\"", url);
                    if (!strvIsEmpty(text)) {
                        ostrPrintf(out, " alt=\"%v\"", text);
                    }
                    ostrPutc(out, '>');
                    i += istrTell(in) - 1;
                }
                else{
                    ostrPutc(out, line.buf[i]);
                }
                break;
            }
            // code block
            case '`':
            {
                bool is_escaped = false;
                if ((i + 1) < line.len) {
                    is_escaped = line.buf[i + 1] == '`';
                }
                instream_t in = istrInitLen(line.buf + i, line.len - i);

                istrSkip(&in, is_escaped ? 2 : 1); // skip `
                ostrPuts(out, strv("<code>"));
                while (!istrIsFinished(in)) {
                    strview_t code = istrGetView(&in, '`');
                    markdown__escape(code, out);
                    if (!is_escaped || istrPeek(&in) == '`') {
                        break;
                    }
                    ostrPutc(out, '`');
                }
                ostrPuts(out, strv("</code>"));
                i += istrTell(in);
                break;
            }
            default:
                ostrPutc(out, line.buf[i]);
                break;
        }
    }

    if (add_newline && !md->code.is_in_block) {
        ostrPutc(out, '\n');
    }
}

static void markdown__empty_line(markdown_ctx_t *md, outstream_t *out) {
    // close lists
    while (md->list.count > 0) {
        if (md->list.list_is_ordered[--md->list.count]) {
            ostrPuts(out, strv("</ol>\n"));
        }
        else {
            ostrPuts(out, strv("</ul>\n"));
        }
    }
    md->list.indent = -1;

    // close paragraph
    if (md->is_in_paragraph) {
        ostrPuts(out, strv("</p>\n"));
    }
    md->is_in_paragraph = false;
}

static void markdown__close_list(markdown_ctx_t *md, outstream_t *out) {
    if (md->list.count > 0) {
        if (md->list.list_is_ordered[--md->list.count]) {
            ostrPuts(out, strv("</ol>\n"));
        }
        else {
            ostrPuts(out, strv("</ul>\n"));
        }
    }
}

static void markdown__escape(strview_t view, outstream_t *out) {
    for (usize i = 0; i < view.len; ++i) {
        switch (view.buf[i]){
            case '&':
                ostrPuts(out, strv("&amp"));
                break;
            case '<':
                ostrPuts(out, strv("&lt"));
                break;
            case '>':
                ostrPuts(out, strv("&gt"));
                break;
            default:
                ostrPutc(out, view.buf[i]);
                break;
        }
    }
}