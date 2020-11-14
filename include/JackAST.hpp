#ifndef JACK_AST_HPP
#define JACK_AST_HPP

#include <cassert>
#include <memory>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

#include "SymbolTable.hpp"
#include "Visitor.hpp"

// TODO(matt): Currently, it is extremely likely that we will need to generate
// code in two passes -- the first pass will create all the user-defined types.
// The second pass will do the code generation. Without this, types used across
// files will not exist

// TODO(matt): Need to create a subclass for just statements that implements
// codegen, Exprs need to be a different type. Change Base class to Node that
// Expr and Statement are derived from Separate out relational and arithmetic
// operators Array indexing is a subclass of expr
// TODO(matt): createa a plantuml diagram for the ast nodes

#define VISITABLE()                                           \
public:                                                       \
  void accept(MutableVisitor& v) override { v.visit(*this); } \
  void accept(ImmutableVisitor& v) const override { v.visit(*this); }

namespace llvm {
class Value;
class ConstantInt;
class LLVMContext;
}  // namespace llvm

namespace jcc {

namespace sym {

class Table;
class TableEntry;
enum class Kind;

}  // namespace sym

namespace utils {

llvm::ConstantInt* CreateConstant(llvm::LLVMContext& c, int i);

}  // namespace utils

namespace ast {
class Node;
class VarDecl;
class ClassDecl;
class FunctionDecl;
class Block;

using offset_t = int;
template <typename T>
using List = std::vector<std::unique_ptr<T>>;
using NodeList = List<Node>;
using ParamList = List<VarDecl>;
using FunctionList = List<FunctionDecl>;
using NodePtr = std::unique_ptr<jcc::ast::Node>;

// TODO(matt) refactor this into something else
struct VarDecList {
  void push_back(std::unique_ptr<VarDecl> expr, sym::Kind kind);
  ParamList fields;
  ParamList statics;
};

class Node {
public:
  virtual ~Node() = default;
  virtual void accept(MutableVisitor&) = 0;
  virtual void accept(ImmutableVisitor&) const = 0;
};

// Terminals
class Terminal : public Node {};

class EmptyNode : public Terminal {
  VISITABLE()
};

class IntConst : public Terminal {
  VISITABLE()
public:
  IntConst(int val) : m_int{val} {}
  int getInt() const { return m_int; }

private:
  int m_int;
};

class CharConst : public Terminal {
  VISITABLE()
public:
  CharConst(unsigned char val) : m_char{val} {}
  unsigned char getChar() const { return m_char; }

private:
  unsigned char m_char;
};

class NamedValue : public Terminal {
public:
  NamedValue(std::string id, FunctionDecl* parent)
      : m_name{std::move(id)}, m_parent{parent} {}
  const std::string& getName() const { return m_name; }
  const FunctionDecl* getParent() const { return m_parent; }
  const std::string& getType() const;

private:
  std::string m_name;
  FunctionDecl* m_parent;
};

class Identifier : public NamedValue {
  VISITABLE()
public:
  Identifier(std::string id, FunctionDecl* parent)
      : NamedValue{std::move(id), parent} {}
};

class IndexExpr : public NamedValue {
  VISITABLE()
public:
  IndexExpr(std::string arr, std::unique_ptr<Node> expr, FunctionDecl* parent)
      : NamedValue{std::move(arr), parent}, m_idxExpr{std::move(expr)} {}

  Node* getIndex() { return m_idxExpr.get(); }
  const Node* getIndex() const { return m_idxExpr.get(); }

private:
  std::unique_ptr<Node> m_idxExpr;
};

class This : public Terminal {
  VISITABLE()
};
class True : public Terminal {
  VISITABLE()
};
class False : public Terminal {
  VISITABLE()
};

struct Constant {
  static auto getThis() { return std::make_unique<This>(); }
  static auto getTrue() { return std::make_unique<True>(); }
  static auto getFalse() { return std::make_unique<False>(); }
};

class StrConst : public Terminal {
  VISITABLE()
public:
  StrConst(std::string str) : m_str{std::move(str)} {}
  const std::string& getString() const { return m_str; }

private:
  std::string m_str;
};

class VarDecl : public Node {
  VISITABLE()
public:
  VarDecl(std::string name, std::string type)
      : m_name{std::move(name)}, m_type{std::move(type)} {}

