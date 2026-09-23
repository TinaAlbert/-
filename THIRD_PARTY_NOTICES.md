# Third-party notices

## SDL2 2.32.6

The application uses [SDL2](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.6)
for window creation, input events and presenting the CPU pixel buffer. SDL2 is
downloaded unchanged by the build script and is not authored by this project.

```text
Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
```

## Toolchain runtime

Local builds may copy `libwinpthread-1.dll` from the installed MinGW-w64 toolchain.
If distributing binaries, include the license notices supplied by that toolchain.
This repository distributes source and preview images, not a packaged runtime.

## Coursework material

The original course report is retained in `docs/course-report.docx`. No new
license is assigned to the coursework or report by this maintenance update.
