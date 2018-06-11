#include "string_view.h"

#include <cstdint>

#include "modified_xxhash.h"

// Implementation of the specializations of the std::hash template. These are in a separate .cc
// file such that the implementation is not inlined and we can easily change the implementation.

#define SEED UINT64_C(0xf8e150b342d610e3)

namespace std {

size_t hash<t3_widget::string_view>::operator()(t3_widget::string_view v) const noexcept {
  return t3_widget::ModifiedXXHash(v.data(), v.size() * sizeof(decltype(v)::value_type), SEED);
}

size_t hash<t3_widget::wstring_view>::operator()(t3_widget::wstring_view v) const noexcept {
  return t3_widget::ModifiedXXHash(v.data(), v.size() * sizeof(decltype(v)::value_type), SEED);
}

size_t hash<t3_widget::u16string_view>::operator()(t3_widget::u16string_view v) const noexcept {
  return t3_widget::ModifiedXXHash(v.data(), v.size() * sizeof(decltype(v)::value_type), SEED);
}

size_t hash<t3_widget::u32string_view>::operator()(t3_widget::u32string_view v) const noexcept {
  return t3_widget::ModifiedXXHash(v.data(), v.size() * sizeof(decltype(v)::value_type), SEED);
}

}  // namespace std