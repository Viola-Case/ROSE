# Radical Open Software Engine
The Radical Open Software Engine (ROSE) is a game and software engine built in C++. The idea is for moddability and development to be as easy as including the headers from the game files and linking against the objects also in the game files.

![ROSE logo](assets/img/logo/rose.svg)

## Building ROSE
**Compiler: LLVM is required.** Compile with either `clang++` or `clang-cl`. MSVC (`cl.exe`) is not supported currently. Install the [LLVM toolchain](https://releases.llvm.org/) and ensure it is on your PATH.

Do not use Visual Studio's CMake integration. For the love of God do not use Visual Studio's CMake integration. Use the configure scripts, or better yet use JetBrains. As of me writing this, Visual Studio's built-in CMake is at least a version behind the bare minimum functionality I need. Also it won't let me use a goddamn custom CMake output directory so I am doing my got dang best here

### Quick start

```
./vendor.ps1                     # or ./vendor.sh -- fetches and builds dependencies
cmake --preset release
cmake --build --preset release
```

The first `vendor` run takes a while because it builds everything from source.
Every run after that is a no-op unless you change something, so re-running it is
free — that is the entire point of it.

### Dependencies

Pinned to explicit upstream git tags in [`dependencies.toml`](dependencies.toml),
checked out and built into `.vendor/` by `vendor.ps1` / `vendor.sh`:

| | |
|---|---|
| SDL3 | window, input, audio, GPU |
| SDL3_image | jpeg, png, tiff |
| SDL3_mixer | FLAC, Vorbis, MP3, Opus, WavPack |
| SDL3_ttf | FreeType, HarfBuzz, SVG via plutosvg |
| Dear ImGui | the sdl3, sdlrenderer3 and opengl3 backends |
| glad | OpenGL 4.5 loader, pre-generated in `third_party/glad` |
| mimalloc | allocator, `MI_OVERRIDE=ON` |
| nlohmann-json | JSON |
| CLI11 | command line parsing for the tools |

**To bump a dependency**, change its `tag` in `dependencies.toml` and re-run
`./vendor.ps1`. Only that dependency refetches and rebuilds.

Useful flags: `--list` shows what is pinned and what is stale without touching
the network, `--only <name>` acts on one dependency, `--force` rebuilds
regardless, `--purge` deletes `.vendor` entirely. `--help` lists them all.

### vcpkg

vcpkg still works and is kept as a fallback, driven by `vcpkg.json`:

```
cmake --preset release-vcpkg
cmake --build --preset release-vcpkg
```

It needs `VCPKG_ROOT` set, and it builds into `build/*-vcpkg` so the two trees
never collide. It is slower — a cache miss rebuilds the world — which is why it
is no longer the default.

## Why did I decide to make this?
Skyrim modding is one difficult son of a bitshift. Everytime the engine updates, mods break and the script extender has to update before they can work again. So I decided to make an engine with a consistent, object-oriented ABI (scary, I know) to avoid such Bethesda-esque terribleness. (Todd, you know I love you, but this ain't it man)

Also I thought it would be really cool.

## Code Style
A line should not be more than 120 characters if you can help it. This includes comments.

## Contributing to ROSE
Feel free to branch and PR. I'll look over your changes and if they follow the style and spirit and don't have any issues I'll probably merge it in.

## AI agents
While agentic developers are extremely useful, don't vibe code. Be the primary and let the agent act as an advisor. If you don't know what something does, ***do not touch it***.
Do not commit a `CLAUDE.md`. 

