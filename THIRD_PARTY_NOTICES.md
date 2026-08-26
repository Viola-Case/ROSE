# Third-party notices

ROSE is distributed under the terms in [LICENSE](LICENSE). It also links, and
therefore ships, code written by other people under their own terms. This file
is the summary; the binding text is the licenses themselves.

**Where the actual license texts are.** `./vendor.ps1` (or `./vendor.sh`)
collects them out of every checkout it builds and installs them into the vendor
prefix:

```
.vendor/install/share/licenses/<component>/PACKAGE.txt   repository, tag, commit
.vendor/install/share/licenses/<component>/LICENSE...    the component's own text
.vendor/install/share/licenses/<component>/<bundled>/    each library it vendors
```

`rose_deploy_licenses()` in [CMakeLists.txt](CMakeLists.txt) copies that tree to
`licenses/` next to every executable ROSE builds, alongside the DLLs it covers,
so a copied-out build directory carries its attribution with it. Nothing has to
be assembled by hand at release time.

The tables below are maintained by hand against
[dependencies.toml](dependencies.toml) and are accurate for the versions pinned
there. Bumping a pin can change a license; re-read the collected text when you
do.

## Directly linked

| Component | Version | License | Used for |
|---|---|---|---|
| [SDL3](https://github.com/libsdl-org/SDL) | release-3.4.14 | Zlib | Window, input, audio, GPU |
| [SDL3_image](https://github.com/libsdl-org/SDL_image) | release-3.4.4 | Zlib | JPEG / PNG / TIFF decoding |
| [SDL3_mixer](https://github.com/libsdl-org/SDL_mixer) | release-3.2.4 | Zlib | Audio decoding and mixing |
| [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf) | release-3.2.2 | Zlib | Font rendering |
| [Dear ImGui](https://github.com/ocornut/imgui) | v1.92.9b | MIT | Editor UI |
| [glad](https://github.com/Dav1dde/glad) | 0.1.36 | MIT (see below) | OpenGL 4.5 loader |
| [mimalloc](https://github.com/microsoft/mimalloc) | v3.5.0 | MIT | Allocator |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.12.0 | MIT | Scene and config serialization |
| [CLI11](https://github.com/CLIUtils/CLI11) | v2.7.2 | BSD-3-Clause | Asset and UUID tool front ends |

### glad, specifically

`third_party/glad` is generated code committed straight into this repository,
with no license header of its own, which is the easiest kind of attribution to
lose. [third_party/glad/LICENSE](third_party/glad/LICENSE) is glad's own file
copied verbatim from the 0.1.36 tag and covers three separate things:

- `src/glad.c` and `include/glad/glad.h` — output of the glad generator, MIT,
  Copyright (c) 2013-2021 David Herberth.
- The enum values and command signatures inside that output, which come from the
  Khronos OpenGL registry (`gl.xml`) — Apache-2.0, Copyright (c) 2013-2020
  The Khronos Group Inc.
- `include/KHR/khrplatform.h` — Khronos' MIT-style license. Its text is in the
  header itself and must stay there.

The header of [third_party/glad/CMakeLists.txt](third_party/glad/CMakeLists.txt)
records the exact generator invocation, so the files can be reproduced rather
than trusted.

## Bundled inside the SDL libraries

Most of these are built from submodules under each satellite's `external/` and
statically linked into `SDL3_image.dll`, `SDL3_mixer.dll` and `SDL3_ttf.dll` —
`SDL*_VENDORED=ON` with `SDL*_DEPS_SHARED=OFF`. A few are third-party code SDL
keeps directly in its own `src/`. Either way they ship inside those binaries, so
their notices ship too.

| Component | Inside | License |
|---|---|---|
| hidapi | SDL3 (`src/hidapi`) | BSD-3-Clause, at our option |
| yuv2rgb | SDL3 (`src/video/yuv2rgb`) | BSD-3-Clause |
| libjpeg | SDL3_image | IJG (Independent JPEG Group) |
| libpng | SDL3_image | PNG Reference Library License v2 |
| libtiff | SDL3_image | libtiff (BSD-style, Sam Leffler / SGI) |
| zlib | SDL3_image | Zlib |
| libFLAC | SDL3_mixer | BSD-3-Clause (Xiph) |
| libogg | SDL3_mixer | BSD-3-Clause (Xiph) |
| libvorbis | SDL3_mixer | BSD-3-Clause (Xiph) |
| Opus | SDL3_mixer | BSD-3-Clause |
| opusfile | SDL3_mixer | BSD-3-Clause |
| WavPack | SDL3_mixer | BSD-3-Clause |
| dr_libs | SDL3_mixer (`src/dr_libs`) | Unlicense or MIT-0 |
| FreeType | SDL3_ttf | FTL or GPL-2.0, at our option; used under the FTL |
| HarfBuzz | SDL3_ttf | "Old MIT" |
| dlg | SDL3_ttf (via FreeType) | Boost Software License 1.0 |
| PlutoSVG | SDL3_ttf | MIT |
| PlutoVG | SDL3_ttf | MIT |

hidapi is offered under a choice of BSD-3-Clause, GPL-3.0 or its original
license; SDL takes the BSD option, and all three texts are collected so the
choice can be checked rather than taken on trust.

Codecs the manifest deliberately leaves out — AVIF, JXL, WebP, libxmp,
game-music-emu, Tremor, mpg123, and the TiMidity MIDI backend — are never
compiled, and so are not listed here and have no notice in `licenses/`. The
collector only copies the notices of submodules that were actually initialised,
and the vendor engine deinitialises any submodule the manifest stops asking for,
so a notice cannot outlive the code it covers.

## Obligations worth knowing about

**Everything here requires the notice to be distributed with the binaries.**
MIT, BSD-3-Clause, Zlib, Apache-2.0 and the libpng/libtiff/IJG licenses all say
some version of "retain this notice in redistributions". That is what the
deployed `licenses/` directory is for; shipping a build without it is the one
way to get this wrong by accident.

**FreeType asks for credit in documentation.** The FTL requires that use of
FreeType be acknowledged in the documentation of any product using it, with the
suggested wording *"Portions of this software are copyright © \<year\> The
FreeType Project (www.freetype.org). All rights reserved."* Include that in the
manual or credits of anything shipped, not only in the licenses directory.

**Nothing here is copyleft, and that is deliberate.** Every license above is
permissive: satisfy the notice requirement and there is nothing further to do —
no source offer, no relinking obligation, no reciprocal licensing of ROSE or of
anything built with it.

Keeping it that way took one decision worth recording. SDL_mixer can decode MP3
through either mpg123 or dr_mp3, and it used to be built here with both.
mpg123 is LGPL-2.1, and `SDLMIXER_DEPS_SHARED=OFF` linked it statically into
`SDL3_mixer.dll` — which makes that DLL a combined work, so LGPL §6 would have
required that recipients be able to relink it against their own mpg123. That
obligation would have been inherited by everyone shipping a game built on ROSE.
So mpg123 is off (`SDLMIXER_MP3_MPG123=OFF`), and MP3 decodes through dr_mp3
(Unlicense / MIT-0), which SDL_mixer already compiled in as the fallback.

The cost is that dr_mp3 is less forgiving of malformed streams and weaker on
Xing/LAME gapless metadata than mpg123. That is a fair trade for authored game
audio, particularly when Ogg Vorbis, Opus and FLAC are all available and all
better suited than MP3. If ROSE ever needs to play MP3s it did not produce —
user-supplied soundtracks, mod support — the fix is to re-enable mpg123 with
`SDLMIXER_MP3_MPG123_SHARED=ON`, which keeps it a replaceable DLL and satisfies
§6 by construction, rather than to link it statically again.

This is a description of what the licenses say, not legal advice. Before
shipping commercially, have someone qualified confirm it.

## Keeping this file honest

The vendor engine warns if any dependency produced no license file at all, and
fails outright on a `license_paths` entry that does not exist, so the manifest
cannot quietly drift away from the tree. Adding a dependency without attribution
is meant to be noisy. Adding one without updating the tables here is not caught
automatically — so update them in the same commit as the manifest change.
