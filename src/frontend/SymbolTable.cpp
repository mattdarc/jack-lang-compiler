#include "SymbolTable.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "JackAST.hpp"

namespace jcc::sym {

bool Table::addValue(ast::VarDecl *v) {
  auto [itr, success] = m_entries.insert({v->getName(), v});
  return success;
}

const ast::VarDecl *Table::lookup(const std::string &name) const {
  auto found = m_entries.find(name);

  return (found != m_entries.end()) ? found->second : nullptr;
}

}  // namespace jcc::sym
