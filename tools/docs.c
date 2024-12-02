#include "../arena.c"
#include "../file.c"
#include "../format.c"
#include "../ini.c"
#include "../str.c"
#include "../strstream.c"
#include "../tracelog.c"
#include "../vmem.c"
#include "../markdown.c"
#include "../highlight.c"
#include "../dir.c"
#include "../socket.c"
#include "../http.c"
#include "../server.c"

#include "../html.h"

const char *raw_css;

typedef struct page_t {
    str_t title;
    str_t url;
    str_t data;
    struct page_t *next;
} page_t;

typedef struct {
    uint8 arenabuf[KB(5)];
    arena_t arena;
    hl_ctx_t *hl;
    int line;
} cparser_ctx_t;

void md_cparser_init(void *udata) {
    cparser_ctx_t *cparser = udata;
    cparser->line = 1;
    if (cparser->hl) {
        return;
    }
    cparser->arena = arenaMake(ARENA_STATIC, sizeof(cparser->arenabuf), cparser->arenabuf);
    cparser->hl = hlInit(&cparser->arena, &(hl_config_t){
        .colors = {
            [HL_COLOR_NORMAL]       = strv("</span>"),
            [HL_COLOR_PREPROC]      = strv("<span class=\"c-preproc\">"),
            [HL_COLOR_TYPES]        = strv("<span class=\"c-types\">"),
            [HL_COLOR_CUSTOM_TYPES] = strv("<span class=\"c-custom-types\">"),
            [HL_COLOR_KEYWORDS]     = strv("<span class=\"c-kwrds\">"),
            [HL_COLOR_NUMBER]       = strv("<span class=\"c-num\">"),
            [HL_COLOR_STRING]       = strv("<span class=\"c-str\">"),
            [HL_COLOR_COMMENT]      = strv("<span class=\"c-cmnt\">"),
            [HL_COLOR_FUNC]         = strv("<span class=\"c-func\">"),
            [HL_COLOR_SYMBOL]       = strv("<span class=\"c-sym\">"),
            [HL_COLOR_MACRO]        = strv("<span class=\"c-macro\">"),
        },
        .flags = HL_FLAG_HTML,
    });
}

void md_cparser_callback(strview_t line, outstream_t *out, void *udata) {
    cparser_ctx_t *cparser = udata;

    arena_t scratch = cparser->arena;
    str_t highlighted = hlHighlight(&scratch, cparser->hl, line);
    ostrPrintf(out, "<li>%v</li>", highlighted);
}

page_t *get_pages(arena_t *arena, strview_t path, strview_t default_page) {
    arena_t scratch = arenaMake(ARENA_VIRTUAL, MB(1));

    dir_t *dir = dirOpen(&scratch, path);
    dir_entry_t *entry = NULL;

    page_t *first = NULL;
    page_t *head = NULL;
    page_t *tail = NULL;

    cparser_ctx_t cparser = {0};

    while ((entry = dirNext(&scratch, dir))) {
        if (entry->type != DIRTYPE_FILE) {
            continue;
        }

        strview_t name, ext;
        fileSplitPath(strv(entry->name), NULL, &name, &ext);

        if (!strvEquals(ext, strv(".md"))) {
            continue;
        }

        str_t fullname = strFmt(&scratch, "%v/%v", path, entry->name);
        str_t markdown_str = fileReadWholeStr(&scratch, strv(fullname));

        str_t md = markdownStr(&scratch, strv(markdown_str), &(md_options_t){
            .parsers = (md_parser_t[]){
                {
                    .init = md_cparser_init,
                    .callback = md_cparser_callback,
                    .userdata = &cparser,
                    .lang = strv("c"),
                },
            },
            .parsers_count = 1,
        });

        page_t *page = alloc(arena, page_t);
        page->data = md;
        page->url = str(arena, name);

        usize line_end = strvFind(strv(markdown_str), '\n', 0);
        strview_t line = strvSub(strv(markdown_str), 0, line_end);
        strview_t page_title = strvTrim(strvRemovePrefix(line, 1));
        page->title = strFmt(arena, "%v", page_title);

        if (!first && strvEquals(name, default_page)) {
            first = page;
        }
        else {
            if (!head) head = page;
            if (tail) tail->next = page;
            tail = page;
        }
    }

    if (first) {
        first->next = head;
        head = first;
    }

    strview_t css = strv(raw_css);

    for_each(page, head) {
        str_t html = STR_EMPTY;
        
        htmlBeg(arena, &html);
            headBeg();
                title(page->title);
                style(css);
            headEnd();
            bodyBeg();
                divBeg(.id="main");
                    divBeg(.class="content");
                        divBeg(.class="pages-container");
                            divBeg(.class="pages");
                                for_each(item, head) {
                                    str_t class   = strFmt(&scratch, "page-item%s", item == page ? " page-current" : "");
                                    str_t href    = strFmt(&scratch, "/%v", item->url);
                                    str_t onclick = strFmt(&scratch, "window.location = \"/%v\"", item->url);
                                    
                                    a(
                                        item->title, 
                                        .href    = href.buf,
                                        .class   = class.buf,
                                        .onclick = onclick.buf
                                    );
                                }
                            divEnd();
                        divEnd();
                        
                        divBeg(.class="document-container");
                            divBeg(.class="document");
                                htmlPuts(page->data);
                                htmlRaw(<div class="document-padding"></div>);
                            divEnd();
                        divEnd();
                    divEnd();
                divEnd();
            bodyEnd();
        htmlEnd();

        page->data = html;
    }

    arenaCleanup(&scratch);

    return head;
}

