/**

  @file       archive.h
  @brief
  @details    ~
  @author     Viola Case
  @date       11.09.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/api.h>
#include <ROSE/Core/fileformat.h>
#include <ROSE/Core/version.h>
#include <ROSE/Core/asset.h>
#include <ROSE/Core/buffer.h>

namespace ROSE {

  /*!
   * Bumped whenever the on-disk layout below changes in a way an older reader would misparse. Kept
   * separate from the engine version on purpose: the format outlives individual releases, and an
   * engine bump that does not touch the layout must not invalidate everyone's archives.
   */
  constexpr uint32_t RPKG_FORMAT_REVISION = 1;

  constexpr const char *RPKG_EXTENSION = "rpkg";

  /*! How the body is stored. One setting for the whole archive; see docs/internal/assets.md. */
  enum class ArchiveCodec : uint32_t {
    None = 0, //!< the body is on disk verbatim
    Zstd = 1,
  };

  using ArchiveFlags = uint32_t;
  constexpr ArchiveFlags ArchiveFlags_None = 0;

  /**
    The 64-byte plaintext header of an .rpkg. Little-endian throughout.

        [4]  magic           "ROSE"                                        \n
        [4]  file_kind       "ARCH"                                        \n
        [2]  version major                                                 \n
        [2]  version minor                                                 \n
        [4]  format_revision RPKG_FORMAT_REVISION                          \n
        [4]  codec           ArchiveCodec                                  \n
        [4]  flags           ArchiveFlags                                  \n
        [4]  entry_count                                                   \n
        [4]  string_bytes    size of the string table                      \n
        [8]  body_size_raw   bytes after decompression                     \n
        [8]  body_size_stored bytes on disk                                \n
        [8]  body_hash       FNV1A64 over the *raw* body                   \n
        [8]  reserved        zero                                          \n

    Then the body, as a single codec frame:

        [entry_count * 48]  AssetEntry[]   ascending by pathHash           \n
        [string_bytes]      string table   NUL-terminated virtual paths    \n
        [remainder]         payload blob   entries 16-byte aligned         \n

    The header stays uncompressed so `ROSE-rpkg list` and a corruption diagnostic work without
    decoding anything. The directory is inside the body because nothing ever consults it
    independently -- the whole body is resident from Open until Unmount.
   */
  struct RpkgHeader {
    FileHeader header { FileType::Archive };

    uint16_t versionMajor { ROSE_VERSION_MAJOR };
    uint16_t versionMinor { ROSE_VERSION_MINOR };

    uint32_t formatRevision { RPKG_FORMAT_REVISION };

    ArchiveCodec codec { ArchiveCodec::Zstd };
    ArchiveFlags flags { ArchiveFlags_None };

    uint32_t entryCount {};
    uint32_t stringBytes {}; //!< size of the string table, in bytes

    uint64_t bodySizeRaw {};    //!< bytes after decompression
    uint64_t bodySizeStored {}; //!< bytes as they sit on disk
    uint64_t bodyHash {};       //!< FNV1A64 over the raw body

    uint64_t reserved {};
  };

  static_assert(sizeof(RpkgHeader) == 64, "RpkgHeader is a wire format; padding would corrupt it");

  enum class ArchiveResult : uint8_t {
    Ok,
    NotFound,   //!< could not open the file
    BadMagic,   //!< not a ROSE file, or not an archive
    BadVersion, //!< format revision this reader does not understand
    Truncated,  //!< the file is shorter than its own header claims
    BadHash,    //!< the body decompressed, but not into what was packed
    UnknownCodec,
    DecodeFailed,
    OutOfMemory,
  };

  ROSE_API(CORE) const char *ArchiveResultToString(ArchiveResult _result) noexcept;

  /*!
   * A mounted .rpkg. Open reads the file, decompresses the body once into a single RawBuffer, and
   * points the directory, the string table and the payload blob into it. Nothing is read lazily and
   * nothing is decompressed again, which is the whole design: ROSE loads everything at startup.
   *
   * Move-only. Every AssetView it hands out dies with it.
   */
  class ROSE_API(CORE) Archive {
  public:
    Archive() noexcept = default;
    Archive(const Archive &) = delete;
    Archive &operator=(const Archive &) = delete;
    Archive(Archive &&_other) noexcept;
    Archive &operator=(Archive &&_other) noexcept;
    ~Archive() noexcept = default;

    /*!
     * Reads and validates an archive. On any failure the returned Archive is invalid and `_result`,
     * when given, says why; nothing throws and nothing is logged twice.
     */
    [[nodiscard]] static Archive Open(const char *_path, ArchiveResult *_result = nullptr) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return m_entries != nullptr; }
    [[nodiscard]] uint32_t Count() const noexcept { return m_count; }

    /*! Binary search over the directory, which the packer sorted by hash. Null on a miss. */
    [[nodiscard]] const AssetEntry *Find(AssetID _id) const noexcept;

    /*!
     * Find, plus a full string compare against the entry's recorded path. The packer rejects hash
     * collisions at pack time, so this is belt and braces -- but it is cheap and it means a
     * collision can never silently hand back the wrong asset.
     */
    [[nodiscard]] const AssetEntry *FindChecked(const StringView &_virtualPath) const noexcept;

    [[nodiscard]] const AssetEntry *At(uint32_t _index) const noexcept;

    [[nodiscard]] AssetView View(const AssetEntry &_entry) const noexcept;
    [[nodiscard]] StringView PathOf(const AssetEntry &_entry) const noexcept;

    /*! The header as it was read. Zeroed on an invalid archive. */
    [[nodiscard]] const RpkgHeader &Header() const noexcept { return m_header; }

  private:
    void Rebind() noexcept; //!< re-points the three views after a move or a fresh read

    RpkgHeader m_header {};
    RawBuffer  m_body {}; //!< the whole decompressed body; everything below points into it

    const AssetEntry *m_entries { nullptr };
    const char       *m_strings { nullptr };
    const uint8_t    *m_payload { nullptr };
    uint64_t          m_payloadBytes { 0 };
    uint32_t          m_count { 0 };
  };

  /*!
   * Reads an archive's header without decompressing its body. For `ROSE-rpkg list` and for telling a
   * truncated archive from a corrupt one.
   */
  ROSE_API(CORE) ArchiveResult ReadArchiveHeader(const char *_path, RpkgHeader &_out) noexcept;

} // namespace ROSE
