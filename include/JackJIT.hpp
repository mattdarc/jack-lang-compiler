#ifndef _exec_JIT_hpp_
#define _exec_JIT_hpp_

#include <memory>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"

namespace jcc {
class Runtime;
}

namespace exec {
using namespace llvm;
using namespace orc;

class JIT {
public:
  llvm::JITSymbol findSymbol(StringRef symbol);
  // void removeModule(VModuleKey K) { cantFail(CODLayer.removeModule(K)); }

  void addModule(std::unique_ptr<Module> M);

  static std::unique_ptr<JIT> Create();
  int run(llvm::JITSymbol &symbol);

  void dumpEngine() const;
  void dumpModule() const;

private:
  friend class jcc::Runtime;

  JIT(JITTargetMachineBuilder JTMB, DataLayout DL);

  ExecutionSession ES;
  RTDyldObjectLinkingLayer ObjectLayer;
  IRCompileLayer CompileLayer;
  IRTransformLayer OptimizeLayer;

  std::unique_ptr<LazyCallThroughManager> LazyCTM;
  CompileOnDemandLayer CODLayer;

  DataLayout DL;
  MangleAndInterner Mangle;
  ThreadSafeContext Ctx;
  JITDylib &MainJD;
};

// TODO(matt): create mock class

}  // namespace exec

#endif /* _mock_JIT_hpp_ */
