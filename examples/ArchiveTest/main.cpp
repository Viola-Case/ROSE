/**

  @file       main.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       11.09.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

/* End-to-end check for the .rpkg asset path. Opens no window: it mounts the archive CMake packed
 * next to this executable, walks it, decodes a texture out of it, then turns on the loose-file
 * override and proves the same virtual path resolves to the file on disk instead.
 *
 * Exits non-zero on the first thing that does not hold, so it is usable as a smoke test. */

#include <ROSE/ROSE.h>
#include <ROSE/Core/archive.h>

using namespace ROSE;

namespace {

  constexpr const char *ARCHIVE = "ArchiveTest.rpkg";
  constexpr const char *LOOSE_ROOT = "assets";

  /* Small, and present in the repository's assets/ directory, so it is in the archive and on disk
   * both -- which is what the override half of this test needs. */
  constexpr const char *PROBE = "img/default/missingtex.png";

  int failures = 0;

  void Check(const bool _condition, const char *_what) {
    if (_condition) {
      PrintF("  \033[32mok  \033[0m {}\n", _what);
    } else {
      PrintF("  \033[31mFAIL\033[0m {}\n", _what);
      ++failures;
    }
  }

} // namespace

int main() {
  AssetCatalog &catalog = AssetCatalog::Get();

  PrintF("\n\033[36mMounting\033[0m {}\n", ARCHIVE);
  const MountResult mounted = catalog.Mount(ARCHIVE);
  if (mounted != MountResult::Ok) {
    PrintF("\033[31mFAIL\033[0m mount failed: {}\n", MountResultToString(mounted));
    return 1;
  }

  Check(catalog.Count() > 0, "the catalog indexed at least one entry");
  Check(catalog.ArchiveCount() == 1, "exactly one archive is mounted");

  PrintF("\n\033[36mDirectory\033[0m\n");
  const Archive *archive = catalog.ArchiveAt(0);
  char           tag[5] {};
  for (uint32_t i = 0; i < archive->Count(); ++i) {
    const AssetEntry *entry = archive->At(i);
    AssetTypeToTag(entry->type, tag);
    PrintF("  {} {:>9}  {}\n", tag, entry->payloadSize, String(archive->PathOf(*entry)));
  }

  PrintF("\n\033[36mResolving\033[0m {}\n", PROBE);
  const AssetView packed = catalog.Resolve(PROBE);
  Check(packed.IsValid(), "the probe resolves out of the archive");
  Check(catalog.TypeOf(PROBE) == AssetType::Texture, "the probe is typed as a texture");
  Check(!catalog.Resolve("no/such/asset.png").IsValid(), "a missing path resolves to nothing");

  const UUID id = LoadTexture(PROBE, "missingtex");
  Check(id != UUID::Invalid(), "LoadTexture registered the probe");

  const Surface *texture = TextureRegistry::Get().GetTexture(id);
  Check(texture != nullptr && texture->IsValid(), "the registered texture decoded");
  if (texture && texture->IsValid()) {
    PrintF("  decoded {}x{}, pitch {}\n", texture->GetWidth(), texture->GetHeight(), texture->GetPitch());
  }

  /* The hot-loading half. Both copies are byte-identical here, so compare pointers rather than
   * contents: the point is that the bytes now come from the loose cache and not from the archive's
   * decompressed body. */
  PrintF("\n\033[36mLoose override\033[0m {}\n", LOOSE_ROOT);
  catalog.SetLooseRoot(LOOSE_ROOT);
  const AssetView loose = catalog.Resolve(PROBE);
  Check(loose.IsValid(), "the probe still resolves with a loose root set");
  Check(loose.data != packed.data, "the loose copy shadows the packed one");
  Check(loose.size == packed.size, "both copies are the same length");

  catalog.SetLooseRoot(nullptr);
  const AssetView again = catalog.Resolve(PROBE);
  Check(again.data == packed.data, "clearing the loose root falls back to the archive");

  catalog.Unmount();
  Check(catalog.Count() == 0, "Unmount empties the catalog");

  PrintF("\n{}\n", failures == 0 ? "\033[32mAll checks passed.\033[0m" : "\033[31mThere were failures.\033[0m");
  return failures == 0 ? 0 : 1;
}
