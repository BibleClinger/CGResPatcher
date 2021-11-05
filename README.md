# CGResPatcher
This project provides a way to patch the available resolutions in Comanche Gold.

# History

Comanche Gold is a fantastic helicopter simulation from the late 1990's by Novalogic. The sim can still be played on Windows 10 even in multiplayer. Unfortunately this requires modifications.

- The Modern OS Fix to run on Windows 10 (exe patch + copying of in-game video files) along with one of the menu files.
- Multiplayer via TCP/IP is possible with the CGoldIP utility (with or without tunneling programs like Hamachi).
- The DLLs from dgVoodoo can be used to allow more resolutions to work correctly. This allows selecting of the 1280x1024 resolution.

# Current Problem

Comanche Gold's support for 1280x1024 was probably not tested very well. It's locked behind the menu by default. It's a 5:4 resolution, and not a 4:3 resolution. As such, when fullscreen 1280x1024 is selected, the top portion of the screen is missing. The cutoff at the top can be seen in this video: https://www.youtube.com/watch?v=MNK6rcxdkZk

# Solution

This utility, CGResPatcher can patch an existing WC3.exe (the Comanche Gold exe) with a new series of resolutions for the modern age. The ini file can be manually edited, and the exe run with the correct parameters to patch the exe.

# Known Issues

- The way Comanche Gold scales widescreen resolutions in fullscreen seems particularly weird. Letterboxing does not begin to describe it. To fly in widescreen, edit one of the window resolutions.
- Not all resolutions are supported. Comanche Gold seems to switch rendering modes in fullscreen, and weird things can happen in fullscreen. You may have to experiment between your graphic interception utility (ie. dgVoodoo), and this utility.
