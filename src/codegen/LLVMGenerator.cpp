#undef NDEBUG

#include "LLVMGenerator.hpp"

#include "NameMangling.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace {

namespace ir {

template <typename T>
llvm::APInt Int(T i) {
  return llvm::APInt(jcc::constants::bit_width, i, jcc::constants::is_signed);
}

template <typename T>
llvm::APInt Char(T c) {
  return llvm::APInt(jcc::constants::char_width, c, jcc::constants::is_signed);
}

}  // namespace ir

}  // namespace

namespace jcc::ast {

llvm::Function *LLVMGenerator::getLLVMFunction(const std::string &cls,
                                               const std::string &fname) const {
  return module()->getFunction(builtin::generateName(cls, fname));
}

std::string LLVMGenerator::mangleFunction(const FunctionDecl &f) const {
  return builtin::generateName(m_class->getName(), f.getName());
}

std::string LLVMGenerator::mangleStatic(const std::string &varName) const {
  return builtin::generateName(m_class->getName(), varName);
}

llvm::Value *LLVMGenerator::codegen(Node &node) {
  node.accept(*this);

  auto ReplaceAllUsesWith_Unsafe = [&](llvm::Function *from,
                                       llvm::Function *to) {
    assert(from->getFunctionType()->getNumParams() ==
               to->getFunctionType()->getNumParams() &&
           "Function replacement must have the same number of parameters");
    while (!from->use_empty()) {
      auto &U = *from->use_begin();
      auto *C = llvm::cast<llvm::CallInst>(U.getUser());
      auto OldRetTy = C->getFunctionType()->getReturnType();
      auto NewRetTy = to->getFunctionType()->getReturnType();
      C->mutateFunctionType(to->getFunctionType());
      C->setCalledFunction(to);
      if (OldRetTy != NewRetTy) {
        // If the types are not the same, perform a sign ext/trunc to
        // match the old type since that type was chosen based on the
        // surrounding IR
        const auto &uses = C->uses();
        builder().SetInsertPoint(C->getNextNode());
        auto Ext = builder().CreateSExtOrTrunc(C, OldRetTy);
        for (auto &use : uses) { use.set(Ext); }
      }
    }
    from->eraseFromParent();
  };

  // Generate code for the unresolved symbols
  for (auto &unresolved : m_unresolved) {
    // Ideally we would use replaceAllUsesWith but if there is a return type
    // mismatch we will error. Therefore, we use the above unsafe version
    // that still checks the number of arguments
    // unresolved.ToReplace->replaceAllUsesWith(unresolved.ReplaceWith(*this));
    ReplaceAllUsesWith_Unsafe(unresolved.ToReplace,
                              unresolved.ReplaceWith(*this));
  }
  return m_last;
}

llvm::Type *LLVMGenerator::getTypeByName(const std::string &name) {
  llvm::Type *varT = nullptr;
  if (name == "int") {
    varT = builder().getInt32Ty();
  } else if (name == "char") {
    varT = llvm::Type::getInt8Ty(context());
  } else if (name == "boolean") {
    varT = llvm::Type::getInt1Ty(context());
  } else if (name == "Array") {
    varT = module()->getTypeByName(name);
  } else if (name == "void") {
    varT = builder().getVoidTy();
  } else {
    // we have a user-defined type, make it a pointer
    varT = module()->getTypeByName(name);
  }
  assert(varT);
  return varT;
}

llvm::Value *LLVMGenerator::findIdentifier(const std::string &name) {
  auto itr = m_ScopedValueTable.find(name);
  llvm::Value *found = itr != m_ScopedValueTable.end() ? itr->second : nullptr;

  if (!found) {
    // Check class for member

    auto thisItr = m_ScopedValueTable.find("this");
    if (thisItr != m_ScopedValueTable.end()) {
      llvm::Value *thisPtr = thisItr->second;
      auto index = m_class->getFieldIdx(name);
      if (index <
          std::distance(m_class->fields_begin(), m_class->fields_end())) {
        found = builder().CreateInBoundsGEP(
            thisPtr, {builder().getInt32(0), builder().getInt32(index)});
      }
    }
    if (!found) {
      // Check class for static
      found = module()->getGlobalVariable(mangleStatic(name));
    }
  }

  return found;
}

void LLVMGenerator::visit(IntConst &i) {
  m_last = builder().getInt32(i.getInt());
  m_ExpType = m_last->getType();
}

void LLVMGenerator::visit(CharConst &c) {
  m_last = builder().getInt8(c.getChar());
  m_ExpType = m_last->getType();
}

void LLVMGenerator::visit(This &) {
  auto thisItr = m_ScopedValueTable.find("this");
  assert(thisItr != m_ScopedValueTable.end());
  m_last = thisItr->second;
  m_ExpType = builder().getVoidTy();
}

void LLVMGenerator::visit(True &) {
  m_last = builder().getTrue();
  m_ExpType = m_last->getType();
}

void LLVMGenerator::visit(False &) {
  m_last = builder().getFalse();
  m_ExpType = m_last->getType();
}

void LLVMGenerator::visit(Identifier &ident) {
  auto found = findIdentifier(ident.getName());

  // This should be handled in the frontend and we should never not find an
  // identifier
  assert(found && "Undefined identifier fouond in backend");
  m_last = found;
  m_ExpType =
      llvm::cast<llvm::PointerType>(m_last->getType())->getElementType();
}

void LLVMGenerator::visit(StrConst &str) {
  auto charPtr = builder().CreateGlobalStringPtr(str.getString());
  auto strPtr = builder().CreateAlloca(module()->getTypeByName("String"));
  builder().CreateStore(
      builder().CreateCall(getLLVMFunction("String", "ptrtostr"), charPtr),
      strPtr);
  m_last = builder().CreateLoad(strPtr);
  m_ExpType = m_last->getType();
}

void LLVMGenerator::visit(BinaryOp &binop) {
  llvm::Value *lhs = codegenChild(*binop.getLHS());
  llvm::Value *rhs = codegenChild(*binop.getRHS());
  switch (binop.getOp()) {
    case '+':
      m_last = builder().CreateAdd(lhs, rhs, "tmpadd");
      break;
    case '-':
      m_last = builder().CreateSub(lhs, rhs, "tmpsub");
      break;
    case '*':
      m_last = builder().CreateMul(lhs, rhs, "tmpmul");
      break;
    case '/':
      m_last = builder().CreateSDiv(lhs, rhs, "tmpdiv");
      break;
    case '>':
      m_last = builder().CreateICmpSGT(lhs, rhs, "tmpgt");
      break;
    case '<':
      m_last = builder().CreateICmpSLT(lhs, rhs, "tmplt");
      break;
    case '&':
      m_last = builder().CreateAnd(lhs, rhs, "tmpand");
      break;
    case '|':
      m_last = builder().CreateOr(lhs, rhs, "tmpor");
      break;
    case '=':
      m_last = builder().CreateICmpEQ(lhs, rhs, "tmpeq");
      break;
    default:
      assert(false && "Unsupported binary operator");
      m_last = nullptr;
  }
  m_ExpType = m_last->getType();
}

void LLVMGenerator::visit(UnaryOp &unop) {
  llvm::Value *operand = codegenChild(*unop.getOperand());
  switch (unop.getOp()) {
    case '-':
      m_last = builder().CreateNeg(operand, "tmpneg");
      break;
    case '~':
      m_last = builder().CreateNot(operand, "tmpnot");
      break;
    default:
      assert(false && "Unsupported Unary operator");
      m_last = nullptr;
  }
  m_ExpType = m_last->getType();
}

void LLVMGenerator::visit(FunctionCall &call) {
  auto RetTy = m_ExpType;
  llvm::Function *funcI = getLLVMFunction(call.getClassType(), call.getName());

  std::vector<llvm::Value *> argIs;
  std::transform(call.args_begin(), call.args_end(), std::back_inserter(argIs),
                 [&](auto &arg) { return codegenChild(*arg); });

  auto resolve = [&call](LLVMGenerator &g) {
    llvm::Function *funcI =
        g.getLLVMFunction(call.getClassType(), call.getName());

    if (!funcI) {
      // Even after parsing everything, the method still does not exist.
      // Throw an error and exit. This is an internal error for now
      llvm::errs() << "Missing " << call.getName() << '\n';
      g.InternalError(nullptr);
    }
    return funcI;
  };

  if (!funcI) {
    // We could not resolve the symbol, try again once we parse and generate
    // IR for everything
    auto unresolved = UnresolvedFunction(RetTy, argIs);
    m_last = builder().CreateCall(unresolved, argIs);
    m_unresolved.emplace_back(resolve, unresolved);
  } else {
    m_last = builder().CreateCall(resolve(*this), argIs);
  }
}

void LLVMGenerator::visit(MethodCall &call) {
  auto RetTy = m_ExpType;
  std::string classTStr;
  llvm::Value *CalleeV = nullptr;

  if (!call.getCallee()) {
    // Method call from same class
    classTStr = m_class->getName();
    CalleeV = builder().GetInsertBlock()->getParent()->getArg(0);
  } else {
    // Method call on an object callee
    classTStr = call.getCallee()->getType();
    CalleeV = builder().CreateLoad(codegenChild(*call.getCallee()));
  }

  // add this to the argument list
  std::vector<llvm::Value *> argIs;
  argIs.push_back(CalleeV);
  std::transform(call.args_begin(), call.args_end(), std::back_inserter(argIs),
                 [&](auto &arg) { return codegenChild(*arg); });

  auto resolve = [&call, classTStr](LLVMGenerator &g) {
    llvm::Function *funcI = g.getLLVMFunction(classTStr, call.getName());

    if (!funcI) {
      llvm::errs() << "Missing " << call.getName() << '\n';
      g.InternalError(nullptr);
    }

    return funcI;
  };

  llvm::Function *funcI = getLLVMFunction(classTStr, call.getName());

  if (!funcI) {
    // We could not resolve the symbol, try again once we parse and generate
    // IR for everything
    auto unresolved = UnresolvedFunction(RetTy, argIs);
    m_unresolved.emplace_back(resolve, unresolved);
    m_last = builder().CreateCall(unresolved, argIs);
  } else {
    m_last = builder().CreateCall(resolve(*this), argIs);
  }
}

void LLVMGenerator::visit(LetStmt &let) {
  llvm::Value *LHS = codegenChild(*let.getAssignee());
  assert(llvm::isa<llvm::PointerType>(LHS->getType()));
  llvm::Value *RHS = codegenChild(*let.getExpression());
  m_last = builder().CreateStore(RHS, LHS);
  m_ExpType = m_last->getType();
}

// For now, create a default version of thet type specified in the var expr.
// We can then use getType() to find the type in thet class
void LLVMGenerator::visit(VarDecl &decl) {
  llvm::Type *VarT = getTypeByName(decl.getType());
  m_last = builder().CreateAlloca(VarT, nullptr, decl.getName());
  m_ScopedValueTable.insert({decl.getName(), m_last});
  m_ExpType = builder().getVoidTy();
}

void LLVMGenerator::visit(IfStmt &stmt) {
  llvm::BasicBlock *preBB = builder().GetInsertBlock();
  llvm::Function *funcI = preBB->getParent();
  llvm::Value *condV = builder().CreateICmpEQ(codegenChild(*stmt.getCond()),
                                              builder().getTrue(), "ifcond");
  llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context(), "then", funcI);

