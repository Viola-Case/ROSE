# Assets and `.rpkg` archives

How assets get from the disk into the engine. Written against `master` @ `de3eafa` plus the
`asset-hotloading` work (**2026-09-11**); every claim here was confirmed by building and running
`ArchiveTest` and the `ROSE-rpkg` round trip described at the bottom.

Files this covers:

| File | Role |
|---|---|
| `include/ROSE/Core/asset.h` | `AssetType`, `AssetID`, `AssetView`, `AssetEntry`, `AssetCatalog` |
| `include/ROSE/Core/archive.h` | `RpkgHeader`, `ArchiveCodec`, `ArchiveResult`, `Archive` |
| `src/Core/archive.cpp` | reading and validating an `.rpkg` |
| `src/Core/asset.cpp` | the catalog: mounting, the index, the loose-file override |
| `src/Tools/Packer/main.cpp` | `ROSE-rpkg` — `pack`, `list`, `extract` |
| `examples/ArchiveTest/main.cpp` | the end-to-end check |

**What this replaced.** `.roseasset`, `AssetFileHeader`, `AssetFile` and `ROSE_AssetMaker`. None of
it ever moved a byte: every payload write in AssetMaker's `main.cpp` was commented out, `AssetFile`
was two owning `char *` with no destructor, and nothing in `src/Core/` ever included
`Editor/assetfile.h`. `Surface::LoadAsset` was a stub whose body was commented out.

All of it is deleted rather than commented out, `src/Tools/AssetMaker/` included. `ROSE_Editor` is
kept behind a `#[[ ]]` block because a redesign is pending; nothing is pending here, and the sources
could not have compiled anyway once `assetfile.h` went. The one part worth keeping, the
extension-sniffing table, is now `AssetTypeFromExtension` in `asset.h`.

---

## 1. The format

Little-endian throughout. Two regions: a 64-byte plaintext header and one compressed body.

```
  ---- header: 64 bytes, never compressed ----
  [4]  magic            "ROSE"
  [4]  file_kind        "ARCH"   (FileType::Archive)
  [2]  version major    ROSE_VERSION_MAJOR
  [2]  version minor    ROSE_VERSION_MINOR
  [4]  format_revision  RPKG_FORMAT_REVISION
  [4]  codec            ArchiveCodec: None | Zstd
  [4]  flags            ArchiveFlags
  [4]  entry_count
  [4]  string_bytes     size of the string table, padded to a multiple of 16
  [8]  body_size_raw    bytes after decompression
  [8]  body_size_stored bytes on disk
  [8]  body_hash        FNV1A64 over the *raw* body
  [8]  reserved         zero
  ---- body: a single zstd frame (or raw bytes when codec == None) ----
  [entry_count * 48]  AssetEntry[]   ascending by pathHash, strictly
  [string_bytes]      string table   NUL-terminated virtual paths, NUL-padded
  [remainder]         payload blob   each entry 16-byte aligned
```

`RPKG_FORMAT_REVISION` is bumped independently of the engine version. The format outlives individual
releases, and an engine bump that does not touch the layout must not invalidate everyone's archives.
A reader rejects any revision it does not recognise outright.

### Why the header is outside the frame

So `ROSE-rpkg list` and a corruption diagnostic work without decoding anything. A truncated or
corrupt archive still prints what it claimed to be, which is most of the diagnosis.

### Why the directory is inside the frame

Nothing ever consults it independently. `Archive::Open` decompresses the whole body once into one
`RawBuffer` and points three views into it; from then on the entire archive is resident. That is the
design, not an accident of it — see §4.

### Why one frame rather than one per entry

Cross-file redundancy is exactly what zstd's window exploits, and a tree of shaders, scene JSON and
OBJ text shares a great deal. One frame also means one `ZSTD_decompress` call into one allocation.
Per-entry frames would buy random access, which nothing wants yet.

### Alignment

The directory is `entryCount * 48` bytes, and 48 is a multiple of 16, so it never misaligns what
follows. The packer pads the string table up to a multiple of 16 for the same reason, which is why
`stringBytes` can exceed the sum of the path lengths. Payload entries are then individually padded
to 16, so `payloadOffset` is always 16-aligned and a caller may reinterpret payload bytes in place up
to that alignment. `RawBuffer` allocates through `::operator new`, whose guarantee on x64 is 16.

### `AssetEntry`

