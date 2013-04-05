Yahoo! Messenger Archive Decoder
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
A free utility that decodes messages from Yahoo! Messenger archives (.dat files).
I created this program to merge Yahoo! Messenger logs with my linux Pidgin logs, in one common place.


Installation
    No installation needed.

Usage
    Method 1
      Just drag & drop a profile folder on ym_decoder.exe
      For example, C:\Program Files\Yahoo!\Messenger\Profiles\my_id
    Method 2
      From the command line, run:
        ym_decoder <profile_dir>

    A "Decoded_Archive" subfolder will be created in the target folder,
    which contains all the decoded messages, sorted by friend's id.

This software decodes ALL the messages in that profile directory. If you need to view just a .dat file,
have a look at the source code, where i separated the file decoding part. You can even port it to linux if you want,
as that part is not system-dependent.



I've included the Dev-C++ project file (.dev)

You can use the MinGw compiler from Dev-C++ package to compile it.
http://bloodshed.net/devcpp.html
