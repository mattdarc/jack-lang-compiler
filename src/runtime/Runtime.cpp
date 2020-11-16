#include "Runtime.hpp"

#include <sstream>

#include "Builtins.hpp"
#include "JackAST.hpp"
#include "PrettyPrinter.hpp"

namespace jcc::builtin {

struct Array : BuiltinType<int> {
  using Self::Self;
  constexpr static auto class_name = "Array";
};

struct ASTNode : BuiltinType<jcc::ast::Node> {
  using Self::Self;
  constexpr static auto class_name = "ASTNode";
};

struct String : BuiltinType<std::string> {
  using Self::Self;
  constexpr static auto class_name = "String";
};

struct Output : BuiltinType<void> {
  using Self::Self;
  constexpr static auto class_name = "Output";
};

struct Input : BuiltinType<void> {
  using Self::Self;
  constexpr static auto class_name = "Keyboard";
};

struct TestAPI : BuiltinType<void> {
  using Self::Self;
  constexpr static auto class_name = "Test";
};

}  // namespace jcc::builtin

namespace {
using namespace jcc::builtin;
using namespace jcc;

// FIXME: This shouldn't really exist, we should have a way to marshall from
// llvm
class TestAPIClass : public BuiltinRegistrar<TestAPI> {
public:
  using Self::Self;

  template <typename T>
  static void toVar(T) {
    // inspect_impl::addr = from.impl;
  }

  template <typename T>
  static void toValueType(T) {
    // inspect_impl::data = from;
    // inspect_impl::addr = &inspect_impl::data;
  }
};

void registerTestAPI(llvm::Module *module) {
  TestAPIClass TestAPICls(module);
  TestAPICls.addFunction(TestAPIClass::toVar<String>, "inspectStr");
  TestAPICls.addFunction(TestAPIClass::toValueType<int>, "inspectInt");
  TestAPICls.addFunction(TestAPIClass::toValueType<char>, "inspectChar");
  TestAPICls.addFunction(TestAPIClass::toValueType<bool>, "inspectBool");
}

void registerArray(llvm::Module *module) {
  class ArrayClass : public BuiltinRegistrar<Array> {
  public:
    using Self::Self;

    static void dispose(Array arr) { delete[] arr.impl; }

    static Array create(int size) { return Array{new int[size]}; }

  } ACls(module);

  ACls.addFunction(ArrayClass::create, "new");
  ACls.addFunction(ArrayClass::dispose, "dispose");
}

void registerAST(llvm::Module *module) {
  class ASTNodeClass : public BuiltinRegistrar<ASTNode> {
  public:
    using Self::Self;

    static void print(ASTNode n) {
      Runtime::instance().ostream() << ast::PrettyPrinter::print(*n.impl);
    }

    static ASTNode get() {
      // TODO(matt) this needs to be shared ownership
      return ASTNode{Runtime::instance().getAST()};
    }
  } NodeCls(module);

  NodeCls.addFunction(ASTNodeClass::print, "print");
  NodeCls.addFunction(ASTNodeClass::get, "getRoot");
}

void registerString(llvm::Module *module) {
  class StringClass : public BuiltinRegistrar<String> {
  public:
    using Self::Self;

    static String create(int size) {
      auto str = new std::string;
      str->reserve(size);
      return String{str};
    }

    static void dispose(String str) { delete str.impl; }

    static int length(String str) { return str.impl->size(); }

    static char charAt(String str, int idx) {
      // TODO(matt) error check bounds builtins
      return (*str.impl)[idx];
    }

    static void setCharAt(String str, int idx, char c) {
      // TODO(matt) error check builtins
      (*str.impl)[idx] = c;
    }

    static String appendChar(String str, char c) {
      str.impl->push_back(c);
      return str;
    }

    static void eraseLastChar(String str) {
      str.impl->erase(str.impl->size() - 1, str.impl->size());
    }

    // TODO(matt) remove this with overloading
    static String ptrToString(char *c) { return String{new std::string(c)}; }

  } StrCls(module);

  StrCls.addFunction(StringClass::create, "new");
  StrCls.addFunction(StringClass::dispose, "dispose");
  StrCls.addFunction(StringClass::length, "length");
  StrCls.addFunction(StringClass::charAt, "charAt");
  StrCls.addFunction(StringClass::setCharAt, "setCharAt");
  StrCls.addFunction(StringClass::appendChar, "appendChar");
  StrCls.addFunction(StringClass::eraseLastChar, "eraseLastChar");
  StrCls.addFunction(StringClass::ptrToString, "ptrtostr");
}

void registerOutput(llvm::Module *module) {
  class OutputClass : public BuiltinRegistrar<Output> {
  public:
    using Self::Self;

    static void printChar(char c) { Runtime::instance().ostream() << c; }

    static void printString(String str) {
      Runtime::instance().ostream() << *str.impl;
    }

    static void printInt(int i) { Runtime::instance().ostream() << i; }

    static void println() { Runtime::instance().ostream() << '\n'; }

  } OutCls(module);

  OutCls.addFunction(OutputClass::printChar, "printChar");
  OutCls.addFunction(OutputClass::printString, "printString");
  OutCls.addFunction(OutputClass::printInt, "printInt");
  OutCls.addFunction(OutputClass::println, "println");
}

void registerInput(llvm::Module *module) {
  class InputClass : public BuiltinRegistrar<Input> {
  public:
    using Self::Self;

    static String readLine(String msg) {
      Runtime::instance().ostream() << *msg.impl;

      auto str = new std::string;

      *str = std::accumulate(
          std::istream_iterator<std::string>(Runtime::instance().istream()),
          std::istream_iterator<std::string>(), std::string{},
          [](std::string s, const std::string &inp) {
            return std::move(s) + inp + " ";
          });
      str->erase(str->size() - 1);
      return String{str};
    }

    static int readInt(String msg) {
      Runtime::instance().ostream() << *msg.impl;

      int v;
      Runtime::instance().istream() >> v;
      return v;
    }
  } InpCls(module);

  InpCls.addFunction(InputClass::readLine, "readLine");
  InpCls.addFunction(InputClass::readInt, "readInt");
}

static std::unique_ptr<Runtime> RTInstance;

}  // namespace