  llvm::BasicBlock *contBB =
      llvm::BasicBlock::Create(context(), "ifcont", funcI);

  // FIXME: Not sure how I was using this before
  // llvm::BranchInst *preBranch = nullptr;
  if (stmt.getElseBlock()) {
    llvm::BasicBlock *elseBB =
        llvm::BasicBlock::Create(context(), "else", funcI);
    /* preBranch = */ builder().CreateCondBr(condV, thenBB, elseBB);

    builder().SetInsertPoint(elseBB);
    if (!llvm::isa<llvm::ReturnInst>(codegenChild(*stmt.getElseBlock()))) {
      builder().CreateBr(contBB);
    }
  } else {
    /* preBranch = */ builder().CreateCondBr(condV, thenBB, contBB);
  }

  builder().SetInsertPoint(thenBB);
  if (!llvm::isa<llvm::ReturnInst>(codegenChild(*stmt.getIfBlock()))) {
    builder().CreateBr(contBB);
  }

  // Will need this if we use phi nodes
  // // Why then, are we getting the current block when we just set it to
  // ThenBB
  // // 5 lines above? The problem is that the “Then” expression may actually
  // // itself change the block that the Builder is emitting into if, for
  // // example, it contains a nested “if/then/else” expression. Because
  // calling
  // // codegen() recursively could arbitrarily change the notion of the
  // current
  // // block, we are required to get an up-to-date value for code that will
  // set
  // // up the Phi node.
  // thenBB = builder().GetInsertBlock();