48 bytes, standard layout, written verbatim. Reordering it means bumping the revision.

```cpp
struct AssetEntry {
  AssetID  pathHash;      // lookup key, and the sort key of the directory
  uint64_t payloadOffset; // from the start of the payload blob; 16-byte aligned
  uint64_t payloadSize;
  uint32_t pathOffset;    // into the string table
  uint32_t pathLength;    // excluding the NUL
  AssetType  type;
  AssetFlags flags;
  uint64_t contentHash;   // FNV1A64 of the payload
};
```

`contentHash` is not checked on load — `bodyHash` already covers every byte. It is there so hot
reload can tell a touched file from a changed one without comparing contents.

### Validation

The bytes came off disk and nothing signed them, so `DirectoryIsSane` in `archive.cpp` checks every
offset against the region it indexes before a single pointer is built from it: payload ranges inside
the payload blob, string ranges inside the string table, a NUL where each path ends, a NUL at the end
of the table, no zero hashes, and strictly ascending hashes (which proves sortedness and
duplicate-freedom at once). A hand-edited archive fails to mount rather than reading out of bounds.

`bodyHash` is load-bearing and not redundant with zstd. Frame checksums are off by default; flipping
one byte of a compressed body decompresses cleanly into *different* bytes. The FNV1A64 over the raw
body is what catches that, and it is the check that fires in practice.

## 2. Identity

An asset is named by its **virtual path**: the input-relative path the packer saw, with forward
slashes. `assets/img/default/flat.png` packed from `assets/` becomes `img/default/flat.png`.

`AssetID` is `FNV1A64` of that string. Zero is reserved as "invalid", so the one input that hashes to
zero is nudged to 1 — `MakeAssetID` handles it, never call `FNV1A64` directly for this.

Hash the `(pointer, length)` pair, never `c_str()`: `StringView::c_str()` is not guaranteed
NUL-terminated.

**Collisions** are an error at pack time — `ROSE-rpkg pack` refuses and names both files, while
someone can still rename one. The runtime also re-checks the full path string on every lookup
(`Archive::FindChecked`, `AssetCatalog::Lookup`), because a hash hit silently returning the wrong
asset is the kind of bug that gets blamed on the renderer for a week.

This is deliberately *not* the `UUID` the registries use. `TextureRegistry` and `MeshRegistry` are
still keyed by `UUID::Generate()`; the catalog sits in front of them and answers "what bytes are at
this path". Making scene JSON able to name an asset stably is a separate piece of work — see
`mesh-importer.md` §4.4.

## 3. `AssetCatalog`

One process-wide singleton, shaped like `TextureRegistry::Get()`.

```cpp
MountResult Mount(const char *_rpkgPath);   // later mounts shadow earlier ones
void        SetLooseRoot(const char *_dir); // nullptr disables
void        Unmount();

AssetView Resolve(const StringView &_virtualPath);
AssetType TypeOf(const StringView &_virtualPath);
bool      Contains(const StringView &_virtualPath);
```

Resolution order is: the loose root when one is set and the file exists there, then the index. The
index is a `TypedHashMap<AssetID, Location>` built at mount time; because `TypedHashMap::insert`
overwrites on a duplicate key, indexing in mount order gives last-mounted-wins for free, which is how
a patch archive would work.

`SetLooseRoot` is the whole of the hot-loading story so far. It is **not** a virtual filesystem:
no mount points, no search-path algebra, no write side, one override directory. Loose files are read
on first use and cached in a `TypedHashMap<AssetID, RawBuffer>` until `Unmount` or the next
`SetLooseRoot` — so touching a file mid-run does not currently re-read it. Making that work is the
next obvious step and is not done.

Every `AssetView` points into a mounted archive's body or into that loose cache. All of them die at
`Unmount`. Nothing reference-counts.

## 4. Consumers

- `LoadTexture(virtualPath, name)` resolves through the catalog and decodes via
  `Surface::LoadImageFromMemory`. It is **catalog-only** on purpose: there is no second code path
  that opens a file directly, because `SetLooseRoot` already covers the unpacked-working-tree case.
- `Surface::LoadImage(path)` still exists for tools.
- `ApplicationInitSettings::AddSceneFromFile` tries the catalog first and falls back to a direct
  `std::ifstream`. That fallback is a transition measure: every example still passes a real path like
  `"assets/cube.json"` and mounts nothing.