  const std::string& getName() const { return m_name; }
  const std::string& getType() const { return m_type; }
  void setType(const std::string& type) { m_type = type; }

private:
  std::string m_name;
  std::string m_type;
};

class Block : public Node {
  VISITABLE()
public:
  void addStmt(std::unique_ptr<Node> expr) {
    m_exprs.push_back(std::move(expr));
  }

  NodeList::iterator stmts_begin() { return m_exprs.begin(); }
  NodeList::iterator stmts_end() { return m_exprs.end(); }
  NodeList::const_iterator stmts_begin() const { return m_exprs.begin(); }
  NodeList::const_iterator stmts_end() const { return m_exprs.end(); }

private:
  NodeList m_exprs;
};

class ClassDecl : public Node {
  VISITABLE()
public:
  ClassDecl(std::string name)
      : m_name{std::move(name)},
        m_fields{},
        m_statics{},
        m_functions{},
        m_methods{},
        m_table{m_name} {}

  void addField(std::unique_ptr<VarDecl> field);
  void addStatic(std::unique_ptr<VarDecl> var);
  void addFunction(std::unique_ptr<FunctionDecl> fun);
  void addMethod(std::unique_ptr<FunctionDecl> mth);

  unsigned getFieldIdx(const std::string& name) const;

  const std::string& getName() const { return m_name; }
  std::string getStaticName(const std::string& varName) const;
  const sym::Table& getTable() const { return m_table; }
  sym::Table& getTable() { return m_table; }

  size_t numFields() const { return m_fields.size(); }
  ParamList::iterator fields_begin() { return m_fields.begin(); }
  ParamList::iterator fields_end() { return m_fields.end(); }

  size_t numStatics() const { return m_statics.size(); }
  ParamList::iterator statics_begin() { return m_statics.begin(); }
  ParamList::iterator statics_end() { return m_statics.end(); }

  size_t numFunctions() const { return m_functions.size(); }
  FunctionList::iterator fcns_begin() { return m_functions.begin(); }
  FunctionList::iterator fcns_end() { return m_functions.end(); }

  size_t numMethods() const { return m_methods.size(); }
  FunctionList::iterator mths_begin() { return m_methods.begin(); }
  FunctionList::iterator mths_end() { return m_methods.end(); }

  ParamList::const_iterator fields_begin() const { return m_fields.begin(); }
  ParamList::const_iterator fields_end() const { return m_fields.end(); }
  ParamList::const_iterator statics_begin() const { return m_statics.begin(); }
  ParamList::const_iterator statics_end() const { return m_statics.end(); }
  FunctionList::const_iterator fcns_begin() const {
    return m_functions.begin();
  }
  FunctionList::const_iterator fcns_end() const { return m_functions.end(); }
  FunctionList::const_iterator mths_begin() const { return m_methods.begin(); }
  FunctionList::const_iterator mths_end() const { return m_methods.end(); }

private:
  std::string m_name;
  ParamList m_fields;
  ParamList m_statics;
  FunctionList m_functions;
  FunctionList m_methods;
  sym::Table m_table;
};

class FunctionDecl : public Node {
public:
  FunctionDecl(std::string name, std::string returnType, ParamList params = {})
      : m_name{std::move(name)},
        m_return{std::move(returnType)},
        m_params{std::move(params)},
        m_table{m_name} {
    for (auto& param : m_params) { m_table.addValue(param.get()); }
  }

  virtual void setParent(const ClassDecl* cls) { m_parent = cls; }
  const ClassDecl* getParent() const { return m_parent; }

  const sym::Table& getTable() const { return m_table; }
  sym::Table& getTable() { return m_table; }

  const std::string& getName() const { return m_name; }
  const std::string& getReturnType() const { return m_return; }
  Block* getDefinition() { return m_body.get(); }
  const Block* getDefinition() const { return m_body.get(); }
  void addDefinition(std::unique_ptr<Block> body) { m_body = std::move(body); }

