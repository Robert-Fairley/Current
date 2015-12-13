/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Maxim Zhurovich <zhurovich@gmail.com>
          (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

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

#ifndef CURRENT_TRANSACTIONAL_STORAGE_BASE_H
#define CURRENT_TRANSACTIONAL_STORAGE_BASE_H

#include "../port.h"

#include <cassert>

#include "../TypeSystem/struct.h"

#include "../Bricks/template/typelist.h"
#include "../Bricks/template/variadic_indexes.h"

namespace current {
namespace storage {

// Instantiation types.
struct DeclareFields {};
struct CountFields {};

// Dummy type for `CountFields` instantiation type.
struct CountFieldsImplementationType {
  template <typename... T>
  CountFieldsImplementationType(T&&...) {}
  long long x[100];  // TODO(dkorolev): Fix JSON parse/serialize on Windows. Gotta go deeper. -- D.K.
};

// Storage field indexes.
template <int N>
struct Index {};

// Fields declaration and counting.
template <typename INSTANTIATION_TYPE, typename T>
struct FieldImpl;

template <typename T>
struct FieldImpl<DeclareFields, T> {
  typedef T type;
};

template <typename T>
struct FieldImpl<CountFields, T> {
  typedef CountFieldsImplementationType type;
};

template <typename INSTANTIATION_TYPE, typename T>
using Field = typename FieldImpl<INSTANTIATION_TYPE, T>::type;

template <typename T>
struct FieldCounter {
  enum {
#ifndef _MSC_VER
    value = (sizeof(typename T::template CURRENT_STORAGE_FIELDS_HELPER<T>::CURRENT_STORAGE_FIELD_COUNT_STRUCT) /
             sizeof(CountFieldsImplementationType))
#else
#endif  // _MSC_VER
  };
};

template <typename ADDER, typename DELETER>
struct FieldInfo {
  using T_ADDER = ADDER;
  using T_DELETER = DELETER;
};

template <typename FIELDS, typename INDEXES>
struct TypeListMapperImpl;

template <typename FIELDS, int... NS>
struct TypeListMapperImpl<FIELDS, current::variadic_indexes::indexes<NS...>> {
  using result = TypeList<typename std::result_of<FIELDS(Index<NS>)>::type::T_ADDER...,
                          typename std::result_of<FIELDS(Index<NS>)>::type::T_DELETER...>;
};

template <typename FIELDS, int COUNT>
using FieldsTypeList =
    typename TypeListMapperImpl<FIELDS, current::variadic_indexes::generate_indexes<COUNT>>::result;

struct MutationJournal {
  std::vector<std::unique_ptr<current::CurrentSuper>> commit_log;
  std::vector<std::function<void()>> rollback_log;

  template <typename T>
  void LogMutation(T&& entry, std::function<void()> rollback) {
    commit_log.push_back(make_unique<T>(std::move(entry)));
    rollback_log.push_back(rollback);
  }

  void Rollback() {
    for (auto rit = rollback_log.rbegin(); rit != rollback_log.rend(); ++rit) {
      (*rit)();
    }
    commit_log.clear();
    rollback_log.clear();
  }

  void AssertEmpty() const {
    assert(commit_log.empty());
    assert(rollback_log.empty());
  }
};

struct FieldsBase {
 protected:
  MutationJournal current_storage_mutation_journal_;
};

}  // namespace storage
}  // namespace current

#endif  // CURRENT_TRANSACTIONAL_STORAGE_BASE_H