namespace jcc {

void Runtime::reset(std::unique_ptr<ast::Node> ast) {
  reset();
  m_ast.push_back(std::move(ast));
}

void Runtime::reset() {
  m_ast.clear();
  m_gen.reset();
  m_jit.reset();
  m_context.reset(new llvm::LLVMContext);
  m_gen = ast::LLVMGenerator::Create(*m_context);
  m_jit = exec::JIT::Create();
  registerBuiltins();  // TODO: move this to separate module so we don't need
                       // to keep generating. Once this is done we should be
                       // able to parallelize it
}

void Runtime::addAST(std::unique_ptr<ast::Node> ast) {
  m_ast.push_back(std::move(ast));
}

void Runtime::registerBuiltins() {
  registerTestAPI(m_gen->module());
  registerArray(m_gen->module());
  registerString(m_gen->module());
  registerOutput(m_gen->module());
  registerAST(m_gen->module());
  registerInput(m_gen->module());
}

Runtime &Runtime::instance() { return *RTInstance; }
void Runtime::init(std::istream &is, std::ostream &os) {
  RTInstance.reset(new Runtime(is, os));
}

llvm::Module &Runtime::module() {
  llvm::Module *mod = m_gen->module();
  assert(mod && "Uninitialized Jack Runtime!");
  return *mod;
}

llvm::orc::JITDylib &Runtime::engine() { return m_jit->MainJD; }

llvm::Value *Runtime::codegen() {
  llvm::Value *ret = nullptr;
  for (auto &ast : m_ast) { ret = m_gen->codegen(*ast); }
  return ret;
}

int Runtime::run() {
  m_jit->addModule(m_gen->moveModule());
  auto sym = m_jit->findSymbol(builtin::generateName("Main", "main"));
  return m_jit->run(sym);
}

}  // namespace jcc
