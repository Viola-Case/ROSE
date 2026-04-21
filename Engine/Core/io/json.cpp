/**

  @file      json.cpp
  @brief     JsonValue implementation: recursive-descent parser, dumper,
             and ROSE math/UUID converters
  @details   ~
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/Core/io/ROSE_json.h>
#include <ROSE/Core/ROSE_macros.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace ROSE {

  // =========================================================================
  // JsonValue — lifecycle
  // =========================================================================

  void JsonValue::Destroy() noexcept {
    delete m_array;
    delete m_object;
    m_array  = nullptr;
    m_object = nullptr;
  }

  void JsonValue::CopyFrom(const JsonValue &o) {
    m_type   = o.m_type;
    m_bool   = o.m_bool;
    m_number = o.m_number;
    m_string = o.m_string;
    m_array  = o.m_array  ? new Array(*o.m_array)   : nullptr;
    m_object = o.m_object ? new Object(*o.m_object) : nullptr;
  }

  JsonValue::JsonValue(nullptr_t) noexcept {}

  JsonValue::JsonValue(bool b) noexcept
    : m_type(Type::Bool), m_bool(b) {}

  JsonValue::JsonValue(int n) noexcept
    : m_type(Type::Number), m_number(static_cast<double>(n)) {}

  JsonValue::JsonValue(double n) noexcept
    : m_type(Type::Number), m_number(n) {}

  JsonValue::JsonValue(const char *s)
    : m_type(Type::String), m_string(s) {}

  JsonValue::JsonValue(const String &s)
    : m_type(Type::String), m_string(s) {}

  JsonValue::JsonValue(String &&s) noexcept
    : m_type(Type::String), m_string(Move(s)) {}

  JsonValue JsonValue::MakeArray() noexcept {
    JsonValue v;
    v.m_type  = Type::Array;
    v.m_array = new Array;
    return v;
  }

  JsonValue JsonValue::MakeObject() noexcept {
    JsonValue v;
    v.m_type   = Type::Object;
    v.m_object = new Object;
    return v;
  }

  JsonValue::JsonValue(const JsonValue &other) { CopyFrom(other); }

  JsonValue &JsonValue::operator=(const JsonValue &other) {
    if (this != &other) { Destroy(); CopyFrom(other); }
    return *this;
  }

  JsonValue::JsonValue(JsonValue &&other) noexcept
    : m_type(other.m_type), m_bool(other.m_bool), m_number(other.m_number),
      m_string(Move(other.m_string)), m_array(other.m_array), m_object(other.m_object) {
    other.m_type   = Type::Null;
    other.m_array  = nullptr;
    other.m_object = nullptr;
  }

  JsonValue &JsonValue::operator=(JsonValue &&other) noexcept {
    if (this != &other) {
      Destroy();
      m_type   = other.m_type;
      m_bool   = other.m_bool;
      m_number = other.m_number;
      m_string = Move(other.m_string);
      m_array  = other.m_array;
      m_object = other.m_object;
      other.m_type   = Type::Null;
      other.m_array  = nullptr;
      other.m_object = nullptr;
    }
    return *this;
  }

  JsonValue::~JsonValue() noexcept { Destroy(); }

  // =========================================================================
  // Type predicates
  // =========================================================================

  bool JsonValue::IsNull()   const noexcept { return m_type == Type::Null;   }
  bool JsonValue::IsBool()   const noexcept { return m_type == Type::Bool;   }
  bool JsonValue::IsNumber() const noexcept { return m_type == Type::Number; }
  bool JsonValue::IsString() const noexcept { return m_type == Type::String; }
  bool JsonValue::IsArray()  const noexcept { return m_type == Type::Array;  }
  bool JsonValue::IsObject() const noexcept { return m_type == Type::Object; }

  // =========================================================================
  // Typed getters
  // =========================================================================

  bool          JsonValue::GetBool()   const noexcept { ROSE_ASSERT(IsBool());   return m_bool;   }
  double        JsonValue::GetNumber() const noexcept { ROSE_ASSERT(IsNumber()); return m_number; }
  const String &JsonValue::GetString() const noexcept { ROSE_ASSERT(IsString()); return m_string; }

  JsonValue::Array &JsonValue::GetArray() {
    ROSE_ASSERT(IsArray()); return *m_array;
  }
  const JsonValue::Array &JsonValue::GetArray() const {
    ROSE_ASSERT(IsArray()); return *m_array;
  }
  JsonValue::Object &JsonValue::GetObject() {
    ROSE_ASSERT(IsObject()); return *m_object;
  }
  const JsonValue::Object &JsonValue::GetObject() const {
    ROSE_ASSERT(IsObject()); return *m_object;
  }

  // =========================================================================
  // Object operations
  // =========================================================================

  bool JsonValue::Contains(const char *key) const noexcept {
    return Find(key) != nullptr;
  }

  const JsonValue *JsonValue::Find(const char *key) const noexcept {
    if (!IsObject()) return nullptr;
    for (const auto &e : *m_object)
      if (strcmp(e.first.c_str(), key) == 0) return &e.second;
    return nullptr;
  }

  JsonValue *JsonValue::Find(const char *key) noexcept {
    if (!IsObject()) return nullptr;
    for (auto &e : *m_object)
      if (strcmp(e.first.c_str(), key) == 0) return &e.second;
    return nullptr;
  }

  const JsonValue &JsonValue::At(const char *key) const {
    const JsonValue *v = Find(key);
    if (!v) throw std::runtime_error("JsonValue: key not found");
    return *v;
  }

  JsonValue &JsonValue::At(const char *key) {
    JsonValue *v = Find(key);
    if (!v) throw std::runtime_error("JsonValue: key not found");
    return *v;
  }

  void JsonValue::Set(const char *key, JsonValue value) {
    ROSE_ASSERT(IsObject());
    if (JsonValue *existing = Find(key)) {
      *existing = Move(value);
    } else {
      m_object->push_back(Pair<String, JsonValue>{String(key), Move(value)});
    }
  }

  // =========================================================================
  // Array operations
  // =========================================================================

  size_t JsonValue::Size() const noexcept {
    if (IsArray())  return m_array->size();
    if (IsObject()) return m_object->size();
    return 0;
  }

  void JsonValue::Push(JsonValue value) {
    ROSE_ASSERT(IsArray());
    m_array->push_back(Move(value));
  }

  const JsonValue &JsonValue::At(size_t idx) const {
    ROSE_ASSERT(IsArray() && idx < m_array->size());
    return (*m_array)[idx];
  }

  JsonValue &JsonValue::At(size_t idx) {
    ROSE_ASSERT(IsArray() && idx < m_array->size());
    return (*m_array)[idx];
  }

  // =========================================================================
  // Parser
  // =========================================================================

  namespace {

    struct Parser {
      const char *p;
      const char *end;

      explicit Parser(const char *src) noexcept
        : p(src), end(src + strlen(src)) {}

      void skipWs() noexcept {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
          ++p;
      }

      char peek() const noexcept { return p < end ? *p : '\0'; }
      char next()       noexcept { return p < end ? *p++ : '\0'; }

      bool match(char c) noexcept {
        if (p < end && *p == c) { ++p; return true; }
        return false;
      }

      void expect(char c) {
        if (!match(c)) {
          char buf[48];
          snprintf(buf, sizeof(buf), "expected '%c' got '%c'", c, peek());
          throw std::runtime_error(buf);
        }
      }

      // ------------------------------------------------------------------

      JsonValue value() {
        skipWs();
        switch (peek()) {
        case 'n': return parseNull();
        case 't': return parseTrue();
        case 'f': return parseFalse();
        case '"': return parseString();
        case '[': return parseArray();
        case '{': return parseObject();
        default:
          if (peek() == '-' || (peek() >= '0' && peek() <= '9'))
            return parseNumber();
          throw std::runtime_error("JSON: unexpected character");
        }
      }

      JsonValue parseNull() {
        if (p + 4 <= end && memcmp(p, "null", 4) == 0) { p += 4; return JsonValue{}; }
        throw std::runtime_error("JSON: invalid literal, expected 'null'");
      }

      JsonValue parseTrue() {
        if (p + 4 <= end && memcmp(p, "true", 4) == 0) { p += 4; return JsonValue(true); }
        throw std::runtime_error("JSON: invalid literal, expected 'true'");
      }

      JsonValue parseFalse() {
        if (p + 5 <= end && memcmp(p, "false", 5) == 0) { p += 5; return JsonValue(false); }
        throw std::runtime_error("JSON: invalid literal, expected 'false'");
      }

      JsonValue parseNumber() {
        const char *start = p;
        if (peek() == '-') ++p;
        if (peek() == '0') { ++p; }
        else { while (p < end && *p >= '0' && *p <= '9') ++p; }
        if (p < end && *p == '.') {
          ++p;
          while (p < end && *p >= '0' && *p <= '9') ++p;
        }
        if (p < end && (*p == 'e' || *p == 'E')) {
          ++p;
          if (p < end && (*p == '+' || *p == '-')) ++p;
          while (p < end && *p >= '0' && *p <= '9') ++p;
        }
        return JsonValue(strtod(start, nullptr));
      }

      // Returns raw unescaped string contents, cursor past closing '"'.
      String parseStringRaw() {
        expect('"');
        String s;
        while (p < end && *p != '"') {
          if (*p != '\\') { s.push_back(*p++); continue; }
          ++p;  // skip backslash
          if (p >= end) throw std::runtime_error("JSON: truncated escape");
          switch (*p++) {
          case '"':  s.push_back('"');  break;
          case '\\': s.push_back('\\'); break;
          case '/':  s.push_back('/');  break;
          case 'b':  s.push_back('\b'); break;
          case 'f':  s.push_back('\f'); break;
          case 'n':  s.push_back('\n'); break;
          case 'r':  s.push_back('\r'); break;
          case 't':  s.push_back('\t'); break;
          case 'u':  appendUnicode(s);  break;
          default:   throw std::runtime_error("JSON: bad escape sequence");
          }
        }
        expect('"');
        return s;
      }

      // Append a \uXXXX (possibly surrogate-paired) code point as UTF-8.
      void appendUnicode(String &out) {
        auto readHex4 = [&]() -> unsigned {
          if (p + 4 > end) throw std::runtime_error("JSON: truncated \\u escape");
          char h[5] = {p[0], p[1], p[2], p[3], 0};
          p += 4;
          return static_cast<unsigned>(strtoul(h, nullptr, 16));
        };

        unsigned cp = readHex4();
        if (cp >= 0xD800 && cp <= 0xDBFF) {
          if (p + 2 > end || p[0] != '\\' || p[1] != 'u')
            throw std::runtime_error("JSON: expected low surrogate");
          p += 2;
          const unsigned lo = readHex4();
          cp = 0x10000u + (cp - 0xD800u) * 0x400u + (lo - 0xDC00u);
        }

        if (cp < 0x80u) {
          out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800u) {
          out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
          out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else if (cp < 0x10000u) {
          out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
          out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
          out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else {
          out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
          out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
          out.push_back(static_cast<char>(0x80u | ((cp >> 6)  & 0x3Fu)));
          out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        }
      }

      JsonValue parseString() { return JsonValue(parseStringRaw()); }

      JsonValue parseArray() {
        expect('[');
        JsonValue arr = JsonValue::MakeArray();
        skipWs();
        if (match(']')) return arr;
        for (;;) {
          arr.Push(value());
          skipWs();
          if (match(']')) break;
          expect(',');
        }
        return arr;
      }

      JsonValue parseObject() {
        expect('{');
        JsonValue obj = JsonValue::MakeObject();
        skipWs();
        if (match('}')) return obj;
        for (;;) {
          skipWs();
          String key = parseStringRaw();
          skipWs();
          expect(':');
          obj.Set(key.c_str(), value());
          skipWs();
          if (match('}')) break;
          expect(',');
        }
        return obj;
      }
    };

  } // anonymous namespace

  JsonValue JsonValue::Parse(const char *src) {
    Parser p{src};
    JsonValue v = p.value();
    p.skipWs();
    if (p.p != p.end)
      throw std::runtime_error("JSON: trailing content after root value");
    return v;
  }

  JsonValue JsonValue::ParseFile(const char *filePath) {
    std::ifstream f(filePath, std::ios::binary | std::ios::ate);
    if (!f.is_open())
      throw std::runtime_error("JsonValue::ParseFile: cannot open file");
    const auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    List<char> buf(sz + 1, '\0');
    f.read(buf.data(), static_cast<std::streamsize>(sz));
    return Parse(buf.data());
  }

  // =========================================================================
  // Dumper
  // =========================================================================

  namespace {

    void indent(String &out, int spaces, int depth) {
      const int n = spaces * depth;
      for (int i = 0; i < n; ++i) out.push_back(' ');
    }

    void dumpString(const String &s, String &out) {
      out.push_back('"');
      for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        switch (c) {
        case '"':  out.append("\\\""); break;
        case '\\': out.append("\\\\"); break;
        case '\b': out.append("\\b");  break;
        case '\f': out.append("\\f");  break;
        case '\n': out.append("\\n");  break;
        case '\r': out.append("\\r");  break;
        case '\t': out.append("\\t");  break;
        default:
          if (c < 0x20u) {
            char esc[7];
            snprintf(esc, sizeof(esc), "\\u%04x", c);
            out.append(esc);
          } else {
            out.push_back(static_cast<char>(c));
          }
        }
      }
      out.push_back('"');
    }

    void dumpValue(const JsonValue &v, String &out, int spaces, int depth) {
      if (v.IsNull())   { out.append("null");                        return; }
      if (v.IsBool())   { out.append(v.GetBool() ? "true" : "false"); return; }
      if (v.IsNumber()) {
        const double n = v.GetNumber();
        if (std::isnan(n) || std::isinf(n)) { out.append("null"); return; }
        char buf[32];
        snprintf(buf, sizeof(buf), "%.17g", n);
        out.append(buf);
        return;
      }
      if (v.IsString()) { dumpString(v.GetString(), out); return; }

      if (v.IsArray()) {
        const auto &arr = v.GetArray();
        out.push_back('[');
        for (size_t i = 0; i < arr.size(); ++i) {
          if (i > 0) out.push_back(',');
          if (spaces > 0) { out.push_back('\n'); indent(out, spaces, depth + 1); }
          dumpValue(arr[i], out, spaces, depth + 1);
        }
        if (spaces > 0 && !arr.empty()) { out.push_back('\n'); indent(out, spaces, depth); }
        out.push_back(']');
        return;
      }

      if (v.IsObject()) {
        const auto &obj = v.GetObject();
        out.push_back('{');
        for (size_t i = 0; i < obj.size(); ++i) {
          if (i > 0) out.push_back(',');
          if (spaces > 0) { out.push_back('\n'); indent(out, spaces, depth + 1); }
          dumpString(obj[i].first, out);
          out.push_back(':');
          if (spaces > 0) out.push_back(' ');
          dumpValue(obj[i].second, out, spaces, depth + 1);
        }
        if (spaces > 0 && !obj.empty()) { out.push_back('\n'); indent(out, spaces, depth); }
        out.push_back('}');
      }
    }

  } // anonymous namespace

  String JsonValue::Dump(int indent) const {
    String out;
    dumpValue(*this, out, indent, 0);
    return out;
  }

  void JsonValue::SaveFile(const char *filePath, int indent) const {
    const String dumped = Dump(indent);
    std::ofstream f(filePath);
    if (!f.is_open())
      throw std::runtime_error("JsonValue::SaveFile: cannot write file");
    f.write(dumped.c_str(), static_cast<std::streamsize>(dumped.size()));
  }

  // =========================================================================
  // ROSE math / UUID converters
  // =========================================================================

  JsonValue JsonFromUUID(const UUID &uuid) noexcept {
    char buf[34];
    snprintf(buf, sizeof(buf), "%016llx-%016llx",
             static_cast<unsigned long long>(uuid.high),
             static_cast<unsigned long long>(uuid.low));
    return JsonValue(buf);
  }

  UUID JsonToUUID(const JsonValue &j) {
    const String &s = j.GetString();
    UUID uuid{};
    if (s.size() == 33) {
      uuid.high = static_cast<uint64_t>(strtoull(s.c_str(),      nullptr, 16));
      uuid.low  = static_cast<uint64_t>(strtoull(s.c_str() + 17, nullptr, 16));
    }
    return uuid;
  }

  JsonValue JsonFromVec3d(const Vec3d &v) {
    JsonValue j = JsonValue::MakeObject();
    j.Set("x", JsonValue(v.x));
    j.Set("y", JsonValue(v.y));
    j.Set("z", JsonValue(v.z));
    return j;
  }

  Vec3d JsonToVec3d(const JsonValue &j) {
    return { j.At("x").GetNumber(), j.At("y").GetNumber(), j.At("z").GetNumber() };
  }

  JsonValue JsonFromQuatd(const Quatd &q) {
    JsonValue j = JsonValue::MakeObject();
    j.Set("w", JsonValue(q.w));
    j.Set("x", JsonValue(q.x));
    j.Set("y", JsonValue(q.y));
    j.Set("z", JsonValue(q.z));
    return j;
  }

  Quatd JsonToQuatd(const JsonValue &j) {
    return {
      j.At("w").GetNumber(), j.At("x").GetNumber(),
      j.At("y").GetNumber(), j.At("z").GetNumber()
    };
  }

  JsonValue JsonFromTransform(const Transform &t) {
    JsonValue j = JsonValue::MakeObject();
    j.Set("position", JsonFromVec3d(t.position));
    j.Set("rotation", JsonFromQuatd(t.rotation));
    j.Set("scale",    JsonFromVec3d(t.scale));
    return j;
  }

  Transform JsonToTransform(const JsonValue &j) {
    return {
      JsonToVec3d(j.At("position")),
      JsonToQuatd(j.At("rotation")),
      JsonToVec3d(j.At("scale"))
    };
  }

} // namespace ROSE
