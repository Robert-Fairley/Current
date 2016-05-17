/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2016 Maxim Zhurovich <zhurovich@gmail.com>
          (c) 2016 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

// This `test.cc` file is `#include`-d from `../test.cc`, and thus needs a header guard.

#ifndef CURRENT_TYPE_SYSTEM_EVOLUTION_TEST_CC
#define CURRENT_TYPE_SYSTEM_EVOLUTION_TEST_CC

#include "type_evolution.h"

#include "../struct.h"
#include "../variant.h"

#include "../Serialization/json.h"

#include "../../3rdparty/gtest/gtest-main.h"

namespace type_evolution_test {
// We assume that the namespace with all the structure definitions is autogenerated during the schema export
// stage.
///////////////////////////////////////////////////////////////////////////////////////////////////
// Autogenerated code {
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace current_user_namespace {
namespace SchemaV1 {

CURRENT_STRUCT(SimpleStruct) {
  CURRENT_FIELD(x, int32_t, 101);
  CURRENT_FIELD(y, int32_t, 102);
  CURRENT_FIELD(z, std::string, "foo");
  CURRENT_DEFAULT_CONSTRUCTOR(SimpleStruct) {}
};

CURRENT_STRUCT(StructWithStruct) {
  CURRENT_FIELD(s, SimpleStruct);
  CURRENT_DEFAULT_CONSTRUCTOR(StructWithStruct) {}
};

CURRENT_STRUCT(StructWithVector) {
  CURRENT_FIELD(numerical_ids, std::vector<int32_t>);
  CURRENT_DEFAULT_CONSTRUCTOR(StructWithVector) : numerical_ids({8, 50}) {}
};

CURRENT_STRUCT(StructWithVariant) {
  CURRENT_FIELD(v, (Variant<SimpleStruct, StructWithStruct>));
  CURRENT_DEFAULT_CONSTRUCTOR(StructWithVariant) {}
};

CURRENT_STRUCT(Name) {
  CURRENT_FIELD(first, std::string, "Karl");
  CURRENT_FIELD(last, std::string, "Marx");
  CURRENT_DEFAULT_CONSTRUCTOR(Name) {}
};

}  // namespacetype_evolution_test:: current_user_namespace::SchemaV1
}  // namespace type_evolution_test::current_user_namespace

CURRENT_NAMESPACE(SchemaV1) {
  CURRENT_NAMESPACE_TYPE(SimpleStruct, current_user_namespace::SchemaV1::SimpleStruct);
  CURRENT_NAMESPACE_TYPE(StructWithStruct, current_user_namespace::SchemaV1::StructWithStruct);
  CURRENT_NAMESPACE_TYPE(StructWithVector, current_user_namespace::SchemaV1::StructWithVector);
  CURRENT_NAMESPACE_TYPE(StructWithVariant, current_user_namespace::SchemaV1::StructWithVariant);
  CURRENT_NAMESPACE_TYPE(Name, current_user_namespace::SchemaV1::Name);
};

}  // namespace type_evolution_test

