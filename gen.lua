local outfile = "colla.h"
local colladir = "src/"

local function hasArg(value)
    for k,v in pairs(arg) do
        if v == value then 
            return true 
        end
    end
    return false
end

local use_namespace = hasArg("-namespace")

os.remove(outfile)

local function cat(f)
    local fp = io.open(f)
    local text = fp:read("a")
    fp:close()
    return text
end

str = [[
/*
    colla.h -- All colla libraries in a single header
    Do the following in *one* C file to create the implementation
      #define COLLA_IMPL
    Use the following in the same C file for options
      #define COLLA_NO_THREADS // don't include the threads module
      #define COLLA_NO_NET     // don't include networking stuff
*/
]]

str = str .. cat(colladir .. "collatypes.h") .. "\n"
str = str .. cat(colladir .. "tracelog.h") .. "\n"
str = str .. cat(colladir .. "str.h") .. "\n"
str = str .. cat(colladir .. "vec.h") .. "\n"
str = str .. cat(colladir .. "hashmap.h") .. "\n"
str = str .. cat(colladir .. "utf8.h") .. "\n"
str = str .. cat(colladir .. "ini.h") .. "\n"
str = str .. cat(colladir .. "strstream.h") .. "\n"
str = str .. cat(colladir .. "win32_slim.h") .. "\n"
str = str .. cat(colladir .. "os.h") .. "\n"
str = str .. cat(colladir .. "file.h") .. "\n"
str = str .. cat(colladir .. "dir.h") .. "\n"

str = str .. "#ifndef COLLA_NO_NET\n"
    str = str .. cat(colladir .. "socket.h") .. "\n"
    str = str .. cat(colladir .. "http.h") .. "\n"
str = str .. "#endif // COLLA_NO_NET\n"

str = str .. "#if !defined(__TINYC__) && !defined(COLLA_NO_THREADS)\n"
    str = str .. cat(colladir .. "cthreads.h") .. "\n"
str = str .. "#endif // !defined(__TINYC__) && !defined(COLLA_NO_THREADS)\n"

str = str .. "#ifdef COLLA_IMPL\n"
    str = str .. cat(colladir .. "tracelog.c") .. "\n"
    str = str .. cat(colladir .. "strstream.c") .. "\n"
    str = str .. cat(colladir .. "str.c") .. "\n"
    str = str .. cat(colladir .. "hashmap.c") .. "\n"
    str = str .. cat(colladir .. "utf8.c") .. "\n"
    str = str .. cat(colladir .. "ini.c") .. "\n"
    str = str .. cat(colladir .. "os.c") .. "\n"
    str = str .. cat(colladir .. "file.c") .. "\n"
    str = str .. cat(colladir .. "dir.c") .. "\n"

    str = str .. "#ifndef COLLA_NO_NET\n"
        str = str .. cat(colladir .. "socket.c") .. "\n"
        str = str .. cat(colladir .. "http.c") .. "\n"
    str = str .. "#endif // COLLA_NO_NET\n"

    str = str .. "#if !defined(__TINYC__) && !defined(COLLA_NO_THREADS)\n"
        str = str .. cat(colladir .. "cthreads.c") .. "\n"
    str = str .. "#endif // !defined(__TINYC__) && !defined(COLLA_NO_THREADS)\n"
str = str .. "#endif /* COLLA_IMPL */\n"

-- remove includes
str = str:gsub('#include "([^"]+)"', '/* #include "%1" */')

str = str .. string.format([[
/*
    MIT License
    Copyright (c) 1994-2019 Lua.org, PUC-Rio.
    Copyright (c) 2020-%s snarmph.
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
]], os.date("%Y"))

local f = io.open(outfile, "w")
f:write(str)
f:close()