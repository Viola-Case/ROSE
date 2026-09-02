# Mesh importing: options and open questions

Notes toward a mesh importer. Nothing here is implemented yet; this is the shape of the decision,
written down so it can be argued with before any of it is code.

Files this touches, or would:

| File | Role today |
|---|---|
| `include/ROSE/Core/mesh.h` | `Vert`, `Mesh`, `MeshInstance`, `MeshRegistry` |
| `src/Core/meshregistry.cpp` | the registry; `RegisterMesh` is an empty function body |
| `src/Core/renderable.cpp` | `MeshRenderable::Rebuild`/`Collect`, the only consumer of a `Mesh` |
| `src/Core/texture.cpp` | `LoadTexture`, the one working loader and the precedent to copy |
| `include/ROSE/Core/asset.h`, `include/ROSE/Editor/assetfile.h` | the asset format, sketched but hollow |
| `src/Tools/AssetMaker/main.cpp` | the offline packer, which does not yet write payload bytes |
| `dependencies.toml` | where a third-party parser would have to be pinned |

## 1. What is actually there

`Mesh` is a `List<Vert>` plus a `List<uint32_t>`, and `Vert` is `Vec3d position`, `Vec3d normal`,
`Vec2d texCoord`. That is 64 bytes per vertex.

`MeshRegistry::RegisterMesh` (`src/Core/meshregistry.cpp:20`) has an empty body. Nothing has ever put
a mesh into the registry, which means `MeshRenderable::Collect` (`src/Core/renderable.cpp:101`) has
always taken its `if (!m_mesh) return;` early-out. The mesh path is wired end to end and has never
carried a single triangle.

`MeshRenderable::Unpack` reads a `"mesh"` UUID out of the scene JSON and deliberately does not
resolve it there, because `Unpack` runs at parse time before anything could have registered a mesh.
Resolution is retried once per frame in `Collect`.

The asset pipeline is declared but not built: `AssetType::Mesh` exists in `asset.h`,
`AssetFileHeader` exists in `Editor/assetfile.h`, `ROSE_AssetMaker` has its payload writes commented
out, and `Surface::LoadAsset` is a stub that returns an invalid surface. None of it can round-trip a
byte yet.

The only loader that works is `LoadTexture(path, name)` in `src/Core/texture.cpp:73`: decode from
disk, generate a UUID, hand ownership to the registry, return the id. Whatever the mesh importer
looks like, it should look like that, because that is the shape the engine already understands.

## 2. Decision A: where the importer lives

**(a) Runtime loader in Core.** `LoadMesh(path, name) -> UUID`, symmetric with `LoadTexture`. Usable
almost immediately; no new file format to design first.

**(b) Offline only.** AssetMaker converts OBJ/glTF into a `.roseasset` with `AssetType::Mesh`, and the
runtime only ever reads ROSE's own format. This is clearly what `asset.h`, `assetfile.h` and
AssetMaker are aiming at. The cost is that none of that pipeline works today, so choosing this means
building the asset format first and the importer second, with nothing on screen until both land.

**(c) Parser as a shared layer.** A pure `ParseOBJ(bytes) -> Mesh` with no file I/O and no registry
contact, called by Core's `LoadMesh` and by AssetMaker alike. AssetMaker's `main.cpp` already carries
`// TODO rewrite this as a library`, so this is the direction that file is already pointing.

**Recommendation: (c), implemented as (a) first.** Write the parser as a free function over bytes
returning a `Mesh`; wrap it in a `LoadMesh` that mirrors `LoadTexture`. The offline route stays open
without being paid for now, and the parser is testable without depending on a file format that does
not exist. Keep the parser free of `MeshRegistry` entirely, so the thing that reads geometry and the
thing that owns geometry stay separable.

## 3. Decision B: which format

**OBJ.** Roughly 300 lines, no dependency, and it maps exactly onto `Vert`: positions, normals, texture
coordinates and faces, and nothing else that `Mesh` could not hold anyway. No hierarchy, no skinning,
no PBR materials.

**glTF 2.0 / .glb.** The actual interchange standard. `nlohmann_json` is already vendored, so the JSON
half costs nothing; the work is accessor and buffer-view decoding, plus the binary chunk layout for
`.glb`. Either hand-rolled for the subset that matters, or `cgltf` (single header, MIT), which would
be a `path =` entry in `dependencies.toml` alongside glad rather than a fetched one.

**FBX / assimp.** assimp is a large dependency whose data model would have to be translated into ROSE's
anyway, and the vendoring ceremony in `dependencies.toml` is real work per dependency.

