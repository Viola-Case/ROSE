# Roadmap
## Engine
- [x] Scene data dynamically constructs objects/behaviors
- [ ] Logging system
  - [x] Barebones console stdio
  - [ ] Timestamps
  - [ ] Custom sinks
  - [ ] Formatter initializer
- [ ] OpenGL support
- [ ] Vulkan support
- [ ] Mesh loading
  - [ ] Or really any sort of functional 3D graphics at all
- [ ] Working audio API
  - [ ] Native HRTF

## Tools
- [ ] Editor
  - [ ] Scene preview viewport
  - [ ] Camera view
  - [ ] Object maker
  - [ ] Bundled compiler
- [ ] UUID generator 
- [x] Asset packer (`ROSE-rpkg`) — pack, list, extract
  - [ ] Re-read loose assets when they change on disk, instead of caching until unmount
  - [ ] Per-entry codec and level (see the deferred protocol in docs/internal/assets.md)
  - [ ] Stable asset identity a scene file can name

## Plugins
- [ ] Physics
- [ ] Networking
- [ ] OpenXR
- [ ] 

## Examples
- [ ] Game 1: Pong
  - [x] Paddles move
  - [x] Ball
  - [ ] Scoring system
- [ ] Game 2: Visual novel
- [ ] Game 3: [Santa vs the Children](https://viola-case.itch.io/santa-vs-the-children) (or similar)
- [ ] Game 4: 3D environment
- 
