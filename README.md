# Radical Open Software Engine
The Radical Open Software Engine (ROSE) is a game and software engine built in C++. The idea is for moddability and development to be as easy as including the headers from the game files and linking against the objects also in the game files. 


## Building ROSE
Do not use Visual Studio's CMake integration. For the love of God do not use Visual Studio's CMake integration. Use the configure scripts, or better yet use JetBrains. As of me writing this, Visual Studio's built-in CMake is at least a version behind the bare minimum functionality I need. Also it won't let me use a goddamn custom CMake output directory so I am doing my got dang best here 
### Dependencies
- SDL3
- ImGui
- SPIRV-Cross
- Glslang
- 

## Why did I decide to make this?
Skyrim modding is one difficult son of a bitshift. Everytime the engine updates, mods break and the script extender has to update before they can work again. So I decided to make an engine with a consistent, object-oriented ABI (scary, I know) to avoid such Bethesda-esque terribleness. (Todd, you know I love you, but this ain't it man)

Also I thought it would be really cool.

## Code Style

### Naming
| Thing | Convention | Example |
|---|---|---|
| Classes, structs, type aliases | `PascalCase` | `Application`, `AppID` |
| Methods and free functions | `PascalCase` | `GetInstance()`, `FrameUpdate()` |
| Member variables | `m_camelCase` | `m_title`, `m_shouldClose` |
| Local variables | `camelCase` | `curScene`, `inputSystem` |
| Constructor / function parameters | `_camelCase` | `_title`, `_flags` |
| Constants and flag values | `UPPER_SNAKE_CASE` | `APPLICATION_HEADLESS`, `KEYSTATE_SIZE` |
| Regular enum values | `PascalCase` | `GamepadStick::Left` |
| Opaque-handle enum values (KeyCode, etc.) | `UPPER_SNAKE_CASE` | `KeyCode::LEFT_SHIFT` |

### Namespaces
All engine code lives inside `namespace ROSE { }`. No `using namespace` in headers.

### File headers
Every file gets a Doxygen block:
```cpp
/**

  @file      MyFile.h
  @brief     One-line summary
  @details   ~
  @author    Viola Case
  @date      DD.MM.YYYY
  @copyright © Viola Case, 2026. All rights reserved.

**/
```

### Headers
- `#pragma once` only — no include guards.
- Keep includes minimal; prefer forward declarations.
- At API boundaries, use RTL types (`String`, `List<T>`, `UniquePtr<T>`, etc.) instead of `std::` equivalents.

### Class layout
```
private (friended classes first)
public
private (data members)
```
- Mark non-throwing functions `noexcept`.
- Mark [[nodiscard]] on any function whose return value should not be silently discarded.
- Use `explicit` on single-argument constructors.
- Use `= delete` to suppress unwanted copy/move; `= default` for trivial destructors.
- Prefer delegating constructors over duplicated initializer lists.
- Initialize member variables in-class with brace syntax: `String m_title{"game"}`.

### General
- 2-space indentation.
- Opening brace on the same line.
- `constexpr` over `#define` for constants.
- Use `#pragma region` / `#pragma endregion` to group large repetitive blocks (e.g., key-code tables).

## Contributing to ROSE
Feel free to branch and PR. I'll look over your changes and if they follow the style and don't have any issues I'll merge it in. 

## AI agents
While agentic developers are extremely useful, don't vibe code. Be the primary and let the agent act as an advisor. If you don't know what something does, ***do not touch it***.
