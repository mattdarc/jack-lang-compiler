#ifndef jcc_Builtins_hpp
#define jcc_Builtins_hpp

#include <string>
#include <string_view>
#include <type_traits>

#include "NameMangling.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace jcc::builtin {

template <typename>
struct Traits;

template <typename...>
struct to_llvm;

namespace detail {
using invalid_t = std::nullptr_t;

template <typename = invalid_t>
struct to_llvm_helper;

}  // namespace detail

template <typename... Ts>
using TypeClarifier =
    std::conditional_t<(sizeof...(Ts) == 1), llvm::Type *,
                       std::array<llvm::Type *, sizeof...(Ts)>>;

template <typename ImplType>
struct BuiltinType {
  using Self = BuiltinType<ImplType>;
  using impl_type = ImplType;
  constexpr static bool is_instantiable = !std::is_same_v<ImplType, void>;

  BuiltinType(impl_type *aImpl) : impl{aImpl} {}

  impl_type *impl;
};

template <typename Traits>
class BuiltinRegistrar {
public:
  BuiltinRegistrar(llvm::Module *aM) : M{aM} {}

  template <typename U>
  BuiltinRegistrar(const BuiltinRegistrar<U> &) = delete;
  template <typename U>
  BuiltinRegistrar &operator=(const BuiltinRegistrar<U> &) = delete;

  using impl_type = typename Traits::impl_type;
  constexpr static auto class_name = Traits::class_name;

  template <typename Ret, typename... Ts>
  llvm::Function *addFunction(Ret (*FuncAddr)(Ts...),
                              const std::string &FuncName) {
    using namespace llvm;
    using ::jcc::builtin::to_llvm;

    auto &Ctx = M->getContext();
    IRBuilder<> Builder{Ctx};

    auto RetTy = to_llvm<Ret>::doit(M);
    auto ParamTy = to_llvm<Ts...>::doit(M);
    auto FuncTy = FunctionType::get(RetTy, ParamTy, false);
    auto Func =
        Function::Create(FuncTy, Function::ExternalLinkage,
                         builtin::generateName(class_name, FuncName), M);
    auto BB = BasicBlock::Create(Ctx, "entry", Func);
    Builder.SetInsertPoint(BB);
    auto Addr = ConstantInt::get(Type::getInt64Ty(Ctx),
                                 reinterpret_cast<intptr_t>(FuncAddr));
    auto Callable = Builder.CreateIntToPtr(Addr, FuncTy->getPointerTo());

    std::vector<llvm::Value *> Args;
    Args.reserve(FuncTy->getNumParams());
    std::transform(Func->arg_begin(), Func->arg_end(), std::back_inserter(Args),
                   [&](auto &arg) {
                     auto FwdArg = Builder.CreateAlloca(arg.getType());
                     Builder.CreateStore(&arg, FwdArg);
                     return Builder.CreateLoad(FwdArg);
                   });

    auto RetVal = Builder.CreateCall(FuncTy, Callable, Args);
    if constexpr (!std::is_same_v<Ret, void>) {
      Builder.CreateRet(RetVal);
    } else {
      Builder.CreateRetVoid();
    }
    return Func;
  }

  static llvm::Type *marshalling(llvm::Module *M) {
    llvm::StructType *ClassTy = M->getTypeByName(Traits::class_name);
    if (!ClassTy) {
      ClassTy = llvm::StructType::create(
          M->getContext(),
          llvm::Type::getInt32Ty(M->getContext())->getPointerTo(),
          Traits::class_name);
    }
    return ClassTy;
  }

protected:
  using Self = BuiltinRegistrar<Traits>;

private:
  llvm::Module *M;
};

namespace detail {

template <typename, typename = void>
struct is_instantiable : std::false_type {};

template <typename T>
struct is_instantiable<T, std::enable_if_t<T::is_instantiable>>
    : std::true_type {};

template <typename T>
static constexpr bool is_instantiable_v = is_instantiable<T>::value;

template <>
struct to_llvm_helper<> {
  static TypeClarifier<> doit(llvm::Module *) { return {}; }
};

template <typename T>
struct to_llvm_helper<T *> {
  static llvm::Type *doit(llvm::Module *M) {
    return to_llvm_helper<T>::doit(M)->getPointerTo();
  }
};

template <typename T>
struct to_llvm_helper<const T> {
  static llvm::Type *doit(llvm::Module *M) {
    return to_llvm_helper<T>::doit(M);
  }
};

template <>
struct to_llvm_helper<int> {
  static llvm::Type *doit(llvm::Module *M) {
    return llvm::Type::getInt32Ty(M->getContext());
  }
};

template <>
struct to_llvm_helper<char> {
  static llvm::Type *doit(llvm::Module *M) {
    return llvm::Type::getInt8Ty(M->getContext());
  }
};

template <>
struct to_llvm_helper<bool> {
  static llvm::Type *doit(llvm::Module *M) {
    return llvm::Type::getInt1Ty(M->getContext());
  }
};

template <>
struct to_llvm_helper<void> {
  static llvm::Type *doit(llvm::Module *M) {
    return llvm::Type::getVoidTy(M->getContext());
  }
};

template <typename, typename = void>
struct to_llvm_entry;

template <typename TypeOrTypeTraits>
struct to_llvm_entry<TypeOrTypeTraits,
                     std::enable_if_t<!is_instantiable_v<TypeOrTypeTraits>>> {
  // Default type specialization
  static TypeClarifier<TypeOrTypeTraits> doit(llvm::Module *M) {
    return detail::to_llvm_helper<TypeOrTypeTraits>::doit(M);
  }
};

template <typename TypeOrTypeTraits>
struct to_llvm_entry<TypeOrTypeTraits,
                     std::enable_if_t<is_instantiable_v<TypeOrTypeTraits>>> {
  // Class type specialization
  static TypeClarifier<TypeOrTypeTraits> doit(llvm::Module *M) {
    return BuiltinRegistrar<TypeOrTypeTraits>::marshalling(M);
  }
};

}  // namespace detail

template <typename... Ts>
struct to_llvm {
  static TypeClarifier<Ts...> doit(llvm::Module *M __attribute__((unused))) {
    return {detail::to_llvm_entry<Ts>::doit(M)...};
  }
};

}  // namespace jcc::builtin

#endif /* jcc_Builtins_hpp */
