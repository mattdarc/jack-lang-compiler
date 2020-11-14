#ifndef ast_PrettyPrinter_hpp
#define ast_PrettyPrinter_hpp

#include <string>

#include "JackAST_fwd.hpp"
#include "Visitor.hpp"

namespace jcc::ast {

class Node;

class PrettyPrinter : public ImmutableVisitor {
public:
  PrettyPrinter() : m_offset{0}, m_ast{} {}
  static std::string print(const Node &);

  void visit(const EmptyNode &) override {}
  void visit(const IntConst &) override;
  void visit(const CharConst &) override;
  void visit(const Identifier &) override;
  void visit(const StrConst &) override;
  void visit(const IndexExpr &) override;
  void visit(const True &) override { m_ast += "true"; }
  void visit(const False &) override { m_ast += "false"; }
  void visit(const This &) override { m_ast += "this"; }

  void visit(const BinaryOp &) override;
  void visit(const UnaryOp &) override;

  void visit(const MethodCall &) override;
  void visit(const FunctionCall &) override;

  void visit(const LetStmt &) override;
  void visit(const IfStmt &) override;
  void visit(const WhileStmt &) override;
  void visit(const ReturnStmt &) override;

  void visit(const VarDecl &) override;
  void visit(const MethodDecl &) override;
  void visit(const StaticDecl &) override;
  void visit(const ConstructorDecl &) override;
  void visit(const ClassDecl &) override;
  void visit(const Block &) override;

  void visit(const RValueT &) override;

private:
  unsigned m_offset;
  std::string m_ast;

  std::string pad() const;

  template <typename ForwardIt>
  void printExprList(ForwardIt, ForwardIt);

  template <typename FunctionType>
  void visitFunctionDecl(FunctionType &f);

  struct Pad {
    explicit Pad(PrettyPrinter *p) : m_p{p} { ++m_p->m_offset; }
    ~Pad() { --m_p->m_offset; }

  private:
    PrettyPrinter *m_p;
  };
};

}  // namespace jcc::ast

#endif /* ast_PrettyPrinter_hpp */