  size_t numParams() const { return m_params.size(); }
  ParamList::iterator prms_begin() { return m_params.begin(); }
  ParamList::iterator prms_end() { return m_params.end(); }
  ParamList::const_iterator prms_begin() const { return m_params.begin(); }
  ParamList::const_iterator prms_end() const { return m_params.end(); }

protected:
  std::string m_name;
  std::string m_return;
  ParamList m_params;
  std::unique_ptr<Block> m_body;
  const ClassDecl* m_parent;
  sym::Table m_table;
};

class StaticDecl : public FunctionDecl {
  VISITABLE()
public:
  template <typename... Ts>
  StaticDecl(Ts&&... args) : FunctionDecl(std::forward<Ts>(args)...) {}
};

class MethodDecl : public FunctionDecl {
  VISITABLE()
public:
  template <typename... Ts>
  MethodDecl(Ts&&... args) : FunctionDecl(std::forward<Ts>(args)...) {
    auto thisPtr = std::make_unique<VarDecl>("this", "");
    m_table.addValue(thisPtr.get());
    m_params.insert(m_params.begin(), std::move(thisPtr));
  }

  // TODO(matt) this feels like a hack. It may be better for the declarations
  // to know what their parent is on destruction since they will always be
  // defined with one
  void setParent(const ClassDecl* cls) override {
    FunctionDecl::setParent(cls);
    m_params[0]->setType(cls->getName());
  }
};

class ConstructorDecl : public FunctionDecl {
  VISITABLE()
public:
  template <typename... Ts>
  ConstructorDecl(Ts&&... args) : FunctionDecl(std::forward<Ts>(args)...) {}
};

// Operators
class BinaryOp : public Node {
  VISITABLE()
public:
  BinaryOp(char op, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
      : m_lhs{std::move(lhs)}, m_rhs{std::move(rhs)}, m_op{op} {}
  BinaryOp(char op, int lhs, int rhs)
      : BinaryOp(op, std::make_unique<IntConst>(lhs),
                 std::make_unique<IntConst>(rhs)) {}
  template <typename... Ts>
  BinaryOp(char op, int lhs, Ts&&... rhs)
      : BinaryOp(op, std::make_unique<IntConst>(lhs),
                 std::make_unique<BinaryOp>(std::forward<Ts>(rhs)...)) {}
  char getOp() const { return m_op; }
  const Node* getLHS() const { return m_lhs.get(); }
  Node* getLHS() { return m_lhs.get(); }
  const Node* getRHS() const { return m_rhs.get(); }
  Node* getRHS() { return m_rhs.get(); }

private:
  std::unique_ptr<Node> m_lhs;
  std::unique_ptr<Node> m_rhs;
  char m_op;
};

// class ArithmeticOp : public BinaryOp {
//     VISITABLE()
// public:
//     template <typename... Ts>
//     ArithmeticOp(Ts&&... args) : BinaryOp(std::forward<Ts>(args)...) {}
// };

// class RelOp : public BinaryOp {
//     VISITABLE()
// public:
//     template <typename... Ts>
//     RelOp(Ts&&... args) : BinaryOp(std::forward<Ts>(args)...) {}
// };

class UnaryOp : public Node {
  VISITABLE()
public:
  UnaryOp(char op, std::unique_ptr<Node> operand)
      : m_operand{std::move(operand)}, m_op{op} {}
  UnaryOp(char op, int operand)
      : m_operand{std::make_unique<IntConst>(operand)}, m_op{op} {}

  const Node* getOperand() const { return m_operand.get(); }
  Node* getOperand() { return m_operand.get(); }
  char getOp() const { return m_op; }

private:
  std::unique_ptr<Node> m_operand;
  char m_op;
};

class Call : public Node {
public:
  Call(std::string fcn, NodeList args)
      : m_name{std::move(fcn)}, m_args{std::move(args)} {}

