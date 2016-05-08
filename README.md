#AUDIOSPLIT
This tool splits a multichannel audio file into separate files for each channel.

## Building
The build uses CMake. A (somewhat) recent version of gcc or clang is required in
order to support the C++11 standard. While this has been compiled on OSX, it
should also work on Linux machines.

This project first builds the FFMPEG libraries and links against them. After
cloning the repository and initializing the FFMPEG submodule:

    oliver@canopus:~/repos/audiosplit$ git submodule update --init

This is done by:

    oliver@canopus:~/repos/audiosplit$ mkdir build && cd build
    oliver@canopus:~/repos/audiosplit$ cmake ..
    oliver@canopus:~/repos/audiosplit$ make

Note, if yasm is not detected, building FFMPEG libraries will fail. The CMake
option `FFMPEG_DISABLE_YASM=1` can be passed for a "cripped build" of the FFMPEG
libraries. Alternatively, yasm can be easily installed from
[tortall.net](http://yasm.tortall.net/Download.html) or by running (on OSX)

    oliver@canopus:~$ brew install yasm

## Usage
Usage is basic with no options and the only argument is the input file.
Here is an example:

    oliver@canopus:~/repos/audiosplit/build$ ./audiosplit ../music/Brahms\ -\ Variations.wma

The output files are written in the same directory as the input audio file.
The files are written using AAC and contain a single channel (mono) containing
the channel in the original audio stream. The channel name abbreviation is added
to the output file name before the extension. The above example creates the
following files:

    oliver@canopus:~/repos/audiosplit/build$ ls ../music/Brahms\ -\
    Variations.*.aac
    '../music/Brahms - Variations.BL.aac'  '../music/Brahms - Variations.FR.aac'
    '../music/Brahms - Variations.BR.aac'  '../music/Brahms - Variations.LFE.aac'
    '../music/Brahms - Variations.FC.aac'  '../music/Brahms - Variations.FL.aac'