**Recommendation: OBJ first, glTF second, assimp never.** Nothing renders a mesh at all right now, so
the first job is proving the registry, the de-duplication and the renderable path against a format
that can be hand-edited and eyeballed in a text editor. glTF earns its place once there is a material
and node story to hang it on; today there would be nowhere to put most of what it carries.

## 4. Decision C: data-model questions the importer forces

These change the importer's signature, so they want settling before code.

### 4.1 `Vert` is doubles, and half of it is dead

64 bytes per vertex, and `MeshRenderable::Rebuild` (`src/Core/renderable.cpp:75`) narrows every
position to float once per mesh and **drops `normal` on the floor**. `Vert::normal` is declared at
`mesh.h:22` and read nowhere in the codebase.

Nothing loads meshes today, so this is the cheapest moment there will ever be to make `Vert` float:
the blast radius is `mesh.h` plus one loop in `renderable.cpp`. Once meshes are being imported and
scenes reference them, the same change means touching cached data.

Open question: narrow `Vert` to float as part of this work, or leave it and let the importer widen
floats into doubles that are immediately narrowed back?

### 4.2 Index de-duplication is the actual algorithm

OBJ indexes positions, texture coordinates and normals separately, and a face names a triple per
corner. `Mesh` has one index buffer over one vertex array. So the importer has to hash the
(v, vt, vn) triple and emit a unique `Vert` per distinct triple. This is the substance of the work;
the parsing itself is trivial by comparison. The same problem shows up in glTF whenever a file does
not share accessors.

### 4.3 One file usually holds many objects

OBJ `o` / `g` / `usemtl` groups map to N meshes, not one. `Mesh` is a single vertex and index buffer,
and there is no `Material` type at all: `AssetType::Material` is an enum value with nothing behind it.

Two options: merge everything into one `Mesh` and ignore group boundaries, or split by group and
return a `List<UUID>`.

**Recommendation: merge, return a single UUID.** Splitting without materials to split *for* only
makes every caller do bookkeeping that buys nothing. Revisit when materials exist.

### 4.4 UUID stability

`LoadTexture` generates a fresh UUID on every run, which means a scene JSON can never name the result
of one. `MeshRenderable::Unpack` reads only a UUID, so an imported mesh would be unreferenceable from
a scene file for exactly the same reason.

Two ways out: derive the id deterministically (a hash of the name or the path), or add a `"meshName"`
key to `Unpack` and resolve through `MeshRegistry::GetMesh(const String &)`, which already exists and
already has a name map behind it. The second is a smaller change and does not commit to a hashing
scheme, but it leaves two ways to name a mesh in scene files.

### 4.5 `RegisterMesh` has to be written regardless

Whatever else is decided, the empty body has to be filled, and it should mirror `RegisterTexture`
(`src/Core/texture.cpp:22`): reject `UUID::Invalid()`, reject a duplicate id and destroy the loser,
take ownership into the `UniquePtr` map, populate the name map when a name was given. The texture
version also normalises the pixel format at this boundary; the mesh equivalent, if any, would be
whatever invariant the backends want to stop re-checking.

## 5. What the renderer will not do for you

Worth being clear-eyed about before attributing a disappointing result to the importer:

- `GL_DEPTH_TEST` is never enabled (`src/Core/openglrenderer.cpp:212`); the depth buffer is cleared and
  otherwise unused, and draw order comes entirely from layer plus enrollment order.
- The software rasterizer does no backface culling (`src/Core/softwarerenderer.cpp:209`).
- There is no lighting, and `DrawVertex` (`include/ROSE/Core/gfx.h:135`) carries position, colour and
  texture coordinate only. No normal crosses the behavior/backend boundary.
- `MeshRenderable` has a tint and no texture. `SpriteRenderable` is the one that samples.

So a correctly imported model draws as an unlit, untextured, painter's-algorithm triangle soup in a
single flat colour. The importer is worth building on its own terms, but depth testing, a normal in
`DrawVertex` and a texture on `MeshRenderable` are separate pieces of work, and they are what would
actually make a model look right.

## 6. Suggested first slice

1. Fill in `MeshRegistry::RegisterMesh`, mirroring `RegisterTexture`.
2. Decide 4.1 (float vs double `Vert`) and, if narrowing, do it now while the blast radius is two files.
3. `ParseOBJ(const char *bytes, size_t len) -> Mesh`, no I/O, no registry, with the (v, vt, vn)
   de-duplication as its core.
4. `LoadMesh(path, name) -> UUID` in Core, shaped exactly like `LoadTexture`.
5. Decide 4.4 so a scene file can actually name the result.
6. A cube or a teapot in one of the examples, to prove the whole path.

Deferred deliberately: the `.roseasset` mesh payload, materials, glTF, mesh splitting by group,
anything that needs a normal to survive to the backend.