str_t server_default(arena_t scratch, server_t *server, server_req_t *req, void *userdata) {
    strview_t needle = strv(req->page);
    if (strvFront(needle) == '/') { 
        needle = strvRemovePrefix(strv(req->page), 1);
    }

    page_t *page = userdata;
    while (page) {
        if (strvEquals(strv(page->url), needle)) {
            break;
        }
        page = page->next;
    }

    // if the url is invalid, return the default page
    if (!page) {
        page = userdata;
    }

    return serverMakeResponse(&scratch, 200, strv("text/html"), strv(page->data));
}

str_t server_quit(arena_t scratch, server_t *server, server_req_t *req, void *userdata) {
    serverStop(server);
    return STR_EMPTY;
}

int main() {
    arena_t arena = arenaMake(ARENA_VIRTUAL, MB(1));

    page_t *pages = get_pages(&arena, strv("."), strv("readme"));
    if (!pages) {
        err("could not get pages");
        return 1;
    }

    server_t *s = serverSetup(&arena, 8080, true);
    serverRouteDefault(&arena, s, server_default, pages);
    serverRoute(&arena, s, strv("/quit"), server_quit, NULL);
    serverStart(arena, s);

    arenaCleanup(&arena);
}

//// HTML GENERATION STUFF ///////////////////////////////

