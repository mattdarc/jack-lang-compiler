#ifndef Runtime_hpp
#define Runtime_hpp

#include <memory>

#include "JackAST.hpp"
#include "JackJIT.hpp"
#include "LLVMGenerator.hpp"
#include "PrettyPrinter.hpp"
#include "Visitor.hpp"

namespace jcc {
using NodePtr = std::unique_ptr<jcc::ast::Node>;
using ASTList = std::vector<NodePtr>;

// Facade for the code generation and JIT of a Jack program. TODO This should not be a
// singleton, but should own compilers and interpreters
class Runtime {
public:
  static void init(std::istream &is, std::ostream &os);
  static Runtime &instance();

  std::istream &istream() { return m_is; }
  std::ostream &ostream() { return m_os; }

  llvm::Module &module();
  llvm::orc::JITDylib &engine();

  Runtime(const Runtime &) = delete;
  Runtime &operator=(const Runtime &) = delete;
  Runtime(Runtime &&) = delete;
  Runtime &operator=(Runtime &&) = delete;

  const llvm::LLVMContext &getContext() const { return *m_context; }

  const ast::Node *getAST(size_t idx = 0) const { return m_ast[idx].get(); }
  ast::Node *getAST(size_t idx = 0) { return m_ast[idx].get(); }

  void reset();
  void reset(std::unique_ptr<ast::Node>);
  void addAST(std::unique_ptr<ast::Node>);
  int run();
  llvm::Value *codegen();
  void clear() {
    m_gen.reset();
    m_jit.reset();
  }

private:
  // This needs to be owned by the runtime (and first) so we can delete the
  // modules before the context is destroyed
  std::unique_ptr<llvm::LLVMContext> m_context;

  ASTList m_ast;
  std::unique_ptr<ast::LLVMGenerator> m_gen;
  std::unique_ptr<exec::JIT> m_jit;

  // Stream I/O
  std::istream &m_is;
  std::ostream &m_os;

  Runtime(std::istream &is, std::ostream &os)
      : m_context{std::make_unique<llvm::LLVMContext>()}, m_is{is}, m_os{os} {}
  Runtime() : Runtime(std::cin, std::cout) {}

  // Register the builtin functions for manipulating arrays, strings, output,
  // and the AST
  void registerBuiltins();
};

}  // namespace jcc

#endif /* Runtime_hpp */
