# Vunzip

`vunzip` is a command-line utility to unzip Valve's custom .vz compressed files.

# Building

On **OS X**, use the supplied Xcode project.

This code has not been tested on **Windows**, **Linux** or anything else, but I don't see why it shouldn't work there.

# Credits

* [johndrinkwater](https://github.com/johndrinkwater) for his [reference implementation](https://gist.github.com/johndrinkwater/8944787) in Python.
* Ymgve for the file format details (see the reference implementation).
* [SteamDB](https://github.com/steamdatabase) IRC for collectively figuring out the format so quickly.

# File format

## Header
* 2 bytes: 'VZ'
* 1 bytes: Format version ('a')
* 4 bytes: POSIX timestamp, assumedly of when the file was created

## Body
* LZMA-compressed content

## Footer
* 4 bytes: CRC32 of the decompressed content
* 4 bytes: size of the decompressed content
* 2 btyes: 'zv'