- `MeshRegistry` is untouched. There is no mesh importer, so `AssetType::Mesh` entries round-trip as
  opaque bytes.

**Everything is unpacked at load.** ROSE is not streaming-scale and the format is built around that
assumption. It is the reason there is one codec setting per archive, no per-entry framing, and no
seek support.

## 5. The tool

```
ROSE-rpkg pack <dir> -o out.rpkg [-l 1..22] [--store]
ROSE-rpkg list <file.rpkg>
ROSE-rpkg extract <file.rpkg> -o <dir>
```

Default level is 19. `--store` writes the body uncompressed, which is useful when bisecting a
corruption problem. `list` prints the header even when the body is unreadable.

`std::filesystem`, `std::string` and `std::ofstream` are used freely in the tool — it is an authoring
boundary, not engine logic, and `conventions.md` §5 allows it there.

### CMake

`rose_pack_assets(target src_dir)` sits beside `rose_deploy_assets` and produces
`<target>.rpkg` next to the executable. Same stamp-file shape and the same reason for it: a
`POST_BUILD` command hangs off the link step, so touching an asset would never reach the build tree.
Its `DEPENDS` names `ROSE_Packer` as well as the asset files.

A target normally wants one helper or the other. `ArchiveTest` takes both on purpose, because the
override half of the test needs the same files present loose *and* packed.

## 6. Verifying a change to any of this

Round trip, which is the real correctness test — no unit-test framework exists in this repo:

```powershell
$out = "$env:TEMP\rpkg-check"
./build/debug/bin/ROSE-rpkg pack assets/ -o "$out/rose.rpkg" -l 19
./build/debug/bin/ROSE-rpkg list "$out/rose.rpkg"
./build/debug/bin/ROSE-rpkg extract "$out/rose.rpkg" -o "$out/x"
$a = Get-ChildItem -Recurse assets/ -File | Get-FileHash | Select-Object -Expand Hash | Sort-Object
$b = Get-ChildItem -Recurse "$out/x" -File | Get-FileHash | Select-Object -Expand Hash | Sort-Object
if (-not (Compare-Object $a $b)) { "byte-identical" }
```

`assets/` is the right input: thirteen files spanning PNG, SVG, OBJ and FLAC, so both compressible
text and already-compressed binary are covered. Expect a ratio around 91% — the PNGs and the FLAC
dominate and are already compressed.

Then `build/debug/bin/ArchiveTest.exe`, run from its own directory. It mounts, walks the directory,
decodes a texture out of the archive, proves the loose root shadows it, proves clearing the root
falls back, and returns non-zero on any failure.

Failure paths worth re-checking after touching `Archive::Open`: truncate an archive, flip a byte deep
in the body, and hand `list` a `.png`. Expect `truncated`, `body hash mismatch` and `bad magic`
respectively, with the header still printed for the first two.

---

## Deferred: the per-entry packaging protocol

Not built. Written down because the decision to keep compression archive-wide was made knowing what
the alternative looks like, and `RPKG_FORMAT_REVISION` exists to introduce it later.

**Per-entry codec and level.** Add `codec` and `level` columns to `AssetEntry` and drop the
archive-wide `codec` to a default. The packer would then pick per entry: store PNG, FLAC, OGG and
anything else already entropy-coded verbatim; compress JSON, GLSL, OBJ and other text hard. On the
current `assets/` tree this is most of the remaining 91% — the text files are a small minority of the
bytes, so the win is modest today and grows with the number of shaders and scenes.

The cost is that each entry becomes its own frame, which loses the cross-file matching a single frame
gets for free. With a shared zstd dictionary trained over the text entries it comes out ahead; without
one it is close to a wash. That is the actual trade, and it wants measuring on a real project's assets
rather than on thirteen placeholder files.

**Chunked payloads.** For entries above some threshold, split the payload into fixed-size blocks
(64 KiB is the usual choice) compressed independently, with a per-entry block-offset table, so a
caller can decompress only the blocks a read touches. This is what streaming audio and very large
textures would want, and it is pointless until something in ROSE streams anything. It also implies a
real `AssetStream` type, since `AssetView` is a pointer and a length by construction.

**What would force the issue:** an asset set large enough that decompressing all of it at startup is
a visible pause, or a platform where holding every asset in memory at once is not an option. Neither
is true today.
