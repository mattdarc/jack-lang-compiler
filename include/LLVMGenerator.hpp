#ifndef _jcc_LLVMGenerator_hpp_
#define _jcc_LLVMGenerator_hpp_

#include "JackAST.hpp"
#include "Visitor.hpp"

#define LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING 1
#include <iostream>

#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

namespace jcc {

namespace constants {
constexpr int bit_width = 32;
constexpr int char_width = 8;
constexpr bool is_signed = true;
}  // namespace constants

namespace ast {

class LLVMGenerator;

struct UnresolvedSymbol {
  using LazyCall = std::function<llvm::Function *(LLVMGenerator &)>;
  UnresolvedSymbol(LazyCall call, llvm::Function *toReplace)
      : ReplaceWith{call}, ToReplace{toReplace} {}
  LazyCall ReplaceWith;
  llvm::Function *ToReplace;
};

class Node;

class LLVMGenerator : public MutableVisitor {
public:
  llvm::Module *newModule(const std::string &name = "themodule") {
    m_module = std::make_unique<llvm::Module>(name, m_context);
    return m_module.get();
  }

  static std::unique_ptr<LLVMGenerator> Create(llvm::LLVMContext &context) {
    auto g = std::unique_ptr<LLVMGenerator>(new LLVMGenerator(context));
    g->newModule();
    return g;
  }

  llvm::Value *codegen(Node &node);

  // Utility to print out the current module
  void dumpModule() const;

  // Move the ownership of the module from the generator
  std::unique_ptr<llvm::Module> moveModule() { return std::move(m_module); }

  // Return a reference to the LLVM IR Builder
  llvm::IRBuilder<> &builder() { return m_builder; }

  // Lookup the llvm::Type given the type name
  llvm::Type *getTypeByName(const std::string &name);

  // TODO(matt): These should be in a test API
  llvm::Module *module() { return m_module.get(); }
  const llvm::Module *module() const { return m_module.get(); }
  llvm::LLVMContext &context() { return m_builder.getContext(); }
  llvm::LLVMContext &context() const { return m_builder.getContext(); }

  llvm::Function *getLLVMFunction(const std::string &cls,
                                  const std::string &fname) const;

  ClassDecl *getAST() { return m_class; }

  std::string mangleFunction(const FunctionDecl &f) const;
  std::string mangleStatic(const std::string &varName) const;

  void visit(IntConst &) override;
  void visit(CharConst &) override;
  void visit(Identifier &) override;
  void visit(StrConst &) override;
  void visit(IndexExpr &) override;
  void visit(True &) override;
  void visit(False &) override;
  void visit(This &) override;
  void visit(EmptyNode &) override;

  void visit(BinaryOp &) override;
  void visit(UnaryOp &) override;

  void visit(MethodCall &) override;
  void visit(FunctionCall &) override;

  void visit(VarDecl &) override;
  void visit(StaticDecl &) override;
  void visit(MethodDecl &) override;
  void visit(ConstructorDecl &) override;
  void visit(ClassDecl &) override;

  void visit(LetStmt &) override;
  void visit(IfStmt &) override;
  void visit(WhileStmt &) override;
  void visit(ReturnStmt &) override;
  void visit(Block &) override;

  void visit(RValueT &) override;

private:
  llvm::LLVMContext &m_context;
  llvm::IRBuilder<> m_builder;
  std::vector<UnresolvedSymbol> m_unresolved;
  std::unique_ptr<llvm::Module> m_module;
  llvm::Value *m_last;
  llvm::Type *m_ExpType;  // The expected type based on the previous
                          // expression. This is used to cast the result of
                          // expressions to the expected type
  ClassDecl *m_class;     // The current class we are generating code for

  using ValueTable = std::unordered_map<std::string, llvm::Value *>;
  ValueTable m_ScopedValueTable;

  [[noreturn]] void InternalError(llvm::Function *f);
  llvm::Value *findIdentifier(const std::string &);

  // Utility to codegen subexpressions and retrieve the value
  llvm::Value *codegenChild(Node &n) {
    n.accept(*this);
    return m_last;
  }

  auto UnresolvedFunction(llvm::Type *RetTy,
                          const std::vector<llvm::Value *> &args)
      -> llvm::Function *;
  void allocateArguments(llvm::Function *funcI, FunctionDecl &decl);

  template <typename FunctionType>
  llvm::Function *visitFunction(FunctionType &);

  LLVMGenerator(llvm::LLVMContext &ctx)
      : m_context{ctx},
        m_builder{m_context},
        m_module{nullptr},
        m_last{nullptr} {}
};

}  // namespace ast

}  // namespace jcc

#endif  // _jcc_LLVMGenerator_hpp_