  const std::string& getName() const { return m_name; }
  NodeList::iterator args_begin() { return m_args.begin(); }
  NodeList::iterator args_end() { return m_args.end(); }
  NodeList::const_iterator args_begin() const { return m_args.begin(); }
  NodeList::const_iterator args_end() const { return m_args.end(); }

private:
  std::string m_name;
  NodeList m_args;
};

class MethodCall : public Call {
  VISITABLE()
public:
  MethodCall(std::unique_ptr<NamedValue> callee, std::string fcn, NodeList args)
      : Call(std::move(fcn), std::move(args)), m_callee{std::move(callee)} {}

  NamedValue* getCallee() { return m_callee.get(); }
  const NamedValue* getCallee() const { return m_callee.get(); }

private:
  std::unique_ptr<NamedValue> m_callee;
};

class FunctionCall : public Call {
  VISITABLE()
public:
  FunctionCall(std::string cls, std::string fcn, NodeList args)
      : Call(std::move(fcn), std::move(args)), m_class{std::move(cls)} {}

  const std::string& getClassType() const { return m_class; }

private:
  std::string m_class;
};

class LetStmt : public Node {
  VISITABLE()
public:
  LetStmt(std::unique_ptr<NamedValue> assignee, std::unique_ptr<Node> expr)
      : m_assignee{std::move(assignee)}, m_expr{std::move(expr)} {}

  NamedValue* getAssignee() { return m_assignee.get(); }
  const NamedValue* getAssignee() const { return m_assignee.get(); }
  Node* getExpression() { return m_expr.get(); }
  const Node* getExpression() const { return m_expr.get(); }

private:
  std::unique_ptr<NamedValue> m_assignee;
  std::unique_ptr<Node> m_expr;
};

class IfStmt : public Node {
  VISITABLE()
public:
  IfStmt(std::unique_ptr<Node> condition, std::unique_ptr<Block> ifBranch,
         std::unique_ptr<Block> elseBranch)
      : m_condition{std::move(condition)},
        m_ifBranch{std::move(ifBranch)},
        m_elseBranch{std::move(elseBranch)} {}

  Node* getCond() { return m_condition.get(); }
  const Node* getCond() const { return m_condition.get(); }
  Block* getIfBlock() { return m_ifBranch.get(); }
  const Block* getIfBlock() const { return m_ifBranch.get(); }
  Block* getElseBlock() { return m_elseBranch.get(); }
  const Block* getElseBlock() const { return m_elseBranch.get(); }

private:
  std::unique_ptr<Node> m_condition;
  std::unique_ptr<Block> m_ifBranch;
  std::unique_ptr<Block> m_elseBranch;
};

class WhileStmt : public Node {
  VISITABLE()
public:
  WhileStmt(std::unique_ptr<Node> condition, std::unique_ptr<Block> body)
      : m_condition{std::move(condition)}, m_body{std::move(body)} {}

  Node* getCond() { return m_condition.get(); }
  const Node* getCond() const { return m_condition.get(); }
  Block* getBlock() { return m_body.get(); }
  const Block* getBlock() const { return m_body.get(); }

private:
  std::unique_ptr<Node> m_condition;
  std::unique_ptr<Block> m_body;
};

class ReturnStmt : public Node {
  VISITABLE()
public:
  ReturnStmt(std::unique_ptr<Node> expr) : m_expr{std::move(expr)} {}
  ReturnStmt(int i)
      : ReturnStmt{static_cast<std::unique_ptr<Node>>(
            std::make_unique<IntConst>(i))} {}

  Node* getExpr() { return m_expr.get(); }
  const Node* getExpr() const { return m_expr.get(); }

private:
  std::unique_ptr<Node> m_expr;
};

class RValueT : public Terminal {
  VISITABLE()
public:
  RValueT(std::unique_ptr<Terminal> V) : m_wrapped{std::move(V)} {
    assert(m_wrapped);
  }

  Terminal* getWrapped() { return m_wrapped.get(); }
  const Terminal* getWrapped() const { return m_wrapped.get(); }

private:
  std::unique_ptr<Terminal> m_wrapped;
};

inline auto RValue(std::unique_ptr<Terminal> V) {
  return std::make_unique<RValueT>(std::move(V));
}

}  // namespace ast

}  // namespace jcc

#endif  // JACK_AST_HPP
