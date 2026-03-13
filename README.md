# Radical Open Software Engine
The Radical Open Software Engine (ROSE) is a game and software engine built in C++. The idea is for moddability and development to be as easy as including the headers from the game files and linking against the objects also in the game files. 


## Building ROSE
Use the configure scripts. For the love of God use the configure scripts. As of me writing this, Visual Studio's built-in CMake is a version behind the bare minimum functionality I need.
### Dependencies
- SDL
- ImGui
- {fmt}
- Probably some other stuff
- Honestly just use the VCPKG integration, it's easiest.

## Why did I decide to make this?
Okay so you know how you really wanted to impress someone in elementary school so when it was time to stack chairs you carried two or three? That's kinda the mindset I have going into this.

## Contributing to ROSE
Feel free to branch and PR. I'll look over your changes and if they follow the style and don't have any issues I'll merge it in. 
