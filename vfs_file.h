#pragma once

#include "vfs.h"
#include "tracelog.h"

#define fileExists          vfsFileExists
#define fileOpen            vfsFileOpen
#define fileClose           vfsFileClose
#define fileIsValid         vfsFileIsValid
#define fileSize            vfsFileSize
#define fileReadWhole       vfsFileReadWhole
#define fileReadWholeFP     vfsFileReadWholeFP
#define fileReadWholeStr    vfsFileReadWholeStr
#define fileReadWholeStrFP  vfsFileReadWholeStrFP

#define VFS_FAIL_READONLY(fn)  err("VirtualFS: trying to call function " #fn "which requires writing to files. VirtualFS is read only!")
#define VFS_FAIL_STATELESS(fn) err("VirtualFS: trying to call function " #fn "which requires state. VirtualFS only works on stateless file's commands!")

#define fileDelete(...)          VFS_FAIL_READONLY(fileDelete)
#define filePutc(...)            VFS_FAIL_READONLY(filePutc)
#define filePuts(...)            VFS_FAIL_READONLY(filePuts)
#define filePrintf(...)          VFS_FAIL_READONLY(filePrintf)
#define filePrintfv(...)         VFS_FAIL_READONLY(filePrintfv)
#define fileWrite(...)           VFS_FAIL_READONLY(fileWrite)
#define fileWriteWhole(...)      VFS_FAIL_READONLY(fileWriteWhole)
#define fileGetTime(...)         VFS_FAIL_READONLY(fileGetTime)
#define fileGetTimeFP(...)       VFS_FAIL_READONLY(fileGetTimeFP)
#define fileHasChanged(...)      VFS_FAIL_READONLY(fileHasChanged)

#define fileTell(...)            VFS_FAIL_STATELESS(fileTell)
#define fileRead(...)            VFS_FAIL_STATELESS(fileRead)
#define fileSeek(...)            VFS_FAIL_STATELESS(fileSeek)
#define fileSeekEnd(...)         VFS_FAIL_STATELESS(fileSeekEnd)
#define fileRewind(...)          VFS_FAIL_STATELESS(fileRewind)