namespace current {
namespace type_evolution {

// Default evolution for `SimpleStruct`.
template <typename EVOLUTOR>
struct Evolve<type_evolution_test::SchemaV1, type_evolution_test::SchemaV1::SimpleStruct, EVOLUTOR> {
  template <typename INTO>
  static void Go(const typename type_evolution_test::SchemaV1::SimpleStruct& from,
                 typename INTO::SimpleStruct& into) {
    static_assert(reflection::TotalFieldCounter<typename type_evolution_test::SchemaV1::SimpleStruct>::value ==
                      reflection::TotalFieldCounter<typename INTO::SimpleStruct>::value,
                  "Total field count for `SimpleStruct` must match.");
    Evolve<type_evolution_test::SchemaV1, decltype(from.x), EVOLUTOR>::template Go<INTO>(from.x, into.x);
    Evolve<type_evolution_test::SchemaV1, decltype(from.y), EVOLUTOR>::template Go<INTO>(from.y, into.y);
    Evolve<type_evolution_test::SchemaV1, decltype(from.z), EVOLUTOR>::template Go<INTO>(from.z, into.z);
  }
};

// Default evolution for `StructWithStruct`.
template <typename EVOLUTOR>
struct Evolve<type_evolution_test::SchemaV1, type_evolution_test::SchemaV1::StructWithStruct, EVOLUTOR> {
  template <typename INTO>
  static void Go(const typename type_evolution_test::SchemaV1::StructWithStruct& from,
                 typename INTO::StructWithStruct& into) {
    static_assert(
        reflection::TotalFieldCounter<typename type_evolution_test::SchemaV1::StructWithStruct>::value ==
            reflection::TotalFieldCounter<typename INTO::StructWithStruct>::value,
        "Total field count for `StructWithStruct` must match.");
    Evolve<type_evolution_test::SchemaV1, decltype(from.s), EVOLUTOR>::template Go<INTO>(from.s, into.s);
  }
};

// Default evolution for `StructWithVector`.
// It won't compile if attempted to be used, since it tries to evolve `std::string` into `int32_t`, which is
// obviously not what we want to do by default.
template <typename EVOLUTOR>
struct Evolve<type_evolution_test::SchemaV1, type_evolution_test::SchemaV1::StructWithVector, EVOLUTOR> {
  template <typename INTO>
  static void Go(const typename type_evolution_test::SchemaV1::StructWithVector& from,
                 typename INTO::StructWithVector& into) {
    static_assert(
        reflection::TotalFieldCounter<typename type_evolution_test::SchemaV1::StructWithVector>::value ==
            reflection::TotalFieldCounter<typename INTO::StructWithVector>::value,
        "Total field count for `StructWithVector` must match.");
    into.numerical_ids.clear();
    into.numerical_ids.reserve(from.numerical_ids.size());
    for (const auto& cit : from.numerical_ids) {
      into.numerical_ids.push_back(Evolve<type_evolution_test::SchemaV1,
                                          typename decltype(from.numerical_ids)::value_type,
                                          EVOLUTOR>::template Go<INTO>(cit));
    }
  }
};

// Default evolution for `StructWithVariant`. Must auto-generate all `Variant<>` cases too.
template <typename DESTINATION_VARIANT, typename FROM_NAMESPACE, typename INTO, typename EVOLUTOR>
struct CustomVariantTypedEvolutor {
  DESTINATION_VARIANT& into;
  explicit CustomVariantTypedEvolutor(DESTINATION_VARIANT& into) : into(into) {}
  void operator()(const typename FROM_NAMESPACE::SimpleStruct& value) {
    using into_t = typename INTO::SimpleStruct;
    into = into_t();
    Evolve<FROM_NAMESPACE, typename FROM_NAMESPACE::SimpleStruct, EVOLUTOR>::template Go<INTO>(
        value, Value<into_t>(into));
  }
  void operator()(const typename FROM_NAMESPACE::StructWithStruct& value) {
    using into_t = typename INTO::StructWithStruct;
    into = into_t();
    Evolve<FROM_NAMESPACE, typename FROM_NAMESPACE::StructWithStruct, EVOLUTOR>::template Go<INTO>(
        value, Value<into_t>(into));
  }
};

template <typename EVOLUTOR>
struct Evolve<type_evolution_test::SchemaV1, type_evolution_test::SchemaV1::StructWithVariant, EVOLUTOR> {
  template <typename INTO>
  static void Go(const typename type_evolution_test::SchemaV1::StructWithVariant& from,
                 typename INTO::StructWithVariant& into) {
    static_assert(
        reflection::TotalFieldCounter<typename type_evolution_test::SchemaV1::StructWithVariant>::value ==
            reflection::TotalFieldCounter<typename INTO::StructWithVariant>::value,
        "Total field count for `StructWithVariant` must match.");
    CustomVariantTypedEvolutor<decltype(into.v), type_evolution_test::SchemaV1, INTO, EVOLUTOR> v_evolutor(
        into.v);
    from.v.Call(v_evolutor);
  }
};

// Default evolution for `Name`.
// It won't compile if attempted to be used, since we have changed the number of fields in the `Name` structure.
template <typename EVOLUTOR>
struct Evolve<type_evolution_test::SchemaV1, type_evolution_test::SchemaV1::Name, EVOLUTOR> {
  template <typename INTO>
  static void Go(const typename type_evolution_test::SchemaV1::Name& from, typename INTO::Name& into) {
    static_assert(reflection::TotalFieldCounter<typename type_evolution_test::SchemaV1::Name>::value ==
                      reflection::TotalFieldCounter<typename INTO::Name>::value,
                  "Total field count for `Name` must match.");
    into.first =
        Evolve<type_evolution_test::SchemaV1, decltype(from.first), EVOLUTOR>::template Go<INTO>(from.first);
    into.last =
        Evolve<type_evolution_test::SchemaV1, decltype(from.last), EVOLUTOR>::template Go<INTO>(from.last);
  }
};

}  // namespace current::type_evolution
}  // namespace current