  builder().SetInsertPoint(contBB);
  m_ExpType = builder().getVoidTy();
}

void LLVMGenerator::visit(WhileStmt &stmt) {
  llvm::Function *funcI = builder().GetInsertBlock()->getParent();

  llvm::BasicBlock *preHeaderBB =
      llvm::BasicBlock::Create(context(), "preheader", funcI);
  builder().CreateBr(preHeaderBB);
  builder().SetInsertPoint(preHeaderBB);

  llvm::Value *condV = codegenChild(*stmt.getCond());
  llvm::Value *whileV =
      builder().CreateICmpEQ(condV, builder().getTrue(), "whilecond");

  llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context(), "loop", funcI);
  llvm::BasicBlock *contBB =
      llvm::BasicBlock::Create(context(), "endloop", funcI);

  builder().CreateCondBr(whileV, loopBB, contBB);
  builder().SetInsertPoint(loopBB);
  codegenChild(*stmt.getBlock());  // TODO(matt): need to get the second use
                                   // of the identifier in the conditional
  builder().CreateBr(preHeaderBB);

  builder().SetInsertPoint(contBB);
  m_ExpType = builder().getVoidTy();
}

void LLVMGenerator::visit(ReturnStmt &stmt) {
  m_ExpType = builder().GetInsertBlock()->getParent()->getReturnType();
  llvm::Value *Expr = codegenChild(*stmt.getExpr());
  m_ExpType = builder().GetInsertBlock()->getParent()->getReturnType();
  if (Expr && Expr->getType() != m_ExpType &&
      llvm::isa<llvm::IntegerType>(Expr->getType()) &&
      llvm::isa<llvm::IntegerType>(m_ExpType)) {
    Expr = builder().CreateSExtOrTrunc(Expr, m_ExpType);
  }
  m_last = builder().CreateRet(Expr);
  m_ExpType = builder().getVoidTy();
}

