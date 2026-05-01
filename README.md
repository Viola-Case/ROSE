# Radical Open Software Engine
The Radical Open Software Engine (ROSE) is a game and software engine built in C++. The idea is for moddability and development to be as easy as including the headers from the game files and linking against the objects also in the game files.

![ROSE logo](assets/img/rose_256x.png)

## Building ROSE
**Compiler: LLVM is required.** Compile with either `clang++` or `clang-cl`. MSVC (`cl.exe`) is not supported. Install the [LLVM toolchain](https://releases.llvm.org/) and ensure it is on your PATH.

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
| Thing                                     | Convention               | Example(s)                              |
|-------------------------------------------|--------------------------|-----------------------------------------|
| Classes, structs, type aliases            | `PascalCase`             | `Application`, `AppID`                  |
| Methods and free functions                | `PascalCase`             | `GetInstance()`, `FrameUpdate()`        |
| RTL methods                               | `snake_case` (match STL) | `List::push_back()`                     |
| RTL functions                             | `PascalCase`             | `MakeUnique<T>()`                       |
| Private class member variables            | `m_camelCase`            | `m_title`, `m_shouldClose`              |
| Public member variables                   | Case-specific            | `Pair::first` `FixedArray::_data`       |
| RTL public member variables               | Case-specific            |                                         |
| Local variables                           | `camelCase`              | `curScene`, `inputSystem`               |
| Constructor / function parameters         | `_camelCase`             | `_title`, `_flags`                      |
| Constants and flag values                 | `UPPER_SNAKE_CASE`       | `APPLICATION_HEADLESS`, `KEYSTATE_SIZE` |
| Regular enum values                       | `PascalCase`             | `GamepadStick::Left`                    |
| Opaque-handle enum values (KeyCode, etc.) | `UPPER_SNAKE_CASE`       | `KeyCode::LEFT_SHIFT`                   |

### Namespaces
All engine code lives inside `namespace ROSE { }`. No `using namespace` in headers.
Math exists inside `namespace ROSE::math`.
When building a module (e.g., physics), create a child namespace (e.g., `namespace ROSE::phys`).

### File headers
Every file gets a Doxygen block:
```cpp
/**

  @file      [file name]
  @brief     One-line summary
  @details   ~
  @author    Viola Case
  @date      DD.MM.YYYY
  @copyright © Viola Case, 2026. All rights reserved.

**/
```

### Headers
| Rule                            | Notes                                                              |
|---------------------------------|--------------------------------------------------------------------|
| `#pragma once` only             | No include guards — can't figure out a consistent scheme for them  |
| Minimize includes               | Prefer forward declarations                                        |
| Use RTL types at API boundaries | `String`, `List<T>`, `UniquePtr<T>` instead of `std::` equivalents |

### Class layout
```cpp
private (friended classes first)
public
private (data members)
```

| Rule                                             | Example                        |
|--------------------------------------------------|--------------------------------|
| Mark non-throwing functions                      | `noexcept`                     |
| Flag non-discardable return values               | `[[nodiscard]]`                |
| Single-argument constructors                     | `explicit MyClass(int _x)`     |
| Suppress unwanted copy/move; trivial destructors | `= delete` / `= default`       |
| Avoid duplicated initializer lists               | Prefer delegating constructors |
| Initialize members in-class with brace syntax    | `String m_title{"game"}`       |

### General
| Thing        | Convention                             | Example(s)                                           |
|--------------|----------------------------------------|------------------------------------------------------|
| Indentation  | 2 spaces                               |                                                      |
| Braces       | K&R (opening brace on same line)       | `int main() {` <br/>`...`<br/> `}`                   |
| Preprocessor | `costexpr` over `#define`              | `constexpr float PI =  3.141592653589793f;`          |
| Regions      | `#pragma region` / `#pragma endregion` | `#pragma region` <br/>`...`<br/> `#pragma endregion` |

## Contributing to ROSE
Feel free to branch and PR. I'll look over your changes and if they follow the style and don't have any issues I'll merge it in.

## AI agents
While agentic developers are extremely useful, don't vibe code. Be the primary and let the agent act as an advisor. If you don't know what something does, ***do not touch it***.
Do not commit a `CLAUDE.md` or `CLAUDE.local.md`