const char *raw_css = ""
"html, body, #main {\n"
"    margin: 0;\n"
"    width: 100%;\n"
"    height: 100%;\n"
"    font-family: -apple-system,BlinkMacSystemFont,'Segoe UI','Noto Sans',Helvetica,Arial,sans-serif,'Apple Color Emoji','Segoe UI Emoji';\n"
"    \n"
"    --black:      #121212;\n"
"    --dark-gray:  #212121;\n"
"    --gray:       #303030;\n"
"    --light-gray: #424242;\n"
"    --accent:     #FFA500;\n"
"    /* --accent:     #F98334; */\n"
"\n"
"    --blue:       #45559E;\n"
"    --orange:     #F98334;\n"
"    --yellow:     #FABD2F;\n"
"    --red:        #FB4934;\n"
"    --pink:       #D3869B;\n"
"    --green:      #B8BB26;\n"
"    --azure:      #7FA375;\n"
"    --white:      #FBF1C7;\n"
"    --light-blue: #83A598;\n"
"}\n"
"\n"
"hr {\n"
"    color: white;\n"
"    opacity: 0.5;\n"
"}\n"
"\n"
"a {\n"
"    text-decoration: none;\n"
"    font-weight: bold;\n"
"    color: var(--red);\n"
"}\n"
"\n"
"a:hover {\n"
"    text-decoration: underline;\n"
"}\n"
"\n"
".content {\n"
"    width: 100%;\n"
"    height: 100%;\n"
"    background-color: var(--black);\n"
"    display: flex;\n"
"    gap: 20px;\n"
"    align-items: stretch;\n"
"    color: white;\n"
"    overflow-x: scroll;\n"
"}\n"
"\n"
".pages-container {\n"
"    position: sticky;\n"
"    top: 0;\n"
"    min-width: 200px;\n"
"    background-color: var(--dark-gray);\n"
"    border-right: 1px solid var(--light-gray);\n"
"    box-shadow: 0 0 20px rgba(0, 0, 0, 1.0);\n"
"    overflow-y: scroll;\n"
"}\n"
"\n"
".document-container {\n"
"    display: flex;\n"
"    justify-content: center;\n"
"    width: 100%;\n"
"}\n"
"\n"
".document {\n"
"    width: 100%;\n"
"    max-width: 700px;\n"
"    line-height: 1.5;\n"
"}\n"
"\n"
".document-padding {\n"
"    height: 50px;\n"
"}\n"
"\n"
".pages {\n"
"    position: relative;\n"
"    background-color: var(--dark-gray);\n"
"}\n"
"\n"
".page-item:last-of-type {\n"
"    border-bottom: 0;\n"
"}\n"
"\n"
".page-item {\n"
"    text-decoration: none;\n"
"    display: block;\n"
"    color: rgba(255, 255, 255, 0.5);\n"
"    padding: 0.2em 0 0.2em 1.5em;\n"
"    border-bottom: 1px solid var(--light-gray);\n"
"}\n"
"\n"
".page-item:hover {\n"
"    background-color: var(--light-gray);\n"
"    cursor: pointer;\n"
"    color: white;\n"
"    opacity: 1;\n"
"}\n"
"\n"
".page-current {\n"
"    color: var(--accent);\n"
"    opacity: 1;\n"
"}\n"
"\n"
".page-current:hover {\n"
"    color: var(--accent);\n"
"}\n"
"\n"
".page-spacing {\n"
"    height: 25px; \n"
"}\n"
"\n"
"code {\n"
"    color: var(--accent);\n"
"    background-color: var(--dark-gray);\n"
"    padding: 0.2em 0.5em 0.2em 0.5em;\n"
"    border-radius: 0.5em;\n"
"}\n"
"\n"
"pre {\n"
"    margin: 0;\n"
"    margin-top: 2em;\n"
"    background-color: var(--dark-gray);\n"
"    padding: 16px;\n"
"    border-radius: 0.3em;\n"
"    overflow-x: scroll;\n"
"\n"
"    border: 1px solid var(--light-gray);\n"
"    box-shadow: 0 0 20px rgba(0, 0, 0, 1.0);\n"
"}\n"
"\n"
"pre > code {\n"
"    color: #FBF1C7;\n"
"    padding: 0;\n"
"    background-color: transparent;\n"
"}\n"
"\n"
"pre ol {\n"
"    counter-reset: item;\n"
"    padding-left: 0;\n"
"}\n"
"\n"
"pre li {\n"
"    display: block;\n"
"    margin-left: 0em;\n"
"}\n"
"\n"
"pre li::before {\n"
"    display: inline-block;\n"
"    content: counter(item);\n"
"    counter-increment: item;\n"
"    width: 2em;\n"
"    padding-right: 1.5em;\n"
"    text-align: right;\n"
"}\n"
"\n"
"/* code block colors */\n"
".c-preproc {\n"
"    color: #45559E;\n"
"}\n"
"\n"
".c-types {\n"
"    color: #F98334;\n"
"}\n"
"\n"
".c-custom-types {\n"
"    color: #FABD2F;\n"
"}\n"
"\n"
".c-kwrds {\n"
"    color: #FB4934;\n"
"}\n"
"\n"
".c-num {\n"
"    color: #D3869B;\n"
"}\n"
"\n"
".c-str {\n"
"    color: #B8BB26;\n"
"}\n"
"\n"
".c-cmnt {\n"
"    color: #928374;\n"
"}\n"
"\n"
".c-func {\n"
"    color: #7FA375;\n"
"}\n"
"\n"
".c-sym {\n"
"    color: #FBF1C7;\n"
"}\n"
"\n"
".c-macro {\n"
"    color: #83A598;\n"
"}\n"
"";