/**

  @file       main.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       11.09.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
#include <ROSE/Core/archive.h>

#include <CLI/CLI.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <zstd.h>

using namespace ROSE;

namespace {

  namespace fs = std::filesystem;

  constexpr int DEFAULT_LEVEL = 19;

  /*!
   * A file on its way into an archive. `path` is the virtual path -- the input-relative path with
   * forward slashes -- and is what gets hashed.
   */
  struct Candidate {
    std::string          path;
    fs::path             source;
    std::vector<uint8_t> bytes;
    AssetID              id { AssetID_Invalid };
    AssetType            type { AssetType::Raw };
  };

  /*! `_n` rounded up to the next multiple of `_align`, which must be a power of two. */
  uint64_t AlignUp(const uint64_t _n, const uint64_t _align) noexcept { return (_n + _align - 1) & ~(_align - 1); }

  std::string VirtualPathOf(const fs::path &_file, const fs::path &_root) {
    std::string rel = fs::relative(_file, _root).generic_string();
    return rel;
  }

  std::string ExtensionOf(const std::string &_path) {
    const size_t dot = _path.find_last_of('.');
    const size_t slash = _path.find_last_of('/');
    if (dot == std::string::npos) return {};
    if (slash != std::string::npos && dot < slash) return {};
    return _path.substr(dot + 1);
  }

  bool ReadWholeFile(const fs::path &_path, std::vector<uint8_t> &_out) {
    std::ifstream ifs(_path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) return false;

    const auto size = size_t(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    _out.resize(size);
    if (size > 0) ifs.read(reinterpret_cast<char *>(_out.data()), std::streamsize(size));
    return bool(ifs);
  }

  const char *TypeTag(const AssetType _type) {
    static char tag[5] {};
    AssetTypeToTag(_type, tag);
    return tag;
  }

  int Pack(const std::string &_inputDir, const std::string &_output, const int _level, const bool _store) {
    const fs::path  root(_inputDir);
    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
      PrintF("\033[31mERROR\033[0m '{}' is not a directory.\n", _inputDir);
      return 1;
    }

    std::vector<Candidate> candidates;
    for (const auto &dirEntry : fs::recursive_directory_iterator(root, ec)) {
      if (!dirEntry.is_regular_file()) continue;

      Candidate c;
      c.path = VirtualPathOf(dirEntry.path(), root);
      c.source = dirEntry.path();

      const std::string ext = ExtensionOf(c.path);
      c.type = AssetTypeFromExtension(ext.c_str());
      c.id = MakeAssetID(c.path.data(), c.path.size());

      if (!ReadWholeFile(c.source, c.bytes)) {
        PrintF("\033[31mERROR\033[0m could not read '{}'.\n", c.source.generic_string());
        return 1;
      }
      candidates.push_back(Move(c));
    }

    if (candidates.empty()) {
      PrintF("\033[33mWARNING\033[0m '{}' held no files; writing an empty archive.\n", _inputDir);
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) { return a.id < b.id; });

    /* A 64-bit collision is vanishingly unlikely and catastrophically confusing, so it is an error
     * here rather than something the runtime has to cope with. The reader still re-checks the full
     * string on every lookup, but this is where it gets caught while someone can rename a file. */
    for (size_t i = 1; i < candidates.size(); ++i) {
      if (candidates[i].id == candidates[i - 1].id) {
        PrintF("\033[31mERROR\033[0m asset id collision between '{}' and '{}'. Rename one.\n", candidates[i - 1].path,
               candidates[i].path);
        return 1;
      }
    }

    const auto     count = uint32_t(candidates.size());
    const uint64_t dirBytes = uint64_t(count) * sizeof(AssetEntry);

    /* The string table is padded to a 16-byte multiple so the payload blob starts aligned: the
     * directory is already a multiple of 48, which is itself a multiple of 16. */
    uint64_t stringBytes = 0;
    for (const Candidate &c : candidates)
      stringBytes += c.path.size() + 1;
    stringBytes = AlignUp(stringBytes, 16);

    uint64_t payloadBytes = 0;
    for (const Candidate &c : candidates)
      payloadBytes = AlignUp(payloadBytes + c.bytes.size(), 16);

    const uint64_t bodySize = dirBytes + stringBytes + payloadBytes;
    std::vector<uint8_t> body(size_t(bodySize), 0);

    auto    *entries = reinterpret_cast<AssetEntry *>(body.data());
    char    *strings = reinterpret_cast<char *>(body.data() + dirBytes);
    uint8_t *payload = body.data() + dirBytes + stringBytes;

    uint64_t stringCursor = 0;
    uint64_t payloadCursor = 0;
    for (uint32_t i = 0; i < count; ++i) {
      const Candidate &c = candidates[i];
      AssetEntry      &e = entries[i];

      e.pathHash = c.id;
      e.payloadOffset = payloadCursor;
      e.payloadSize = c.bytes.size();
      e.pathOffset = uint32_t(stringCursor);
      e.pathLength = uint32_t(c.path.size());
      e.type = c.type;
      e.flags = AssetFlags_None;
      e.contentHash = FNV1A64(c.bytes.data(), c.bytes.size());

      MemCpy(strings + stringCursor, c.path.data(), c.path.size());
      strings[stringCursor + c.path.size()] = '\0';
      stringCursor += c.path.size() + 1;

      if (!c.bytes.empty()) MemCpy(payload + payloadCursor, c.bytes.data(), c.bytes.size());
      payloadCursor = AlignUp(payloadCursor + c.bytes.size(), 16);
    }

    RpkgHeader header {};
    header.codec = _store ? ArchiveCodec::None : ArchiveCodec::Zstd;
    header.entryCount = count;
    header.stringBytes = uint32_t(stringBytes);
    header.bodySizeRaw = bodySize;
    header.bodyHash = FNV1A64(body.data(), size_t(bodySize));

    std::vector<uint8_t> stored;
    if (_store) {
      stored = body;
    } else {
      stored.resize(ZSTD_compressBound(size_t(bodySize)));
      const size_t produced = ZSTD_compress(stored.data(), stored.size(), body.data(), size_t(bodySize), _level);
      if (ZSTD_isError(produced)) {
        PrintF("\033[31mERROR\033[0m compression failed: {}\n", ZSTD_getErrorName(produced));
        return 1;
      }
      stored.resize(produced);
    }
    header.bodySizeStored = stored.size();

    const fs::path outPath(_output);
    if (outPath.has_parent_path()) fs::create_directories(outPath.parent_path(), ec);

    std::ofstream ofs(outPath, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
      PrintF("\033[31mERROR\033[0m could not open '{}' for writing.\n", _output);
      return 1;
    }
    ofs.write(reinterpret_cast<const char *>(&header), sizeof(RpkgHeader));
    if (!stored.empty()) ofs.write(reinterpret_cast<const char *>(stored.data()), std::streamsize(stored.size()));
    ofs.close();

    const double ratio = bodySize > 0 ? double(stored.size()) / double(bodySize) : 1.0;
    PrintF("Packed {} entries into '{}': {} -> {} bytes ({:.1f}% of raw).\n", count, _output, bodySize, stored.size(),
           ratio * 100.0);
    return 0;
  }

  int ListArchive(const std::string &_input) {
    RpkgHeader          header {};
    const ArchiveResult headerResult = ReadArchiveHeader(_input.c_str(), header);
    if (headerResult != ArchiveResult::Ok) {
      PrintF("\033[31mERROR\033[0m '{}': {}\n", _input, ArchiveResultToString(headerResult));
      return 1;
    }

    const double ratio = header.bodySizeRaw > 0 ? double(header.bodySizeStored) / double(header.bodySizeRaw) : 1.0;
    PrintF("{}\n", _input);
    PrintF("  format revision : {}\n", header.formatRevision);
    PrintF("  engine version  : {}.{}\n", header.versionMajor, header.versionMinor);
    PrintF("  codec           : {}\n", header.codec == ArchiveCodec::None ? "none" : "zstd");
    PrintF("  entries         : {}\n", header.entryCount);
    PrintF("  body            : {} -> {} bytes ({:.1f}%)\n", header.bodySizeRaw, header.bodySizeStored, ratio * 100.0);
    PrintF("\n");

    /* The directory lives inside the compressed body, so the rows cost a full decompress. The header
     * above is already out, which is the point: a corrupt body still tells you what it claimed to be. */
    ArchiveResult result = ArchiveResult::Ok;
    Archive       archive = Archive::Open(_input.c_str(), &result);
    if (!archive.IsValid()) {
      PrintF("\033[31mERROR\033[0m body unreadable: {}\n", ArchiveResultToString(result));
      return 1;
    }

    for (uint32_t i = 0; i < archive.Count(); ++i) {
      const AssetEntry *e = archive.At(i);
      const StringView  path = archive.PathOf(*e);
      PrintF("  {} {:>10}  {}\n", TypeTag(e->type), e->payloadSize, std::string(path.data(), path.size()));
    }
    return 0;
  }

  int Extract(const std::string &_input, const std::string &_outputDir) {
    ArchiveResult result = ArchiveResult::Ok;
    Archive       archive = Archive::Open(_input.c_str(), &result);
    if (!archive.IsValid()) {
      PrintF("\033[31mERROR\033[0m '{}': {}\n", _input, ArchiveResultToString(result));
      return 1;
    }

    const fs::path  root(_outputDir);
    std::error_code ec;
    fs::create_directories(root, ec);

    for (uint32_t i = 0; i < archive.Count(); ++i) {
      const AssetEntry *e = archive.At(i);
      const StringView  path = archive.PathOf(*e);
      const AssetView   view = archive.View(*e);

      const fs::path target = root / std::string(path.data(), path.size());
      if (target.has_parent_path()) fs::create_directories(target.parent_path(), ec);

      std::ofstream ofs(target, std::ios::binary | std::ios::trunc);
      if (!ofs.is_open()) {
        PrintF("\033[31mERROR\033[0m could not write '{}'.\n", target.generic_string());
        return 1;
      }
      if (view.size > 0) ofs.write(reinterpret_cast<const char *>(view.data), std::streamsize(view.size));
    }

    PrintF("Extracted {} entries into '{}'.\n", archive.Count(), _outputDir);
    return 0;
  }

} // namespace

