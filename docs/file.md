# File
----------

This header provides cross platform file functionality. 
It has all the basics that you can expect which work exactly like the stdio counterparts:

* `fileOpen`
* `fileClose`
* `fileIsValid`
* `fileRead`
* `fileWrite`
* `fileSeekEnd`
* `fileRewind`
* `fileTell`

Then there are a few helpers functions for reading / writing:

* Writing:
  * `filePutc`
  * `filePuts`
  * `filePrintf`
  * `fileWriteWhole`
* Reading:
  * `fileReadWhole`
  * `fileReadWholeStr`

There are also some functions to get info about a file without having to open it:

* `fileExists`
* `fileSize`
* `fileGetTime`
* `fileHasChanged`

And finally, there are some helper functions:

* `fileGetFullPath` (for windows)
* `fileSplitPath` / `fileGetFilename` / `fileGetExtension`
* `fileDelete`
