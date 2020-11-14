#ifndef test_ASTBuilder_hpp
#define test_ASTBuilder_hpp

#include "JackAST.hpp"

namespace jcc::ast {
class Builder {
public:
  Builder &setFunction(FunctionDecl *f) {
    m_function = f;
    return *this;
  }

  Builder &setClass(ClassDecl *cls) {
    m_cls = cls;
    return *this;
  }

  auto CreateIdentifier(const std::string &ident) {
    return std::make_unique<Identifier>(ident, m_function);
  }

  auto CreateIndexInto(const std::string &arr, std::unique_ptr<Node> idx) {
    return std::make_unique<IndexExpr>(arr, std::move(idx), m_function);
  }

  auto CreateIndexInto(const std::string &arr, int idx) {
    return CreateIndexInto(arr, std::make_unique<IntConst>(idx));
  }

  auto CreateReturn(int i) {
    return std::make_unique<ReturnStmt>(std::make_unique<IntConst>(i));
  }

  auto CreateReturn(const std::string &s) {
    return std::make_unique<ReturnStmt>(RValue(CreateIdentifier(s)));
  }

  auto CreateReturn(std::unique_ptr<Node> ret) {
    return std::make_unique<ReturnStmt>(std::move(ret));
  }

  auto CreateVarDecl(const std::string &name, const std::string &type) {
    auto decl = std::make_unique<VarDecl>(name, type);
    m_function->getTable().addValue(decl.get());
    return decl;
  }

  auto CreateParameter(const std::string &name, const std::string &type) {
    return std::make_unique<VarDecl>(name, type);
  }

  auto CreateMemberVar(const std::string &name, const std::string &type) {
    m_cls->addField(std::make_unique<VarDecl>(name, type));
  }

  auto CreateStaticVar(const std::string &name, const std::string &type) {
    m_cls->addStatic(std::make_unique<VarDecl>(name, type));
  }

  auto CreateLet(std::unique_ptr<NamedValue> lhs, std::unique_ptr<Node> rhs) {
    return std::make_unique<LetStmt>(std::move(lhs), std::move(rhs));
  }

  auto CreateLet(const std::string &lhs, const std::string &rhs) {
    return CreateLet(CreateIdentifier(lhs), RValue(CreateIdentifier(rhs)));
  }

  auto CreateLet(const std::string &lhs, int rhs) {
    return CreateLet(CreateIdentifier(lhs), std::make_unique<IntConst>(rhs));
  }

  auto CreateLet(std::unique_ptr<NamedValue> lhs, int rhs) {
    return CreateLet(std::move(lhs), std::make_unique<IntConst>(rhs));
  }

  auto CreateLet(const std::string &lhs, std::unique_ptr<Node> rhs) {
    return CreateLet(CreateIdentifier(lhs), std::move(rhs));
  }

  auto CreateArithmetic(char op, int i, int j) {
    return std::make_unique<BinaryOp>(op, std::make_unique<IntConst>(i),
                                      std::make_unique<IntConst>(j));
  }

  auto CreateArithmetic(char op, std::unique_ptr<Node> i,
                        std::unique_ptr<Node> j) {
    return std::make_unique<BinaryOp>(op, std::move(i), std::move(j));
  }

  auto CreateIf(char op, const std::string &s, int i,
                std::unique_ptr<Block> ifBlock,
                std::unique_ptr<Block> elseBlock = nullptr) {
    return std::make_unique<IfStmt>(
        std::make_unique<BinaryOp>(op, RValue(CreateIdentifier(s)),
                                   std::make_unique<IntConst>(i)),
        std::move(ifBlock), std::move(elseBlock));
  }

  auto CreateWhile(char op, const std::string &s, int i,
                   std::unique_ptr<Block> thenBlock) {
    return std::make_unique<WhileStmt>(
        std::make_unique<BinaryOp>(op, RValue(CreateIdentifier(s)),
                                   std::make_unique<IntConst>(i)),
        std::move(thenBlock));
  }

  auto CreateMethodCall(const std::string &var, const std::string &function,
                        NodeList args = {}) {
    return std::make_unique<MethodCall>(CreateIdentifier(var), function,
                                        std::move(args));
  }

  auto CreateMethodCall(const std::string &function, NodeList args = {}) {
    return std::make_unique<MethodCall>(nullptr, function, std::move(args));
  }

  auto CreateFunctionCall(const std::string &cls, const std::string &function,
                          NodeList args = {}) {
    return std::make_unique<FunctionCall>(cls, function, std::move(args));
  }

  template <typename... Ts>
  auto CreateStaticDecl(Ts &&... args) {
    return CreateFunctionDeclImpl<StaticDecl>(std::forward<Ts>(args)...);
  }

  template <typename... Ts>
  auto CreateConstructorDecl(Ts &&... args) {
    return CreateFunctionDeclImpl<ConstructorDecl>("new", m_cls->getName(),
                                                   std::forward<Ts>(args)...);
  }

  template <typename... Ts>
  auto CreateMethodDecl(Ts &&... args) {
    auto fcn = std::make_unique<MethodDecl>(std::forward<Ts>(args)...);
    auto rawF = fcn.get();
    m_cls->addMethod(std::move(fcn));
    rawF->addDefinition(std::make_unique<Block>());
    m_function = rawF;
    return rawF;
  }

private:
  ClassDecl *m_cls = nullptr;
  FunctionDecl *m_function = nullptr;

  template <typename FunctionType, typename... Ts>
  auto CreateFunctionDeclImpl(Ts &&... args) {
    auto fcn = std::make_unique<FunctionType>(std::forward<Ts>(args)...);
    auto rawF = fcn.get();
    m_cls->addFunction(std::move(fcn));
    rawF->addDefinition(std::make_unique<Block>());
    m_function = rawF;
    return rawF;
  }
};

}  // namespace jcc::ast

#endif /* test_ASTBuilder_hpp */