///////////////////////////////////////////////////////////////////////////////////////////////////
// }  // Autogenerated code
///////////////////////////////////////////////////////////////////////////////////////////////////

// Custom evolution for `StructWithVector`.
CURRENT_TYPE_EVOLUTOR(V1ToV2Evolutor,
                      type_evolution_test::SchemaV1,
                      StructWithVector,
                      {
                        into.string_ids.clear();
                        into.string_ids.reserve(from.numerical_ids.size());
                        for (const auto& cit : from.numerical_ids) {
                          into.string_ids.push_back('@' + current::ToString(cit));
                        }
                      });

// Custom evolution for `Name`.
CURRENT_TYPE_EVOLUTOR(V1ToV2Evolutor,
                      type_evolution_test::SchemaV1,
                      Name,
                      { into.full = from.first + ' ' + from.last; });

// Alternative custom evolution for `Name`.
CURRENT_TYPE_EVOLUTOR(AnotherV1ToV2Evolutor,
                      type_evolution_test::SchemaV1,
                      Name,
                      { into.full = from.last + ", " + from.first; });

namespace type_evolution_test {
namespace arbitrarily_called_namespace {

CURRENT_STRUCT(SimpleStruct) {
  CURRENT_FIELD(x, int32_t, 201);
  CURRENT_FIELD(y, int32_t, 202);
  CURRENT_FIELD(z, std::string, "bar");
  CURRENT_DEFAULT_CONSTRUCTOR(SimpleStruct) {}
};

CURRENT_STRUCT(StructWithStruct) {
  CURRENT_FIELD(s, SimpleStruct);
  CURRENT_DEFAULT_CONSTRUCTOR(StructWithStruct) {}
};

// `StructWithVector` should evolve from `vector<int32_t> numerical_ids` in `SchemaV1` into `string_ids`.
CURRENT_STRUCT(StructWithVector) {
  CURRENT_FIELD(string_ids, std::vector<std::string>);
  CURRENT_DEFAULT_CONSTRUCTOR(StructWithVector) : string_ids({"@-1", "@112"}) {}
};

CURRENT_STRUCT(StructWithVariant) {
  CURRENT_FIELD(v, (Variant<SimpleStruct, StructWithStruct>));
  CURRENT_DEFAULT_CONSTRUCTOR(StructWithVariant) {}
};

// `Name` should evolve from two separate fields in `SchemaV1` into one full name field.
CURRENT_STRUCT(Name) {
  CURRENT_FIELD(full, std::string, "Friedrich Engels");
  CURRENT_DEFAULT_CONSTRUCTOR(Name) {}
};

}  // namespace arbitrarily_called_namespace

CURRENT_NAMESPACE(SchemaV2) {
  CURRENT_NAMESPACE_TYPE(SimpleStruct, arbitrarily_called_namespace::SimpleStruct);
  CURRENT_NAMESPACE_TYPE(StructWithStruct, arbitrarily_called_namespace::StructWithStruct);
  CURRENT_NAMESPACE_TYPE(StructWithVector, arbitrarily_called_namespace::StructWithVector);
  CURRENT_NAMESPACE_TYPE(StructWithVariant, arbitrarily_called_namespace::StructWithVariant);
  CURRENT_NAMESPACE_TYPE(Name, arbitrarily_called_namespace::Name);
};

}  // namespace type_evolution_test