int main(int argc, char **argv) {
  const std::string version = std::format("ROSE Package Tool, engine {}.{}.{}\n\t"
                                          "(c) Viola Case, 2026. All rights reserved.\n\n\t"
                                          "Reads and writes .rpkg asset archives.",
                                          ROSE_VERSION_MAJOR, ROSE_VERSION_MINOR, ROSE_VERSION_PATCH);

  CLI::App app { "ROSE asset archive tool" };
  app.name("ROSE-rpkg");
  app.set_version_flag("-v,--version", version);
  app.require_subcommand(1);

  std::string packInput, packOutput;
  int         level = DEFAULT_LEVEL;
  bool        store = false;

  CLI::App *packCmd = app.add_subcommand("pack", "Pack a directory into an .rpkg archive");
  packCmd->add_option("directory", packInput, "Directory whose contents become the archive")->required();
  packCmd->add_option("-o,--output", packOutput, "Output archive path")->required();
  packCmd->add_option("-l,--level", level, "zstd compression level, 1-22")->check(CLI::Range(1, ZSTD_maxCLevel()));
  packCmd->add_flag("--store", store, "Write the body uncompressed");

  std::string listInput;
  CLI::App   *listCmd = app.add_subcommand("list", "Print an archive's header and directory");
  listCmd->add_option("archive", listInput, "Archive to inspect")->required();

  std::string extractInput, extractOutput;
  CLI::App   *extractCmd = app.add_subcommand("extract", "Write every entry back out as a loose file");
  extractCmd->add_option("archive", extractInput, "Archive to extract")->required();
  extractCmd->add_option("-o,--output", extractOutput, "Destination directory")->required();

  CLI11_PARSE(app, argc, argv);

  if (packCmd->parsed()) return Pack(packInput, packOutput, level, store);
  if (listCmd->parsed()) return ListArchive(listInput);
  if (extractCmd->parsed()) return Extract(extractInput, extractOutput);

  PrintF("{}\n", app.help());
  return 1;
}
