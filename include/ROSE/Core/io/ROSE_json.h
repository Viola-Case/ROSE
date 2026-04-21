/**

  @file      ROSE_json.h
  @brief     Native JSON value type and ROSE math/UUID converters
  @details   JsonValue is a self-contained JSON tree built entirely on ROSE's
             own RTL (List, Pair, String, UniquePtr).  No third-party headers
             are exposed.
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_rtl.h>
#include <ROSE/Core/ROSE_uuid.h>
#include <ROSE/Core/ROSE_transform.h>

namespace ROSE {

  /**
    @class   JsonValue
    @brief   Tagged-union JSON node backed entirely by ROSE RTL containers.
    @details Supports null / bool / number / string / array / object.
             Arrays are List<JsonValue>; objects are ordered List<Pair<String,JsonValue>>.
  **/
  class JsonValue {
  public:
    using Array  = List<JsonValue>;
    using Object = List<Pair<String, JsonValue>>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    JsonValue() noexcept = default;         // null
    JsonValue(nullptr_t) noexcept;          // null
    JsonValue(bool b) noexcept;
    JsonValue(int n) noexcept;
    JsonValue(double n) noexcept;
    JsonValue(const char *s);
    JsonValue(const String &s);
    JsonValue(String &&s) noexcept;

    [[nodiscard]] static JsonValue MakeArray()  noexcept;
    [[nodiscard]] static JsonValue MakeObject() noexcept;

    // -----------------------------------------------------------------------
    // Copy / move
    // -----------------------------------------------------------------------

    JsonValue(const JsonValue &other);
    JsonValue &operator=(const JsonValue &other);
    JsonValue(JsonValue &&other) noexcept;
    JsonValue &operator=(JsonValue &&other) noexcept;
    ~JsonValue() noexcept;

    // -----------------------------------------------------------------------
    // Type predicates
    // -----------------------------------------------------------------------

    [[nodiscard]] bool IsNull()   const noexcept;
    [[nodiscard]] bool IsBool()   const noexcept;
    [[nodiscard]] bool IsNumber() const noexcept;
    [[nodiscard]] bool IsString() const noexcept;
    [[nodiscard]] bool IsArray()  const noexcept;
    [[nodiscard]] bool IsObject() const noexcept;

    // -----------------------------------------------------------------------
    // Typed getters (ROSE_ASSERT on type mismatch)
    // -----------------------------------------------------------------------

    [[nodiscard]] bool          GetBool()   const noexcept;
    [[nodiscard]] double        GetNumber() const noexcept;
    [[nodiscard]] const String &GetString() const noexcept;

    [[nodiscard]] Array       &GetArray();
    [[nodiscard]] const Array &GetArray()  const;
    [[nodiscard]] Object       &GetObject();
    [[nodiscard]] const Object &GetObject() const;

    // -----------------------------------------------------------------------
    // Object operations
    // -----------------------------------------------------------------------

    [[nodiscard]] bool             Contains(const char *key) const noexcept;
    [[nodiscard]] const JsonValue *Find(const char *key) const noexcept;
    [[nodiscard]] JsonValue       *Find(const char *key)       noexcept;
    [[nodiscard]] const JsonValue &At(const char *key) const;
    [[nodiscard]] JsonValue       &At(const char *key);
    void Set(const char *key, JsonValue value);

    // -----------------------------------------------------------------------
    // Array operations
    // -----------------------------------------------------------------------

    [[nodiscard]] size_t           Size() const noexcept;
    void                           Push(JsonValue value);
    [[nodiscard]] const JsonValue &At(size_t idx) const;
    [[nodiscard]] JsonValue       &At(size_t idx);

    // -----------------------------------------------------------------------
    // Serialization
    // -----------------------------------------------------------------------

    [[nodiscard]] String           Dump(int indent = 0) const;
    [[nodiscard]] static JsonValue Parse(const char *src);
    [[nodiscard]] static JsonValue ParseFile(const char *filePath);
    void                           SaveFile(const char *filePath, int indent = 2) const;

  private:
    enum class Type : uint8_t { Null, Bool, Number, String, Array, Object };

    Type   m_type  {Type::Null};
    bool   m_bool  {false};
    double m_number{0.0};
    ROSE::String m_string;
    Array  *m_array {nullptr};
    Object *m_object{nullptr};

    void Destroy()  noexcept;
    void CopyFrom(const JsonValue &other);
  };

  // -------------------------------------------------------------------------
  // ROSE math / UUID converters
  // -------------------------------------------------------------------------

  [[nodiscard]] JsonValue JsonFromUUID(const UUID &uuid) noexcept;
  [[nodiscard]] UUID      JsonToUUID(const JsonValue &j);

  [[nodiscard]] JsonValue JsonFromVec3d(const Vec3d &v);
  [[nodiscard]] Vec3d     JsonToVec3d(const JsonValue &j);

  [[nodiscard]] JsonValue JsonFromQuatd(const Quatd &q);
  [[nodiscard]] Quatd     JsonToQuatd(const JsonValue &j);

  [[nodiscard]] JsonValue    JsonFromTransform(const Transform &t);
  [[nodiscard]] Transform    JsonToTransform(const JsonValue &j);

} // namespace ROSE
