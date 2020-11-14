#include "JackJIT.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/OrcABISupport.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"  // Legacy passes

namespace exec {
using namespace llvm;
using namespace orc;

JIT::JIT(JITTargetMachineBuilder JTMB, DataLayout aDL)
    : ES(),
      ObjectLayer(ES,
                  []() { return std::make_unique<SectionMemoryManager>(); }),
      CompileLayer(ES, ObjectLayer,
                   std::make_unique<ConcurrentIRCompiler>(JTMB)),
      OptimizeLayer(
          ES, CompileLayer,
          [](ThreadSafeModule M, const MaterializationResponsibility &) {
            auto FPM = std::make_unique<legacy::FunctionPassManager>(
                M.getModuleUnlocked());

            // Add some optimizations.
            FPM->add(createPromoteMemoryToRegisterPass());
            FPM->doInitialization();

            for (auto &F : *M.getModuleUnlocked()) FPM->run(F);

            return M;
          }),
      LazyCTM(
          cantFail(LocalLazyCallThroughManager::Create<OrcX86_64_SysV>(ES, 0))),
      CODLayer(
          ES, OptimizeLayer, *LazyCTM,
          orc::createLocalIndirectStubsManagerBuilder(JTMB.getTargetTriple())),
      DL(std::move(aDL)),
      Mangle(ES, DL),
      Ctx(std::make_unique<LLVMContext>()),
      MainJD(ES.createJITDylib("<main>")) {
  MainJD.addGenerator(
      cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
          DL.getGlobalPrefix())));
}

llvm::JITSymbol JIT::findSymbol(StringRef symbol) {
  auto sym = ES.lookup({&MainJD}, Mangle(symbol));
  if (!sym) {
    llvm::errs() << "Missing Main.main\n\n";
    dumpEngine();
    exit(1);
  }

  return sym.get();
}

int JIT::run(llvm::JITSymbol &symbol) {
  return llvm::jitTargetAddressToFunction<int (*)()>(
      cantFail(symbol.getAddress()))();
}

std::unique_ptr<JIT> JIT::Create() {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  auto JTMB = cantFail(JITTargetMachineBuilder::detectHost());
  auto aDL = cantFail(JTMB.getDefaultDataLayoutForTarget());
  return std::unique_ptr<JIT>(new JIT(std::move(JTMB), std::move(aDL)));
}

void JIT::addModule(std::unique_ptr<Module> M) {
  M->setDataLayout(DL);
  cantFail(CODLayer.add(MainJD, ThreadSafeModule(std::move(M), Ctx)));
}

void JIT::dumpEngine() const { MainJD.dump(llvm::errs()); }

}  // namespace exec
