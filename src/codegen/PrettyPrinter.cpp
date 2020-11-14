#include "PrettyPrinter.hpp"

#include "JackAST.hpp"

namespace jcc::ast {

template <typename ForwardIt>
void PrettyPrinter::printExprList(ForwardIt beg, ForwardIt end) {
  for (; beg != end; ++beg) { (*beg)->accept(*this); };
}

std::string PrettyPrinter::print(const Node &cls) {
  PrettyPrinter p;
  cls.accept(p);
  return p.m_ast;
}

std::string PrettyPrinter::pad() const {
  return std::string(m_offset * 2, ' ');
}

void PrettyPrinter::visit(const IntConst &integer) {
  Pad p(this);
  m_ast += pad() + "IntConst: " + std::to_string(integer.getInt()) + '\n';
}

void PrettyPrinter::visit(const CharConst &c) {
  Pad p(this);
  m_ast += pad() + "CharConst: " + std::to_string(c.getChar()) + '\n';
}

void PrettyPrinter::visit(const Identifier &identifier) {
  Pad p(this);
  if (!identifier.getName().empty()) {
    m_ast += pad() + "Identifier: " + identifier.getName() + '\n';
  }
}

void PrettyPrinter::visit(const StrConst &str) {
  Pad p(this);
  m_ast += pad() + "StrConst: " + str.getString() + '\n';
}

void PrettyPrinter::visit(const BinaryOp &binop) {
  Pad p(this);
  m_ast += pad() + binop.getOp() + '\n';
  binop.getLHS()->accept(*this);
  binop.getRHS()->accept(*this);
}

void PrettyPrinter::visit(const UnaryOp &unop) {
  Pad p(this);
  m_ast += pad() + "UnaryExpr: " + unop.getOp() + '\n';
  unop.getOperand()->accept(*this);
}

void PrettyPrinter::visit(const FunctionCall &call) {
  Pad p(this);
  m_ast += pad() + "FunctionCall: " + call.getClassType() + '.' +
           call.getName() + '\n' + pad() + "Args:\n";
  for (auto arg = call.args_begin(); arg != call.args_end(); ++arg) {
    (*arg)->accept(*this);
  }
}

void PrettyPrinter::visit(const MethodCall &call) {
  Pad p(this);
  m_ast += pad() + "FunctionCall: " + call.getName() + '\n';
  if (call.getCallee()) { call.getCallee()->accept(*this); }

  m_ast += pad() + "Args:\n";
  for (auto arg = call.args_begin(); arg != call.args_end(); ++arg) {
    (*arg)->accept(*this);
  }
}

void PrettyPrinter::visit(const LetStmt &let) {
  Pad p(this);
  m_ast += pad() + "LetStmt: \n";
  let.getAssignee()->accept(*this);
  let.getExpression()->accept(*this);
}

void PrettyPrinter::visit(const VarDecl &var) {
  Pad p(this);
  m_ast += pad() + "VarDecl: " + var.getType() + ' ' + var.getName() + '\n';
}

void PrettyPrinter::visit(const IfStmt &expr) {
  Pad p(this);
  m_ast += pad() + "IfStmt: \n";
  expr.getCond()->accept(*this);
  expr.getIfBlock()->accept(*this);
  if (expr.getElseBlock()) { expr.getElseBlock()->accept(*this); }
}

void PrettyPrinter::visit(const WhileStmt &expr) {
  Pad p(this);
  m_ast += pad() + "WhileStmt: \n";
  expr.getCond()->accept(*this);
  m_ast += pad() + "{\n";
  expr.getBlock()->accept(*this);
  m_ast += pad() + "}\n";
}

void PrettyPrinter::visit(const ReturnStmt &expr) {
  Pad p(this);
  m_ast += pad() + "ReturnStmt: \n";
  expr.getExpr()->accept(*this);
}

template <typename FunctionType>
void PrettyPrinter::visitFunctionDecl(FunctionType &f) {
  m_ast += f.getReturnType() + ' ' + f.getName() + '\n' + pad() + "Params: \n";
  printExprList(f.prms_begin(), f.prms_end());
  f.getDefinition()->accept(*this);
}

void PrettyPrinter::visit(const ConstructorDecl &f) {
  Pad p(this);
  m_ast += pad() + "ConstructorDecl: ";
  visitFunctionDecl(f);
}

void PrettyPrinter::visit(const MethodDecl &f) {
  Pad p(this);
  m_ast += pad() + "MethodDecl: ";
  visitFunctionDecl(f);
}

void PrettyPrinter::visit(const StaticDecl &f) {
  Pad p(this);
  m_ast += pad() + "StaticDecl: ";
  visitFunctionDecl(f);
}

void PrettyPrinter::visit(const Block &b) {
  Pad p(this);
  m_ast += pad() + "Block: {\n";
  printExprList(b.stmts_begin(), b.stmts_end());
  m_ast += pad() + "}\n";
}

void PrettyPrinter::visit(const ClassDecl &cls) {
  Pad p(this);
  m_ast += pad() + "Class: " + cls.getName() + '\n' + pad() + "Fields: \n";
  printExprList(cls.fields_begin(), cls.fields_end());
  m_ast += pad() + "Statics: \n";
  printExprList(cls.statics_begin(), cls.statics_end());
  m_ast += pad() + "Functions: \n";
  printExprList(cls.fcns_begin(), cls.fcns_end());
  m_ast += pad() + "Methods: \n";
  printExprList(cls.mths_begin(), cls.mths_end());
}

void PrettyPrinter::visit(const IndexExpr &expr) {
  Pad p(this);
  m_ast += pad() + "IndexExpr:" + expr.getName() + '\n' + pad() + "[\n";
  expr.getIndex()->accept(*this);
  m_ast += pad() + "]\n";
}

void PrettyPrinter::visit(const RValueT &expr) {
  Pad p(this);
  m_ast += pad() + "RValue (\n";
  expr.getWrapped()->accept(*this);
  m_ast += pad() + ")\n";
}

}  // namespace jcc::ast
