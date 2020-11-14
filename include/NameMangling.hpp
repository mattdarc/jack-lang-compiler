#ifndef jcc_NameMangling_hpp
#define jcc_NameMangling_hpp

#include <string>

namespace jcc::builtin {

inline std::string generateName(const std::string &classStr,
                                const std::string &funcStr) {
  return "__" + classStr + "__" + funcStr;
}

}  // namespace jcc::builtin

#endif  // jcc_NameMangling_hpp
