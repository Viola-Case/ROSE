/**

  @file       archive.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       11.09.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
#include <ROSE/Core/archive.h>

#include <fstream>

#include <zstd.h>

namespace ROSE {

  const char *ArchiveResultToString(const ArchiveResult _result) noexcept {
    switch (_result) {
    case ArchiveResult::Ok:
      return "ok";
    case ArchiveResult::NotFound:
      return "not found";
    case ArchiveResult::BadMagic:
      return "bad magic";
    case ArchiveResult::BadVersion:
      return "unsupported format revision";
    case ArchiveResult::Truncated:
      return "truncated";
    case ArchiveResult::BadHash:
      return "body hash mismatch";
    case ArchiveResult::UnknownCodec:
      return "unknown codec";
    case ArchiveResult::DecodeFailed:
      return "decode failed";
    case ArchiveResult::OutOfMemory:
      return "out of memory";
    }
    return "unknown";
  }

  namespace {

    constexpr uint64_t ENTRY_STRIDE = sizeof(AssetEntry);

    bool MagicMatches(const FileHeader &_header) noexcept {
      return _header.magic[0] == 'R' && _header.magic[1] == 'O' && _header.magic[2] == 'S' && _header.magic[3] == 'E';
    }

    /*!
     * Everything after this function may trust the directory. The bytes came off disk and nothing
     * signed them, so every offset is checked against the region it indexes before any pointer is
     * built from it -- a hand-edited archive should fail to mount, not read out of bounds.
     */
    bool DirectoryIsSane(const RpkgHeader &_header, const uint8_t *_body) noexcept {
      const uint64_t dirBytes = uint64_t(_header.entryCount) * ENTRY_STRIDE;
      if (dirBytes + _header.stringBytes > _header.bodySizeRaw) return false;

      const uint64_t payloadBytes = _header.bodySizeRaw - dirBytes - _header.stringBytes;
      const auto    *entries = reinterpret_cast<const AssetEntry *>(_body);
      const char    *strings = reinterpret_cast<const char *>(_body + dirBytes);

      /* The string table must end in a NUL, or PathOf on the last entry would run off the end. */
      if (_header.stringBytes > 0 && strings[_header.stringBytes - 1] != '\0') return false;

      AssetID previous = 0;
      for (uint32_t i = 0; i < _header.entryCount; ++i) {
        const AssetEntry &e = entries[i];

        if (e.pathHash == AssetID_Invalid) return false;
        if (i > 0 && e.pathHash <= previous) return false; //!< strictly ascending: sorted, and no duplicates
        previous = e.pathHash;

        if (e.payloadOffset > payloadBytes) return false;
        if (e.payloadSize > payloadBytes - e.payloadOffset) return false;

        if (uint64_t(e.pathOffset) + e.pathLength + 1 > _header.stringBytes) return false;
        if (strings[e.pathOffset + e.pathLength] != '\0') return false;
      }
      return true;
    }

    ArchiveResult Fail(ArchiveResult _result, ArchiveResult *_out) noexcept {
      if (_out) *_out = _result;
      return _result;
    }

  } // namespace

  ArchiveResult ReadArchiveHeader(const char *_path, RpkgHeader &_out) noexcept {
    if (!_path) return ArchiveResult::NotFound;

    std::ifstream ifs(_path, std::ios::binary);
    if (!ifs.is_open()) return ArchiveResult::NotFound;

    RpkgHeader header {};
    ifs.read(reinterpret_cast<char *>(&header), sizeof(RpkgHeader));
    if (ifs.gcount() != std::streamsize(sizeof(RpkgHeader))) return ArchiveResult::Truncated;

    if (!MagicMatches(header.header) || header.header.type != FileType::Archive) return ArchiveResult::BadMagic;
    if (header.formatRevision != RPKG_FORMAT_REVISION) return ArchiveResult::BadVersion;

    _out = header;
    return ArchiveResult::Ok;
  }

  Archive::Archive(Archive &&_other) noexcept
      : m_header(_other.m_header), m_body(Move(_other.m_body)), m_payloadBytes(_other.m_payloadBytes),
        m_count(_other.m_count) {
    Rebind();
    _other.m_header = RpkgHeader {};
    _other.m_entries = nullptr;
    _other.m_strings = nullptr;
    _other.m_payload = nullptr;
    _other.m_payloadBytes = 0;
    _other.m_count = 0;
  }

  Archive &Archive::operator=(Archive &&_other) noexcept {
    if (this == &_other) return *this;

    m_header = _other.m_header;
    m_body = Move(_other.m_body);
    m_payloadBytes = _other.m_payloadBytes;
    m_count = _other.m_count;
    Rebind();

    _other.m_header = RpkgHeader {};
    _other.m_entries = nullptr;
    _other.m_strings = nullptr;
    _other.m_payload = nullptr;
    _other.m_payloadBytes = 0;
    _other.m_count = 0;
    return *this;
  }

  void Archive::Rebind() noexcept {
    const auto *body = static_cast<const uint8_t *>(m_body.data());
    if (!body || m_count == 0) {
      m_entries = nullptr;
      m_strings = nullptr;
      m_payload = nullptr;
      return;
    }

    const uint64_t dirBytes = uint64_t(m_count) * ENTRY_STRIDE;
    m_entries = reinterpret_cast<const AssetEntry *>(body);
    m_strings = reinterpret_cast<const char *>(body + dirBytes);
    m_payload = body + dirBytes + m_header.stringBytes;
  }

  Archive Archive::Open(const char *_path, ArchiveResult *_result) noexcept {
    Archive archive;

    if (!_path) {
      Fail(ArchiveResult::NotFound, _result);
      return archive;
    }

    std::ifstream ifs(_path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) {
      Fail(ArchiveResult::NotFound, _result);
      return archive;
    }

    const auto fileSize = uint64_t(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    RpkgHeader header {};
    if (fileSize < sizeof(RpkgHeader)) {
      Fail(ArchiveResult::Truncated, _result);
      return archive;
    }
    ifs.read(reinterpret_cast<char *>(&header), sizeof(RpkgHeader));

    if (!MagicMatches(header.header) || header.header.type != FileType::Archive) {
      ROSE_LOG_ERROR("'{}' is not a ROSE archive.", _path);
      Fail(ArchiveResult::BadMagic, _result);
      return archive;
    }

    if (header.formatRevision != RPKG_FORMAT_REVISION) {
      ROSE_LOG_ERROR("'{}' is format revision {}, this build reads {}.", _path, header.formatRevision,
                     RPKG_FORMAT_REVISION);
      Fail(ArchiveResult::BadVersion, _result);
      return archive;
    }

    if (fileSize - sizeof(RpkgHeader) < header.bodySizeStored) {
      ROSE_LOG_ERROR("'{}' is truncated: header claims {} body bytes, {} are present.", _path, header.bodySizeStored,
                     fileSize - sizeof(RpkgHeader));
      Fail(ArchiveResult::Truncated, _result);
      return archive;
    }

    if (header.codec != ArchiveCodec::None && header.codec != ArchiveCodec::Zstd) {
      Fail(ArchiveResult::UnknownCodec, _result);
      return archive;
    }

    RawBuffer body;
    body.allocate(size_t(header.bodySizeRaw));
    if (header.bodySizeRaw > 0 && !body.data()) {
      Fail(ArchiveResult::OutOfMemory, _result);
      return archive;
    }

    if (header.codec == ArchiveCodec::None) {
      ifs.read(static_cast<char *>(body.data()), std::streamsize(header.bodySizeStored));
      if (header.bodySizeStored != header.bodySizeRaw) {
        Fail(ArchiveResult::DecodeFailed, _result);
        return archive;
      }
    } else {
      RawBuffer stored(size_t(header.bodySizeStored));
      if (header.bodySizeStored > 0 && !stored.data()) {
        Fail(ArchiveResult::OutOfMemory, _result);
        return archive;
      }
      ifs.read(static_cast<char *>(stored.data()), std::streamsize(header.bodySizeStored));

      const size_t produced =
        ZSTD_decompress(body.data(), size_t(header.bodySizeRaw), stored.data(), size_t(header.bodySizeStored));
      if (ZSTD_isError(produced)) {
        ROSE_LOG_ERROR("'{}' failed to decompress: {}", _path, ZSTD_getErrorName(produced));
        Fail(ArchiveResult::DecodeFailed, _result);
        return archive;
      }
      if (produced != header.bodySizeRaw) {
        ROSE_LOG_ERROR("'{}' decompressed to {} bytes, header claims {}.", _path, produced, header.bodySizeRaw);
        Fail(ArchiveResult::DecodeFailed, _result);
        return archive;
      }
    }

    if (FNV1A64(body.data(), size_t(header.bodySizeRaw)) != header.bodyHash) {
      ROSE_LOG_ERROR("'{}' body hash mismatch; the archive is corrupt.", _path);
      Fail(ArchiveResult::BadHash, _result);
      return archive;
    }

    if (!DirectoryIsSane(header, static_cast<const uint8_t *>(body.data()))) {
      ROSE_LOG_ERROR("'{}' has a malformed directory.", _path);
      Fail(ArchiveResult::DecodeFailed, _result);
      return archive;
    }

    archive.m_header = header;
    archive.m_body = Move(body);
    archive.m_count = header.entryCount;
    archive.m_payloadBytes = header.bodySizeRaw - uint64_t(header.entryCount) * ENTRY_STRIDE - header.stringBytes;
    archive.Rebind();

    Fail(ArchiveResult::Ok, _result);
    return archive;
  }

  const AssetEntry *Archive::Find(const AssetID _id) const noexcept {
    if (!m_entries || _id == AssetID_Invalid) return nullptr;

    uint32_t low = 0;
    uint32_t high = m_count;
    while (low < high) {
      const uint32_t mid = low + (high - low) / 2;
      const AssetID  here = m_entries[mid].pathHash;
      if (here == _id) return &m_entries[mid];
      if (here < _id) {
        low = mid + 1;
      } else {
        high = mid;
      }
    }
    return nullptr;
  }

  const AssetEntry *Archive::FindChecked(const StringView &_virtualPath) const noexcept {
    const AssetEntry *entry = Find(MakeAssetID(_virtualPath));
    if (!entry) return nullptr;

    if (entry->pathLength != _virtualPath.size()) return nullptr;
    if (MemCmp(m_strings + entry->pathOffset, _virtualPath.data(), _virtualPath.size()) != 0) return nullptr;
    return entry;
  }

  const AssetEntry *Archive::At(const uint32_t _index) const noexcept {
    if (!m_entries || _index >= m_count) return nullptr;
    return &m_entries[_index];
  }

  AssetView Archive::View(const AssetEntry &_entry) const noexcept {
    if (!m_payload) return {};
    return { m_payload + _entry.payloadOffset, _entry.payloadSize };
  }

  StringView Archive::PathOf(const AssetEntry &_entry) const noexcept {
    if (!m_strings) return {};
    return { m_strings + _entry.pathOffset, _entry.pathLength };
  }

} // namespace ROSE
