#include "JackAST.hpp"

#include <algorithm>

#include "JackWriter.hpp"

// TODO(matt): need error handling during IR generation

namespace jcc {

namespace ast {

void VarDecList::push_back(std::unique_ptr<VarDecl> expr, sym::Kind kind) {
  switch (kind) {
    case sym::Kind::STATIC:
      statics.push_back(std::move(expr));
      break;
    case sym::Kind::FIELD:
      fields.push_back(std::move(expr));
      break;
    default:
      assert(false);
  }
}

void ClassDecl::addFunction(std::unique_ptr<FunctionDecl> fun) {
  fun->setParent(this);
  m_functions.push_back(std::move(fun));
}

void ClassDecl::addMethod(std::unique_ptr<FunctionDecl> mth) {
  mth->setParent(this);
  m_methods.push_back(std::move(mth));
}

void ClassDecl::addField(std::unique_ptr<VarDecl> field) {
  m_table.addValue(field.get());
  m_fields.push_back(std::move(field));
}

unsigned ClassDecl::getFieldIdx(const std::string &name) const {
  auto toIndex = [&](auto itr) {
    return static_cast<unsigned>(std::distance(fields_begin(), itr));
  };

  auto index = toIndex(std::find_if(
      fields_begin(), fields_end(),
      [&name](const auto &var) { return name == var->getName(); }));
  assert(index < toIndex(fields_end()));
  return index;
}
void ClassDecl::addStatic(std::unique_ptr<VarDecl> var) {
  m_table.addValue(var.get());
  m_statics.push_back(std::move(var));
}

const std::string &NamedValue::getType() const {
  assert(m_parent);  // This error should be handled in the frontend
  auto ent = m_parent->getTable().lookup(getName());
  if (!ent) {
    // Check the class table. We only have two layers of tables in Jack
    ent = m_parent->getParent()->getTable().lookup(getName());
  }
  assert(ent);  // If the symbol is not defined, we messed up in the frontend
  return ent->getType();
}

// TODO(matt): The native Jack codegen should implement this piece

// using SizeArray = std::array<int64_t, static_cast<size_t>(Kind::NUM_KINDS)>;
// int64_t varCount(Kind kind) const {
//     return m_counts[static_cast<size_t>(kind)];
// }

// int64_t &varCount(Kind kind) { return m_counts[static_cast<size_t>(kind)]; }

// std::string toSegment(const Kind kind) {
//     std::string segment{};
//     switch (kind) {
//         case Kind::STATIC:
//             segment = "static";
//             break;
//         case Kind::FIELD:
//             segment = "this";
//             break;
//         case Kind::ARG:
//             segment = "argument";
//             break;
//         case Kind::VAR:
//             segment = "local";
//             break;
//         default:
//             assert(false);  // should never get here
//     }
//     return segment;
// }

// std::ostream &operator<<(std::ostream &os, sym::Kind k) {
//     using namespace sym;
//     switch (k) {
//         case Kind::STATIC:
//             os << "STATIC";
//             break;
//         case Kind::FIELD:
//             os << "FIELD";
//             break;
//         case Kind::ARG:
//             os << "ARG";
//             break;
//         case Kind::VAR:
//             os << "VAR";
//             break;
//         case Kind::NONE:
//             os << "NONE";
//             break;
//         case Kind::NUM_KINDS:
//             os << "NUM_KINDS";
//             break;
//     }
//     return os;
// }

// std::ostream &operator<<(std::ostream &os, const sym::TableEntry *ent) {
//     os << "  [ TYPE ]  : " << ent->getType() << '\n'
//        << "  [ KIND ]  : " << ent->getKind() << '\n'
//        << "  [ INDEX ] : " << ent->getIndex() << '\n';
//     return os;
// }

// std::ostream &operator<<(std::ostream &os, const sym::Table *node) {
//     for (const auto &s : node->getEntries()) {
//         os << " === " << s.first << " ===\n" << s.second.get() << '\n';
//     }
//     return os;
// }

// void IntConst::generateCode(io::JackWriter &writer) const {
//     writer.writePush("constant", m_int);
// }

// void IntConst::constructTable(sym::TableNode *) {}

// void Identifier::generateCode(io::JackWriter &writer) const {
//     writer.writePush(sym::toSegment(m_entry->getKind()),
//     m_entry->getIndex());
// }

// void Identifier::constructTable(sym::TableNode *table) {
//     m_entry = table->getEntry(m_identifier);
// }

// void StrConst::generateCode(io::JackWriter &writer) const {
//     writer.writePush("constant", m_str.size());
//     writer.writeCall("String.new", 1);
//     for (const auto c : m_str) {
//         writer.writePush("constant", static_cast<int>(c));
//         writer.writeCall("String.appendChar", 2);
//     }
// }
// void StrConst::constructTable(sym::TableNode *) {}

// void Keyword::generateCode(io::JackWriter &writer) const {
//     switch (m_keyword.m_type) {
//         case tok::Keyword::Type::TRUE:
//             writer.writePush("constant", 1);
//             break;
//         case tok::Keyword::Type::NULL_:
//         case tok::Keyword::Type::FALSE:
//             writer.writePush("constant", 0);
//             break;
//         case tok::Keyword::Type::THIS:
//             writer.writePush("pointer", 0);
//             break;
//         default:
//             assert("Unsupported keyword type");
//     }
// }

// void BinaryExpr::generateCode(io::JackWriter &writer) const {
//     m_lhs->generateCode(writer);
//     m_rhs->generateCode(writer);
//     writer.writeBinOp(m_op);
// }

// void BinaryExpr::constructTable(sym::TableNode *table) {
//     m_lhs->constructTable(table);
//     m_rhs->constructTable(table);
// }

// void UnaryExpr::generateCode(io::JackWriter &writer) const {
//     m_operand->generateCode(writer);
//     writer.writeUnaryOp(m_op);
// }

// void UnaryExpr::constructTable(sym::TableNode *table) {
//     m_operand->constructTable(table);
// }

// void CallExpr::generateCode(io::JackWriter &writer) const {
//     auto ident = m_callee->getIdentifier();
//     size_t nArgs = 0;
//     const sym::TableEntry *entry = nullptr;
//     if (ident.empty() || (entry = m_table->getEntry(ident))) {
//         // push this
//         writer.writePush("pointer", 0);
//         writer.writePop("argument", nArgs++);

//         // use class name
//         ident = entry ? entry->getType() : m_table->getName();
//     }

//     for (const auto &arg : m_args) {
//         arg->generateCode(writer);
//         writer.writePop("argument", nArgs++);
//     }

//     writer.writeCall(ident + '.' + m_name.getIdentifier(), nArgs);
//     writer.writePop("temp", 0);
// }

// void CallExpr::constructTable(sym::TableNode *parent) {
//     m_name.constructTable(parent);
//     m_callee->constructTable(parent);

//     for (auto &arg : m_args) {
//         arg->constructTable(parent);
//     }

//     m_table = parent->getParent()->getNode(m_name);
// }

// void LetExpr::generateCode(io::JackWriter &writer) const {
//     m_expr->generateCode(writer);
//     m_assignee->generateCode(writer);
// }

// void LetExpr::constructTable(sym::TableNode *parent) {
//     m_expr->constructTable(parent);
//     m_assignee->constructTable(parent);
// }

// void VarDecl::generateCode(io::JackWriter &writer) const {
//     writer.writePush(sym::toSegment(m_kind), 0);  // TODO(matt): needs the
//     index
// }

// void VarDecl::constructTable(sym::TableNode *parent) {
//     parent->addEntry(m_name, std::make_unique<sym::TableEntry>(m_type,
//     m_kind));
// }

// void IfExpr::generateCode(io::JackWriter &writer) const {
//     const std::string elseLabel = "ELSE_" + m_suffix;
//     const std::string endLabel = "END_" + m_suffix;

//     m_condition->generateCode(writer);
//     writer.writeUnaryOp('~');
//     writer.writeGoto(elseLabel);
//     m_ifBranch->generateCode(writer);
//     writer.writeGoto(endLabel);
//     writer.writeLabel(elseLabel);
//     m_elseBranch->generateCode(writer);
//     writer.writeLabel(endLabel);
// }

// void IfExpr::constructTable(sym::TableNode *parent) {
//     m_condition->constructTable(parent);
//     m_ifBranch->constructTable(parent);
//     m_elseBranch->constructTable(parent);
// }

// void WhileExpr::generateCode(io::JackWriter &writer) const {
//     const std::string beginLoop = "LOOP_" + m_suffix;
//     const std::string endLoop = "END_LOOP_" + m_suffix;

//     // write beginloop label
//     writer.writeLabel(beginLoop);
//     m_condition->generateCode(writer);

//     // jump to end if condition is false
//     writer.writeUnaryOp('~');
//     writer.writeIf(endLoop);

//     m_body->generateCode(writer);

//     // jump to beginloop
//     writer.writeGoto(beginLoop);

//     // endloop label
//     writer.writeLabel(endLoop);
// }

// void WhileExpr::constructTable(sym::TableNode *parent) {
//     m_condition->constructTable(parent);
//     m_body->constructTable(parent);
// }

// void ReturnExpr::generateCode(io::JackWriter &writer) const {
//     m_expr->generateCode(writer);
//     writer.writeReturn();
// }

// void ReturnExpr::constructTable(sym::TableNode *parent) {
//     m_expr->constructTable(parent);
// }

// void ClassDecl::generateCode(io::JackWriter &writer) const {
//     std::for_each(m_methods.begin(), m_methods.end(),
//                   [&](const auto &e) { e->generateCode(writer); });
//     std::for_each(m_functions.begin(), m_functions.end(),
//                   [&](const auto &e) { e->generateCode(writer); });
// }

// void ClassDecl::constructTable(sym::TableNode *) {
//     m_table = std::make_unique<sym::TableNode>(m_name);
//     std::for_each(m_statics.begin(), m_statics.end(),
//                   [&](const auto &e) { e->constructTable(m_table.get()); });
//     std::for_each(m_fields.begin(), m_fields.end(),
//                   [&](const auto &e) { e->constructTable(m_table.get()); });
//     std::for_each(m_methods.begin(), m_methods.end(),
//                   [&](const auto &e) { e->constructTable(m_table.get()); });
//     std::for_each(m_functions.begin(), m_functions.end(),
//                   [&](const auto &e) { e->constructTable(m_table.get()); });
// }

// void StaticDeclExpr::generateCode(io::JackWriter &writer) const {
//     writer.writeFunction(m_table->getParent()->getName() + "." + m_name,
//                          m_params.size());
// }

// void StaticDeclExpr::constructTable(sym::TableNode *parent) {
//     auto newTable = std::make_unique<sym::TableNode>(m_name);
//     m_table = newTable.get();
//     for (const auto &param : m_params) {
//         param->constructTable(newTable.get());
//     }
//     auto *rawTable = newTable.get();
//     parent->addChild(std::move(newTable));
//     m_body->constructTable(rawTable);
// }

// void MethodDeclExpr::generateCode(io::JackWriter &writer) const {
//     writer.writePush("argument", 0);
//     writer.writePop("pointer", 0);
//     writer.writeFunction(m_table->getParent()->getName() + "." + m_name,
//                          m_params.size());
// }

// void MethodDeclExpr::constructTable(sym::TableNode *parent) {
//     auto newTable = std::make_unique<sym::TableNode>(m_name);
//     m_table = newTable.get();
//     for (const auto &param : m_params) {
//         param->constructTable(newTable.get());
//     }
//     m_body->constructTable(newTable.get());
//     parent->addChild(std::move(newTable));
// }

// void ConstructorDeclExpr::generateCode(io::JackWriter &writer) const {
//     writer.writePush("constant",
//                      m_table->getParent()->varCount(sym::Kind::FIELD));
//     writer.writeCall("Memory.alloc", 1);
//     writer.writePop("pointer", 0);
//     writer.writeFunction(m_table->getParent()->getName() + "." + m_name,
//                          m_params.size());
// }

// void ConstructorDeclExpr::constructTable(sym::TableNode *parent) {
//     auto newTable = std::make_unique<sym::TableNode>(m_name);
//     m_table = newTable.get();
//     for (const auto &param : m_params) {
//         param->constructTable(newTable.get());
//     }
//     m_body->constructTable(newTable.get());
//     parent->addChild(std::move(newTable));
// }

// void Block::generateCode(io::JackWriter &writer) const {
//     for (const auto &expr : m_exprs) {
//         expr->generateCode(writer);
//     }
// }

// void Block::constructTable(sym::TableNode *parent) {
//     for (const auto &expr : m_exprs) {
//         expr->constructTable(parent);
//     }
// }

// void IndexExpr::generateCode(io::JackWriter &writer) const {
//     m_idxExpr->generateCode(writer);
//     writer.writeBinOp('+');
//     writer.writePop("pointer", 1);
//     writer.writePush("that", 0);
//     Identifier::generateCode(writer);
// }

// void IndexExpr::constructTable(sym::TableNode *parent) {
//     Identifier::constructTable(parent);
//     m_idxExpr->constructTable(parent);
// }

}  // namespace ast

}  // namespace jcc