TEST(TypeEvolutionTest, SimpleStruct) {
  using namespace type_evolution_test;
  {
    const SchemaV1::SimpleStruct from;
    EXPECT_EQ("{\"x\":101,\"y\":102,\"z\":\"foo\"}", JSON(from));
  }
  {
    const SchemaV2::SimpleStruct into;
    EXPECT_EQ("{\"x\":201,\"y\":202,\"z\":\"bar\"}", JSON(into));
  }
  {
    const SchemaV1::SimpleStruct original;
    SchemaV2::SimpleStruct converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::SimpleStruct>::Go<SchemaV2>(original, converted);
    EXPECT_EQ("{\"x\":101,\"y\":102,\"z\":\"foo\"}", JSON(converted));
  }
  {
    // `V1ToV2Evolutor` leaves `SimpleStruct` unaffected.
    using current::type_evolution::V1ToV2Evolutor;
    const SchemaV1::SimpleStruct original;
    SchemaV2::SimpleStruct converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::SimpleStruct, V1ToV2Evolutor>::Go<SchemaV2>(original,
                                                                                                    converted);
    EXPECT_EQ("{\"x\":101,\"y\":102,\"z\":\"foo\"}", JSON(converted));
  }
}

TEST(TypeEvolutionTest, StructWithStruct) {
  using namespace type_evolution_test;
  {
    const SchemaV1::StructWithStruct from;
    EXPECT_EQ("{\"s\":{\"x\":101,\"y\":102,\"z\":\"foo\"}}", JSON(from));
  }
  {
    const SchemaV2::StructWithStruct into;
    EXPECT_EQ("{\"s\":{\"x\":201,\"y\":202,\"z\":\"bar\"}}", JSON(into));
  }
  {
    const SchemaV1::StructWithStruct original;
    SchemaV2::StructWithStruct converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::StructWithStruct>::Go<SchemaV2>(original, converted);
    EXPECT_EQ("{\"s\":{\"x\":101,\"y\":102,\"z\":\"foo\"}}", JSON(converted));
  }
  {
    // `V1ToV2Evolutor` leaves `StructWithStruct` unaffected.
    using current::type_evolution::V1ToV2Evolutor;
    const SchemaV1::StructWithStruct original;
    SchemaV2::StructWithStruct converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::StructWithStruct, V1ToV2Evolutor>::Go<SchemaV2>(
        original, converted);
    EXPECT_EQ("{\"s\":{\"x\":101,\"y\":102,\"z\":\"foo\"}}", JSON(converted));
  }
}

TEST(TypeEvolutionTest, StructWithVector) {
  using namespace type_evolution_test;
  {
    const SchemaV1::StructWithVector from;
    EXPECT_EQ("8,50", current::strings::Join(from.numerical_ids, ','));
    const SchemaV2::StructWithVector into;
    EXPECT_EQ("@-1,@112", current::strings::Join(into.string_ids, ','));
  }
  {
    using current::type_evolution::V1ToV2Evolutor;
    const SchemaV1::StructWithVector original;
    // The default evolution won't compile because `vector<int32_t> numerical_ids` changes to
    // `vector<string> string_ids`.
    // const SchemaV2::StructWithVector converted = current::type_evolution::Evolve<SchemaV1,
    // SchemaV1::StructWithVector>::template Go<SchemaV2>(original);
    SchemaV2::StructWithVector converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::StructWithVector, V1ToV2Evolutor>::template Go<
        SchemaV2>(original, converted);
    EXPECT_EQ("@8,@50", current::strings::Join(converted.string_ids, ','));
  }
}

