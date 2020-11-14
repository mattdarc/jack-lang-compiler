#ifndef ast_Visitor_hpp
#define ast_Visitor_hpp

#include <type_traits>

#include "JackAST_fwd.hpp"

namespace jcc::ast {

template <bool IsConst>
class Visitor {
private:
  template <typename T>
  using visit_t = std::conditional_t<IsConst, const T, T>;

public:
  virtual ~Visitor() = default;

  // Terminals
  virtual void visit(visit_t<EmptyNode> &) = 0;
  virtual void visit(visit_t<True> &) = 0;
  virtual void visit(visit_t<False> &) = 0;
  virtual void visit(visit_t<This> &) = 0;
  virtual void visit(visit_t<IntConst> &) = 0;
  virtual void visit(visit_t<CharConst> &) = 0;
  virtual void visit(visit_t<Identifier> &) = 0;
  virtual void visit(visit_t<IndexExpr> &) = 0;
  virtual void visit(visit_t<StrConst> &) = 0;

  // Expressions
  virtual void visit(visit_t<BinaryOp> &) = 0;
  virtual void visit(visit_t<UnaryOp> &) = 0;
  virtual void visit(visit_t<MethodCall> &) = 0;
  virtual void visit(visit_t<FunctionCall> &) = 0;

  // Statements
  virtual void visit(visit_t<LetStmt> &) = 0;
  virtual void visit(visit_t<IfStmt> &) = 0;
  virtual void visit(visit_t<WhileStmt> &) = 0;
  virtual void visit(visit_t<ReturnStmt> &) = 0;

  // Declarations
  virtual void visit(visit_t<VarDecl> &) = 0;
  virtual void visit(visit_t<StaticDecl> &) = 0;
  virtual void visit(visit_t<MethodDecl> &) = 0;
  virtual void visit(visit_t<ConstructorDecl> &) = 0;
  virtual void visit(visit_t<ClassDecl> &) = 0;
  virtual void visit(visit_t<Block> &) = 0;

  // Modifiers
  virtual void visit(visit_t<RValueT> &) = 0;
};

using MutableVisitor = Visitor<false>;
using ImmutableVisitor = Visitor<true>;

}  // namespace jcc::ast

#endif /* ast_Visitor_hpp */