void LLVMGenerator::visit(ClassDecl &cls) {
  m_class = &cls;

  std::vector<llvm::Type *> memTs;
  memTs.reserve(cls.numFields());

  std::transform(cls.fields_begin(), cls.fields_end(),
                 std::back_inserter(memTs),
                 [&](const auto &f) { return getTypeByName(f->getType()); });

  // define this class type for use in methods and statics
  llvm::StructType::create(context(), memTs, cls.getName());

  // define the static variables of the class globally
  std::for_each(cls.statics_begin(), cls.statics_end(), [&](auto &s) {
    llvm::Type *varT = getTypeByName(s->getType());
    const auto staticName = mangleStatic(s->getName());
    module()->getOrInsertGlobal(staticName, varT);
    // llvm::GlobalVariable *varI = module()->getNamedGlobal(staticName);
    // varI->setInitializer(
    //     llvm::Constant::getIntegerValue(varT, ::ir::Int(0)));
  });

  std::for_each(cls.mths_begin(), cls.mths_end(),
                [&](auto &e) { e->accept(*this); });
  std::for_each(cls.fcns_begin(), cls.fcns_end(),
                [&](auto &e) { e->accept(*this); });
}

[[noreturn]] void LLVMGenerator::InternalError(llvm::Function *f) {
  // Error reading body, remove function.
  llvm::errs()
      << "\n\n\n[Internal Error] Invalid IR: Terminating\n, Function: \n\n";
  if (f) {
    f->print(llvm::errs());
  } else {
    llvm::errs() << "Not found\n\n";
  }
  llvm::errs() << "\n\t\tModule: \n\n";
  dumpModule();
  assert(false && "Internal error");
}

void LLVMGenerator::allocateArguments(llvm::Function *funcI,
                                      FunctionDecl &decl) {
  std::vector<llvm::AllocaInst *> allocs;
  allocs.reserve(decl.numParams());
  auto prm = decl.prms_begin();
  for (auto &arg : funcI->args()) {
    auto alloc = builder().CreateAlloca(arg.getType());
    allocs.push_back(alloc);
    m_ScopedValueTable.insert({(*prm++)->getName(), alloc});
  }

  auto alloc = allocs.begin();
  for (auto &arg : funcI->args()) { builder().CreateStore(&arg, (*alloc++)); }
}

// TODO(matt) this should be done in a pass on the Jack AST, not after passing
// it to LLVM
void removeAndCastReturns(llvm::Function *func) {
  llvm::IRBuilder<> builder{func->getContext()};

  // handle any early returns
  llvm::Value *retVal = nullptr;
  llvm::BasicBlock *retBB = nullptr;

  std::vector<llvm::ReturnInst *> returns;
  for (auto &bb : func->getBasicBlockList()) {
    for (auto &inst : bb.getInstList()) {
      if (auto *ret = llvm::dyn_cast<llvm::ReturnInst>(&inst)) {
        returns.push_back(ret);
      }
    }
  }

  if (returns.empty()) { return; }

  if (returns.size() > 1) {
    // If there is a single return, then we don't need to do anything since
    // it is at the end of the function
    retBB = llvm::BasicBlock::Create(builder.getContext(), "ret", func);
    auto RetTy = func->getFunctionType()->getReturnType();
    if (!RetTy->isVoidTy()) {
      builder.SetInsertPoint(&func->getEntryBlock().front());
      retVal = builder.CreateAlloca(RetTy);
    }

    for (auto *ret : returns) {
      if (ret->getNumOperands() > 0) {
        // This should be handled during semantic analysis
        assert(retVal &&
               "Function returns void but return statement has operand");
        builder.SetInsertPoint(ret);
        builder.CreateStore(ret->getOperand(0), retVal);
      }

      llvm::ReplaceInstWithInst(ret, llvm::BranchInst::Create(retBB));
    }

    builder.SetInsertPoint(retBB);
    if (retVal)
      builder.CreateRet(builder.CreateLoad(retVal));
    else
      builder.CreateRetVoid();
  }
}

