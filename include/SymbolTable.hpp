#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

namespace jcc {

namespace ast {
class VarDecl;
}

namespace sym {

enum class Kind { STATIC = 0, FIELD, ARG, VAR, NONE, NUM_KINDS };

std::string toSegment(const Kind kind);

class Table {
public:
  using TableEntries = std::unordered_map<std::string, ast::VarDecl *>;

  Table(std::string name) : m_entries{}, m_name{std::move(name)} {}

  const std::string &getName() const { return m_name; }
  const ast::VarDecl *lookup(const std::string &) const;
  bool addValue(ast::VarDecl *);

private:
  TableEntries m_entries;
  std::string m_name;
};

}  // namespace sym

}  // namespace jcc

#endif