TEST(TypeEvolutionTest, StructWithVariant) {
  using namespace type_evolution_test;

  {
    SchemaV1::StructWithVariant from;
    from.v = SchemaV1::SimpleStruct();
    EXPECT_EQ("{\"x\":101,\"y\":102,\"z\":\"foo\"}", JSON(Value<SchemaV1::SimpleStruct>(from.v)));
  }
  {
    SchemaV1::StructWithVariant from;
    from.v = SchemaV1::StructWithStruct();
    EXPECT_EQ("{\"s\":{\"x\":101,\"y\":102,\"z\":\"foo\"}}", JSON(Value<SchemaV1::StructWithStruct>(from.v)));
  }

  {
    SchemaV2::StructWithVariant into;
    into.v = SchemaV2::SimpleStruct();
    EXPECT_EQ("{\"x\":201,\"y\":202,\"z\":\"bar\"}", JSON(Value<SchemaV2::SimpleStruct>(into.v)));
  }
  {
    SchemaV2::StructWithVariant into;
    into.v = SchemaV2::StructWithStruct();
    EXPECT_EQ("{\"s\":{\"x\":201,\"y\":202,\"z\":\"bar\"}}", JSON(Value<SchemaV2::StructWithStruct>(into.v)));
  }
  {
    SchemaV1::StructWithVariant original;
    original.v = SchemaV1::SimpleStruct();
    SchemaV2::StructWithVariant converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::StructWithVariant>::template Go<SchemaV2>(original,
                                                                                                  converted);
    ASSERT_TRUE(Exists<SchemaV2::SimpleStruct>(converted.v));
    EXPECT_EQ("{\"x\":101,\"y\":102,\"z\":\"foo\"}", JSON(Value<SchemaV2::SimpleStruct>(converted.v)));
  }
  {
    SchemaV1::StructWithVariant original;
    original.v = SchemaV1::StructWithStruct();
    SchemaV2::StructWithVariant converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::StructWithVariant>::template Go<SchemaV2>(original,
                                                                                                  converted);
    ASSERT_TRUE(Exists<SchemaV2::StructWithStruct>(converted.v));
    EXPECT_EQ("{\"s\":{\"x\":101,\"y\":102,\"z\":\"foo\"}}",
              JSON(Value<SchemaV2::StructWithStruct>(converted.v)));
  }
}

TEST(TypeEvolutionTest, NameWithDifferentEvolutors) {
  using namespace type_evolution_test;
  {
    const SchemaV2::Name SchemaV2;
    EXPECT_EQ("Friedrich Engels", SchemaV2.full);
  }
  {
    using current::type_evolution::V1ToV2Evolutor;
    const SchemaV1::Name original;
    // The default evolution won't compile because the structure layout changes from `SchemaV1` to `SchemaV2`.
    // const SchemaV2::Name converted = current::type_evolution::Evolve<SchemaV1,
    //     SchemaV1::Name>::Go<SchemaV2>(original);
    SchemaV2::Name converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::Name, V1ToV2Evolutor>::Go<SchemaV2>(original,
                                                                                            converted);
    EXPECT_EQ("Karl Marx", converted.full);
  }
  {
    // Using alternative evolution for `Name`.
    using current::type_evolution::AnotherV1ToV2Evolutor;
    const SchemaV1::Name original;
    SchemaV2::Name converted;
    current::type_evolution::Evolve<SchemaV1, SchemaV1::Name, AnotherV1ToV2Evolutor>::Go<SchemaV2>(original,
                                                                                                   converted);
    EXPECT_EQ("Marx, Karl", converted.full);
  }
}

#endif  // CURRENT_TYPE_SYSTEM_EVOLUTION_TEST_CC
