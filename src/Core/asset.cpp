/**

  @file       asset.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       11.09.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
#include <ROSE/Core/archive.h>

#include <fstream>

namespace ROSE {

  void AssetTypeToTag(const AssetType _type, char *_out) noexcept {
    if (!_out) return;
    const auto tag = uint32_t(_type);
    _out[0] = char((tag >> 0) & 0xff);
    _out[1] = char((tag >> 8) & 0xff);
    _out[2] = char((tag >> 16) & 0xff);
    _out[3] = char((tag >> 24) & 0xff);
    _out[4] = '\0';
  }

  const char *MountResultToString(const MountResult _result) noexcept {
    switch (_result) {
    case MountResult::Ok:
      return "ok";
    case MountResult::NotFound:
      return "not found";
    case MountResult::BadFormat:
      return "bad format";
    case MountResult::Corrupt:
      return "corrupt";
    case MountResult::OutOfMemory:
      return "out of memory";
    case MountResult::TooManyArchives:
      return "too many archives mounted";
    }
    return "unknown";
  }

  namespace {

    MountResult ToMountResult(const ArchiveResult _result) noexcept {
      switch (_result) {
      case ArchiveResult::Ok:
        return MountResult::Ok;
      case ArchiveResult::NotFound:
        return MountResult::NotFound;
      case ArchiveResult::BadMagic:
      case ArchiveResult::BadVersion:
      case ArchiveResult::UnknownCodec:
        return MountResult::BadFormat;
      case ArchiveResult::Truncated:
      case ArchiveResult::BadHash:
      case ArchiveResult::DecodeFailed:
        return MountResult::Corrupt;
      case ArchiveResult::OutOfMemory:
        return MountResult::OutOfMemory;
      }
      return MountResult::BadFormat;
    }

    /*! The extension of a virtual path, without the dot. Empty when there is none. */
    StringView ExtensionOf(const StringView &_path) noexcept {
      const size_t size = _path.size();
      const char  *data = _path.data();
      for (size_t i = size; i > 0; --i) {
        const char c = data[i - 1];
        if (c == '/' || c == '\\') break; //!< a dot in a directory name is not an extension
        if (c == '.') return { data + i, size - i };
      }
      return {};
    }

    /*! Joins the loose root and a virtual path into something the OS will open. */
    String LoosePathFor(const String &_root, const StringView &_virtualPath) {
      String full { _root };
      if (!full.empty()) {
        const char last = full[full.size() - 1];
        if (last != '/' && last != '\\') full.push_back('/');
      }
      full += String(_virtualPath);
      return full;
    }

  } // namespace

  AssetCatalog::AssetCatalog() noexcept = default;

  /* Out of line on purpose: Archive is only forward-declared in asset.h, and destroying
   * List<UniquePtr<Archive>> needs the complete type. */
  AssetCatalog::~AssetCatalog() noexcept = default;

  AssetCatalog &AssetCatalog::Get() noexcept {
    static AssetCatalog instance;
    return instance;
  }

  MountResult AssetCatalog::Mount(const char *_rpkgPath) noexcept {
    if (!_rpkgPath) return MountResult::NotFound;

    if (m_archives.size() >= 0xffff) {
      ROSE_LOG_ERROR("Refusing to mount '{}': the catalog indexes archives by uint16_t.", _rpkgPath);
      return MountResult::TooManyArchives;
    }

    ArchiveResult result = ArchiveResult::Ok;
    Archive       archive = Archive::Open(_rpkgPath, &result);
    if (!archive.IsValid()) return ToMountResult(result);

    const auto     slot = uint16_t(m_archives.size());
    const uint32_t count = archive.Count();

    /* Later mounts shadow earlier ones, and TypedHashMap::insert overwrites on a duplicate key, so
     * indexing in mount order gives last-one-wins for free. */
    for (uint32_t i = 0; i < count; ++i) {
      const AssetEntry *entry = archive.At(i);
      m_index.insert(entry->pathHash, Location { slot, i });
    }

    m_archives.push_back(MakeUnique<Archive>(Move(archive)));

    ROSE_LOG_INFO("Mounted '{}': {} entries.", _rpkgPath, count);
    return MountResult::Ok;
  }

  void AssetCatalog::SetLooseRoot(const char *_dir) noexcept {
    m_loose.clear(); //!< cached bytes belong to the old root
    m_looseRoot = _dir ? String(_dir) : String();
    if (_dir) ROSE_LOG_INFO("Loose asset root set to '{}'; it shadows every mounted archive.", _dir);
  }

  bool AssetCatalog::HasLooseRoot() const noexcept { return !m_looseRoot.empty(); }

  void AssetCatalog::Unmount() noexcept {
    m_index.clear();
    m_loose.clear();
    m_archives.clear();
  }

  size_t AssetCatalog::Count() const noexcept { return m_index.size(); }

  size_t AssetCatalog::ArchiveCount() const noexcept { return m_archives.size(); }

  const Archive *AssetCatalog::ArchiveAt(const size_t _index) const noexcept {
    if (_index >= m_archives.size()) return nullptr;
    return m_archives[_index].get();
  }

  const AssetCatalog::Location *AssetCatalog::Lookup(const StringView &_virtualPath) noexcept {
    const AssetID id = MakeAssetID(_virtualPath);
    if (id == AssetID_Invalid) return nullptr;

    auto it = m_index.find(id);
    if (it == m_index.end()) return nullptr;

    /* Verify the whole path, not just the hash. The packer rejects collisions, so this should never
     * fire -- but a hash hit handing back the wrong asset is exactly the kind of bug that would be
     * blamed on the renderer for a week. */
    const Location   &location = (*it).second;
    const Archive    *archive = m_archives[location.archive].get();
    const AssetEntry *entry = archive->At(location.entry);
    if (!entry) return nullptr;
    if (archive->PathOf(*entry) == _virtualPath) return &location;

    ROSE_LOG_ERROR("Asset id collision on '{}'; the archive is not trustworthy.", String(_virtualPath));
    return nullptr;
  }

  AssetView AssetCatalog::ResolveLoose(const StringView &_virtualPath, const AssetID _id) noexcept {
    if (auto it = m_loose.find(_id); it != m_loose.end()) {
      const RawBuffer &cached = (*it).second;
      return { static_cast<const uint8_t *>(cached.data()), cached.size_bytes() };
    }

    const String  full = LoosePathFor(m_looseRoot, _virtualPath);
    std::ifstream ifs(full.c_str(), std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) return {};

    const auto size = size_t(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    RawBuffer buffer(size);
    if (size > 0 && !buffer.data()) return {};
    ifs.read(static_cast<char *>(buffer.data()), std::streamsize(size));

    auto inserted = m_loose.insert(_id, Move(buffer));
    if (inserted == m_loose.end()) return {};

    const RawBuffer &stored = (*inserted).second;
    return { static_cast<const uint8_t *>(stored.data()), stored.size_bytes() };
  }

  AssetView AssetCatalog::Resolve(const StringView &_virtualPath) noexcept {
    if (_virtualPath.size() == 0) return {};

    if (HasLooseRoot()) {
      const AssetID id = MakeAssetID(_virtualPath);
      if (id != AssetID_Invalid) {
        const AssetView loose = ResolveLoose(_virtualPath, id);
        if (loose.IsValid()) return loose;
      }
    }

    const Location *location = Lookup(_virtualPath);
    if (!location) return {};

    const Archive *archive = m_archives[location->archive].get();
    return archive->View(*archive->At(location->entry));
  }

  AssetType AssetCatalog::TypeOf(const StringView &_virtualPath) noexcept {
    if (const Location *location = Lookup(_virtualPath)) {
      const Archive *archive = m_archives[location->archive].get();
      return archive->At(location->entry)->type;
    }

    /* Not in any archive, which a loose-root asset need not be. Fall back to the same guess the
     * packer would have made. */
    const StringView ext = ExtensionOf(_virtualPath);
    if (ext.size() == 0) return AssetType::Raw;
    /* StringView::c_str() is not guaranteed NUL-terminated, so copy before handing it to a
     * const char * API. */
    const String terminated { ext };
    return AssetTypeFromExtension(terminated.c_str());
  }

  bool AssetCatalog::Contains(const StringView &_virtualPath) noexcept { return Resolve(_virtualPath).IsValid(); }

} // namespace ROSE
