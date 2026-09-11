/**

    @file      asset.h
    @brief
    @details   ~
    @author    Viola Case
    @date      05.05.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <cstdint>

#include <ROSE/Core/api.h>
#include <ROSE/Core/utility.h>
#include <ROSE/Core/rtl.h>

namespace ROSE {

  class Archive;

  enum class AssetType : uint32_t {
    Texture = CharTagToInt32("TXTR"),
    Audio = CharTagToInt32("AUDO"),
    Mesh = CharTagToInt32("MESH"),
    Shader = CharTagToInt32("SHDR"),
    Prefab = CharTagToInt32("PRFB"),
    Material = CharTagToInt32("MTRL"),
    Animation = CharTagToInt32("ANIM"),
    Font = CharTagToInt32("FONT"),
    Script = CharTagToInt32("SRPT"),
    Scene = CharTagToInt32("SCNE"),
    Raw = CharTagToInt32("RAWF"),
  };

  /* Per-entry flags. AssetFlags_Compressed used to live here and is gone: compression is a property of
   * the archive, not of an entry, so there is nothing an entry could truthfully say about it. See the
   * deferred per-entry packaging protocol in docs/internal/assets.md. */
  using AssetFlags = uint32_t;
  constexpr AssetFlags AssetFlags_None = 0;

  /*!
   * Identifies one asset: FNV1A64 of its virtual path, which is the `/`-separated path the asset had
   * relative to the packer's input directory. Zero is reserved and never a valid id, so a
   * default-constructed id is a miss rather than a silent hit on whatever happens to hash to zero.
   */
  using AssetID = uint64_t;

  constexpr AssetID AssetID_Invalid = 0;

  /*!
   * Hashes a virtual path into an AssetID. Takes a pointer and a length rather than a StringView
   * because StringView::c_str() is not guaranteed NUL-terminated.
   */
  inline AssetID MakeAssetID(const char *_path, size_t _length) noexcept {
    if (!_path || _length == 0) return AssetID_Invalid;
    const uint64_t h = FNV1A64(_path, _length);
    return h == AssetID_Invalid ? 1 : h; //!< the one value we cannot represent, nudged out of the way
  }

  inline AssetID MakeAssetID(const StringView &_path) noexcept { return MakeAssetID(_path.data(), _path.size()); }

  /*!
   * A non-owning window onto asset bytes. Points into a mounted archive's decompressed body, or into a
   * loose file the catalog has cached, and stays valid until that archive is unmounted.
   */
  struct AssetView {
    const uint8_t *data { nullptr };
    uint64_t       size { 0 };

    [[nodiscard]] constexpr bool IsValid() const noexcept { return data != nullptr; }
  };

  /*!
   * One row of an .rpkg directory, and the whole of what the runtime knows about an asset before
   * something type-specific decodes it. Written to disk verbatim, so it is standard-layout, fixed
   * width and little-endian; do not reorder the members without bumping RPKG_FORMAT_REVISION.
   */
  struct AssetEntry {
    AssetID    pathHash;      //!< lookup key, and the ascending sort key of the directory
    uint64_t   payloadOffset; //!< from the start of the payload blob; always 16-byte aligned
    uint64_t   payloadSize;   //!< bytes
    uint32_t   pathOffset;    //!< byte offset into the string table
    uint32_t   pathLength;    //!< bytes, excluding the NUL
    AssetType  type;
    AssetFlags flags;
    uint64_t   contentHash; //!< FNV1A64 of the payload; how hot reload notices a change
  };

  static_assert(sizeof(AssetEntry) == 48, "AssetEntry is a wire format; padding would corrupt it");
  static_assert(alignof(AssetEntry) == 8);

  namespace detail {
    /*!
     * Case-insensitive compare of a caller-supplied extension against a lower-case literal.
     * `_literal` must already be lower-case; only `_ext` is folded.
     */
    constexpr bool ExtensionEquals(const char *_ext, const char *_literal) noexcept {
      size_t i = 0;
      for (; _literal[i] != '\0'; ++i) {
        if (_ext[i] == '\0') return false;
        if (ToLower(_ext[i]) != _literal[i]) return false;
      }
      return _ext[i] == '\0';
    }
  } // namespace detail

  /*!
   * Guesses an AssetType from a file extension, without the leading dot. Anything unrecognised is
   * AssetType::Raw, which is a legitimate answer and not an error -- an archive carries whatever it is
   * given. Ported from the old AssetMaker's ParseAssetExtensionType.
   */
  constexpr AssetType AssetTypeFromExtension(const char *_ext) noexcept {
    if (!_ext) return AssetType::Raw;

    constexpr const char *textureExts[] { "png", "jpg", "jpeg", "bmp", "tga", "tiff", "dds" };
    for (const char *e : textureExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Texture;
    }

    constexpr const char *meshExts[] { "obj", "fbx", "gltf", "glb" };
    for (const char *e : meshExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Mesh;
    }

    constexpr const char *audioExts[] { "ogg", "mp3", "flac", "wav" };
    for (const char *e : audioExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Audio;
    }

    constexpr const char *shaderExts[] { "glsl", "hlsl", "vert", "frag", "shader", "cg", "fxh", "hlsli" };
    for (const char *e : shaderExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Shader;
    }

    constexpr const char *fontExts[] { "ttf", "otf", "svg" };
    for (const char *e : fontExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Font;
    }

    constexpr const char *scriptExts[] { "lua", "luac", "rosescript" };
    for (const char *e : scriptExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Script;
    }

    constexpr const char *sceneExts[] { "json", "rosescene" };
    for (const char *e : sceneExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Scene;
    }

    constexpr const char *animExts[] { "anim", "roseanim" };
    for (const char *e : animExts) {
      if (detail::ExtensionEquals(_ext, e)) return AssetType::Animation;
    }

    return AssetType::Raw;
  }

  /*!
   * Renders an AssetType back into its four-character tag. Writes five bytes into `_out`, the last a
   * NUL, so the caller's buffer must hold at least that.
   */
  ROSE_API(CORE) void AssetTypeToTag(AssetType _type, char *_out) noexcept;

  enum class MountResult : uint8_t {
    Ok,
    NotFound,  //!< the file is not there, or could not be opened
    BadFormat, //!< magic, file type or format revision did not match
    Corrupt,   //!< the body did not survive its hash check, or the codec rejected it
    OutOfMemory,
    TooManyArchives, //!< the catalog indexes archives by uint16_t
  };

  ROSE_API(CORE) const char *MountResultToString(MountResult _result) noexcept;

  /*!
   * Where assets come from. One process-wide catalog of mounted .rpkg archives, plus an optional loose
   * directory that shadows them for development.
   *
   * Resolution order is: the loose root when one is set and the file exists there, then mounted
   * archives from newest mount to oldest. A later Mount therefore shadows an earlier one, which is how
   * a patch archive would work.
   *
   * This is deliberately not a virtual filesystem. There are no mount points, no search-path algebra
   * and no write side -- one flat namespace of virtual paths, and one override directory.
   */
  class ROSE_API(CORE) AssetCatalog {
  public:
    static AssetCatalog &Get() noexcept;

    /*!
     * Opens an archive and folds its directory into the index. The archive stays mounted, and every
     * AssetView handed out points into its decompressed body, until Unmount.
     */
    MountResult Mount(const char *_rpkgPath) noexcept;

    /*!
     * Points the catalog at a directory of loose files that shadow the archives. Pass nullptr to turn
     * the override off. Files are read on first use and cached until Unmount.
     */
    void SetLooseRoot(const char *_dir) noexcept;

    [[nodiscard]] bool HasLooseRoot() const noexcept;

    /*! Drops every mounted archive and every cached loose file. All outstanding AssetViews die. */
    void Unmount() noexcept;

    [[nodiscard]] AssetView Resolve(const StringView &_virtualPath) noexcept;
    [[nodiscard]] AssetType TypeOf(const StringView &_virtualPath) noexcept;
    [[nodiscard]] bool Contains(const StringView &_virtualPath) noexcept;

    /*! Total entries indexed across every mounted archive, shadowed duplicates counted once. */
    [[nodiscard]] size_t Count() const noexcept;

    /*! How many archives are mounted. */
    [[nodiscard]] size_t ArchiveCount() const noexcept;

    /*!
     * The archive mounted at `_index`, or nullptr. Mount order, oldest first. For tooling that wants
     * to walk a directory; ordinary asset access goes through Resolve.
     */
    [[nodiscard]] const Archive *ArchiveAt(size_t _index) const noexcept;

  private:
    AssetCatalog() noexcept;
    ~AssetCatalog() noexcept;
    AssetCatalog(const AssetCatalog &) = delete;
    AssetCatalog &operator=(const AssetCatalog &) = delete;

    /*! Which archive, and which row of its directory. */
    struct Location {
      uint16_t archive;
      uint32_t entry;
    };

    [[nodiscard]] const Location *Lookup(const StringView &_virtualPath) noexcept;
    [[nodiscard]] AssetView ResolveLoose(const StringView &_virtualPath, AssetID _id) noexcept;

    List<UniquePtr<Archive>>         m_archives {};
    TypedHashMap<AssetID, Location>  m_index {};
    String                           m_looseRoot {};
    TypedHashMap<AssetID, RawBuffer> m_loose {};
  };

} // namespace ROSE