template <typename FunctionType>
llvm::Function *LLVMGenerator::visitFunction(FunctionType &decl) {
  std::vector<llvm::Type *> argTs;
  argTs.reserve(decl.numParams());

  m_ScopedValueTable.clear();

  std::transform(decl.prms_begin(), decl.prms_end(), std::back_inserter(argTs),
                 [&](const auto &p) { return getTypeByName(p->getType()); });

  llvm::FunctionType *funcT = llvm::FunctionType::get(
      getTypeByName(decl.getReturnType()), argTs, false);
  auto funcI = llvm::Function::Create(funcT, llvm::Function::ExternalLinkage,
                                      mangleFunction(decl), module());

  llvm::BasicBlock *bb = llvm::BasicBlock::Create(context(), "entry", funcI);
  builder().SetInsertPoint(bb);

  allocateArguments(funcI, decl);

  return funcI;
}

void LLVMGenerator::visit(StaticDecl &decl) {
  auto funcI = visitFunction(decl);

  decl.getDefinition()->accept(*this);
  removeAndCastReturns(funcI);

  if (llvm::verifyFunction(*funcI, &llvm::errs())) { InternalError(funcI); }

  m_last = funcI;
}

// Constructors are strange - we need to use them as statics, but provide *this*
// as an argument so it can be used in the rest of the function as a keyword
void LLVMGenerator::visit(ConstructorDecl &decl) {
  auto funcI = visitFunction(decl);

  // allocate the object and store the address in this
  auto thisT = getTypeByName(m_class->getName());
  assert(thisT && "Undefined class type");
  llvm::AllocaInst *thisPtr = builder().CreateAlloca(thisT);
  m_ScopedValueTable.insert({"this", thisPtr});

  // codegen rest of the function
  decl.getDefinition()->accept(*this);
  removeAndCastReturns(funcI);

  if (llvm::verifyFunction(*funcI, &llvm::errs())) { InternalError(funcI); }

  m_last = funcI;
}

void LLVMGenerator::visit(MethodDecl &decl) {
  auto funcI = visitFunction(decl);

  decl.getDefinition()->accept(*this);
  removeAndCastReturns(funcI);

  if (llvm::verifyFunction(*funcI, &llvm::errs())) { InternalError(funcI); }

  m_last = funcI;
}

void LLVMGenerator::visit(Block &block) {
  for (auto stmt = block.stmts_begin(); stmt != block.stmts_end(); ++stmt) {
    m_last = codegenChild(**stmt);
  }

  if (builder().GetInsertBlock()->getInstList().empty()) {
    builder().GetInsertBlock()->removeFromParent();
  }
}

void LLVMGenerator::visit(IndexExpr &expr) {
  llvm::Value *idx = codegenChild(*expr.getIndex());
  auto v = findIdentifier(expr.getName());
  assert(v);

  auto dataPtr = builder().CreateInBoundsGEP(
      v, {builder().getInt32(0), builder().getInt32(0)});
  auto data = builder().CreateLoad(dataPtr);
  m_last = builder().CreateInBoundsGEP(data, idx);
  m_ExpType =
      llvm::cast<llvm::PointerType>(m_last->getType())->getElementType();
}

void LLVMGenerator::visit(EmptyNode &) { m_last = nullptr; }

void LLVMGenerator::visit(RValueT &rv) {
  llvm::Value *V = codegenChild(*rv.getWrapped());
  m_last = builder().CreateLoad(V);
  m_ExpType = m_last->getType();
}

void LLVMGenerator::dumpModule() const { module()->print(llvm::errs(), 0); }

auto LLVMGenerator::UnresolvedFunction(llvm::Type *RetTy,
                                       const std::vector<llvm::Value *> &args)
    -> llvm::Function * {
  using namespace llvm;
  std::vector<Type *> argTs;
  argTs.reserve(args.size());
  std::transform(args.begin(), args.end(), std::back_inserter(argTs),
                 [&](const auto *arg) { return arg->getType(); });

  return Function::Create(FunctionType::get(RetTy, argTs, false),
                          Function::ExternalLinkage, "__Unresolved__Function",
                          module());
}

}  // namespace jcc::ast
