#include <iostream>
#include <sstream>

#include "ASTBuilder.hpp"
#include "Runtime.hpp"
#include "gtest/gtest.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"

using namespace jcc;
using namespace jcc::ast;

class LLVMFixture : public ::testing::Test {
protected:
  // void SetUp() override { JackRuntime = Runtime{TestInput, TestOutput}; }
  std::unique_ptr<ClassDecl> rootClass = std::make_unique<ClassDecl>("Main");
  Builder astBuilder = Builder().setClass(rootClass.get());
  FunctionDecl *main = astBuilder.CreateStaticDecl("main", "int");
  Block *block = main->getDefinition();
  std::istringstream TestInput;
  std::ostringstream TestOutput;
  Runtime JackRuntime{TestInput, TestOutput};

  template <typename IRType>
  IRType *CheckCodegen(std::unique_ptr<Node> node) {
    using namespace llvm;
    const std::string Desc = PrettyPrinter::print(*node);

    JackRuntime.addAST(std::move(node));
    auto result = dyn_cast<IRType>(JackRuntime.codegen());
    assert(result && "Incorrect type");

    return result;
  }

  void CheckCodegen(std::unique_ptr<Node> node) {
    CheckCodegen<llvm::Value>(std::move(node));
  }

  auto CheckConstantExpr(std::unique_ptr<Node> node, int ExpVal) {
    const auto asString = PrettyPrinter::print(*block);
    auto result = CheckCodegen<llvm::Value>(std::move(node));
    ASSERT_NE(result, nullptr);

    if (auto Const = llvm::dyn_cast<llvm::ConstantInt>(result)) {
      EXPECT_EQ(Const->getSExtValue(), ExpVal)
          << ExpVal << " != " << Const->getSExtValue();
    } else {
      FAIL() << "Incorrect type - not a constant expression";
    }
  }

  auto CheckExecution(int ExpVal) { EXPECT_EQ(JackRuntime.run(), ExpVal); }
};

TEST_F(LLVMFixture, StrConst) {
  const std::string str = "theConstant";
  block->addStmt(std::make_unique<StrConst>(str));
  block->addStmt(astBuilder.CreateReturn(0));
  CheckCodegen(std::move(rootClass));

  const auto &globals = JackRuntime.module().getGlobalList();

  EXPECT_TRUE(
      std::count_if(globals.begin(), globals.end(), [&](const auto &glob) {
        if (const auto init = llvm::dyn_cast<llvm::ConstantDataArray>(
                glob.getInitializer())) {
          return init->getAsCString() == str;
        }

        return false;
      }) > 0);
}

TEST_F(LLVMFixture, Keywords) {
  auto trueV = CheckCodegen<llvm::ConstantInt>(std::make_unique<True>());
  EXPECT_TRUE(trueV->equalsInt(1));

  auto falseV = CheckCodegen<llvm::ConstantInt>(std::make_unique<False>());
  EXPECT_TRUE(falseV->equalsInt(0));
}

TEST_F(LLVMFixture, IntConst) {
  const int intValue = 10;
  auto constant =
      CheckCodegen<llvm::ConstantInt>(std::make_unique<IntConst>(intValue));

  auto type = constant->getType();
  ASSERT_TRUE(type);
  EXPECT_TRUE(llvm::isa<llvm::IntegerType>(type));
  EXPECT_TRUE(type->getBitWidth() == jcc::constants::bit_width);
  EXPECT_TRUE(type->getSignBit() != 0);
  EXPECT_TRUE(constant->equalsInt(intValue));
}

TEST_F(LLVMFixture, BinOp) {
  // Binary
  CheckConstantExpr(std::make_unique<BinaryOp>('+', 5, 15), 20);
  CheckConstantExpr(std::make_unique<BinaryOp>('*', 5, 15), 75);
  CheckConstantExpr(std::make_unique<BinaryOp>('/', 15, 4), 3);
  CheckConstantExpr(std::make_unique<BinaryOp>('-', 15, 4), 11);
  CheckConstantExpr(std::make_unique<BinaryOp>('-', 4, 15), -11);
  CheckConstantExpr(std::make_unique<BinaryOp>('>', 4, 15), 0);
  CheckConstantExpr(std::make_unique<BinaryOp>('<', 4, 15), -1);
  CheckConstantExpr(std::make_unique<BinaryOp>('&', 1, 1), 1);
  CheckConstantExpr(std::make_unique<BinaryOp>('&', 2, 1), 0);
  CheckConstantExpr(std::make_unique<BinaryOp>('|', 2, 1), 3);
  CheckConstantExpr(std::make_unique<BinaryOp>('|', 1, 1), 1);

  // Binary combinations (Right to left)
  CheckConstantExpr(std::make_unique<BinaryOp>('+', 5, 15), 20);
  CheckConstantExpr(std::make_unique<BinaryOp>('+', 5, '*', 15, 10), 155);
  CheckConstantExpr(std::make_unique<BinaryOp>('-', 15, '+', 5, '/', 100, 10),
                    0);
}

TEST_F(LLVMFixture, UnaryOp) {
  CheckConstantExpr(std::make_unique<UnaryOp>('-', 10), -10);
  CheckConstantExpr(std::make_unique<UnaryOp>('~', 10), ~10);
}

TEST_F(LLVMFixture, LetStmt) {
  const std::string name = "varName";
  const std::string type = "int";
  const int ExpValue = 150;

  block->addStmt(astBuilder.CreateVarDecl(name, type));
  block->addStmt(astBuilder.CreateLet(name, ExpValue));
  block->addStmt(astBuilder.CreateReturn(name));

  CheckCodegen<llvm::Value>(std::move(rootClass));
  EXPECT_TRUE(JackRuntime.run() == ExpValue);
}

TEST_F(LLVMFixture, WhileStmt) {
  const std::string name = "varName";
  const std::string type = "int";
  const int StartValue = 100;
  const int ExpValue = 150;

  // var int varName;
  block->addStmt(astBuilder.CreateVarDecl(name, type));

  // let varName = StartValue;
  block->addStmt(astBuilder.CreateLet(name, StartValue));

  // let varName = varName + 1;
  auto whileBlock = std::make_unique<Block>();
  whileBlock->addStmt(astBuilder.CreateLet(
      name,
      std::make_unique<BinaryOp>('+', RValue(astBuilder.CreateIdentifier(name)),
                                 std::make_unique<IntConst>(1))));

  // while (varName < ExpValue)
  auto While =
      astBuilder.CreateWhile('<', name, ExpValue, std::move(whileBlock));
  block->addStmt(std::move(While));
  block->addStmt(astBuilder.CreateReturn(name));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ExpValue);
}

class IfStmtTest : public LLVMFixture {
protected:
  std::string name = "varName";
  std::string type = "int";
  int ExpValue = 150;
  int ElseExpValue = 100;

  std::unique_ptr<Block> ifBlock;
  std::unique_ptr<Block> elseBlock;

  void SetUp() override {
    LLVMFixture::SetUp();
    name = "varName";
    type = "int";
    ExpValue = 150;
    ElseExpValue = 100;

    // var int varName;
    block->addStmt(astBuilder.CreateVarDecl(name, type));

    // let varName = 0;
    block->addStmt(astBuilder.CreateLet(name, 0));

    // let varName = ExpValue;
    ifBlock = std::make_unique<Block>();
    ifBlock->addStmt(astBuilder.CreateLet(name, ExpValue));

    // let varName = 0;
    elseBlock = std::make_unique<Block>();
    elseBlock->addStmt(astBuilder.CreateLet(name, ElseExpValue));
  }
};

TEST_F(IfStmtTest, TrueWithElse) {
  // if (varName = 0) ... else ...
  auto If = astBuilder.CreateIf('=', name, 0, std::move(ifBlock),
                                std::move(elseBlock));

  block->addStmt(std::move(If));
  block->addStmt(astBuilder.CreateReturn(name));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ExpValue);
}

TEST_F(IfStmtTest, FalseWithElse) {
  // if (varName = 1) ... else ...
  auto If = astBuilder.CreateIf('=', name, 1, std::move(ifBlock),
                                std::move(elseBlock));

  block->addStmt(std::move(If));
  block->addStmt(astBuilder.CreateReturn(name));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ElseExpValue);
}

TEST_F(IfStmtTest, WithoutElse) {
  // if (varName = 0) ...
  auto If = astBuilder.CreateIf('=', name, 0, std::move(ifBlock));

  block->addStmt(std::move(If));
  block->addStmt(astBuilder.CreateReturn(name));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ExpValue);
}

class MethodTest : public LLVMFixture {
protected:
  int MemberVal = 100;
  std::string jackFunction = "callable";
  std::string memberName = "theMember";
  std::string theType = "int";

  void SetUp() override {
    LLVMFixture::SetUp();
    MemberVal = 100;
    jackFunction = "callable";
    memberName = "theMember";
    theType = "int";

    // var int theMember
    astBuilder.CreateMemberVar(memberName, theType);

    // constructor new()
    //     let theMember = 100;
    //     return this;
    // end
    auto ctorDecl = astBuilder.CreateConstructorDecl();
    auto ctorBlock = ctorDecl->getDefinition();
    ctorBlock->addStmt(astBuilder.CreateLet(memberName, MemberVal));
    ctorBlock->addStmt(
        astBuilder.CreateReturn(RValue(std::make_unique<This>())));

    // Create method to call
    // function int callable()
    //     return theMember;
    // end
    auto function = astBuilder.CreateMethodDecl(jackFunction, theType);
    function->getDefinition()->addStmt(astBuilder.CreateReturn(memberName));

    astBuilder.setFunction(main);
  }
};

TEST_F(MethodTest, FirstLevel) {
  using namespace llvm;

  // Create the function call expression
  // var jackclass classInst;
  // let classInst = jackclass.new()
  // return classInst.callable();
  std::string varName = "classInst";
  block->addStmt(astBuilder.CreateVarDecl(varName, rootClass->getName()));
  block->addStmt(astBuilder.CreateLet(
      varName, astBuilder.CreateFunctionCall(rootClass->getName(), "new")));
  block->addStmt(astBuilder.CreateReturn(
      astBuilder.CreateMethodCall(varName, jackFunction)));

  CheckCodegen(std::move(rootClass));
  CheckExecution(MemberVal);
}

TEST_F(MethodTest, SecondLevel) {
  using namespace llvm;

  // Define the method to wrap the first
  auto wrapper = astBuilder.CreateMethodDecl("wrapper", theType);
  wrapper->getDefinition()->addStmt(
      astBuilder.CreateReturn(astBuilder.CreateMethodCall(jackFunction)));

  // Create the function call expression
  // var jackclass classInst;
  // let classInst = jackclass.new()
  // return classInst.wrapper();
  std::string varName = "classInst";
  block->addStmt(astBuilder.CreateVarDecl(varName, rootClass->getName()));
  block->addStmt(astBuilder.CreateLet(
      varName, astBuilder.CreateFunctionCall(rootClass->getName(), "new")));
  block->addStmt(
      astBuilder.CreateReturn(astBuilder.CreateMethodCall(varName, "wrapper")));

  CheckCodegen(std::move(rootClass));
  CheckExecution(MemberVal);
}

class ArrayTest : public LLVMFixture {
protected:
  std::string theArray;
  void SetUp() override {
    LLVMFixture::SetUp();
    theArray = "arr";

    // var Array arr;
    block->addStmt(astBuilder.CreateVarDecl(theArray, "Array"));

    // let arr = Array.new(10)
    NodeList args;
    args.push_back(std::make_unique<IntConst>(10));
    block->addStmt(astBuilder.CreateLet(
        theArray,
        astBuilder.CreateFunctionCall("Array", "new", std::move(args))));
  }
};

TEST_F(ArrayTest, Index) {
  int ExpValue = 10;
  // let arr[5] = 10;
  block->addStmt(
      astBuilder.CreateLet(astBuilder.CreateIndexInto(theArray, 5), ExpValue));

  // return arr[5];
  block->addStmt(
      astBuilder.CreateReturn(RValue(astBuilder.CreateIndexInto(theArray, 5))));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ExpValue);
}

TEST_F(ArrayTest, ComplexIndex) {
  int ExpValue = 50;
  // let arr[a+b] = 50;
  block->addStmt(astBuilder.CreateLet(
      astBuilder.CreateIndexInto(theArray,
                                 astBuilder.CreateArithmetic('+', 2, 3)),
      ExpValue));

  // return arr[5];
  block->addStmt(
      astBuilder.CreateReturn(RValue(astBuilder.CreateIndexInto(theArray, 5))));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ExpValue);
}

TEST_F(ArrayTest, Deallocate) {
  // do arr.dispose()
  block->addStmt(astBuilder.CreateMethodCall(theArray, "dispose"));

  // return arr;
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
}

class StringTest : public LLVMFixture {
protected:
  std::string theString;
  std::string theType;
  void SetUp() override {
    LLVMFixture::SetUp();
    theString = "str";
    theType = "int";
    int cap = 10;

    // var String str;
    block->addStmt(astBuilder.CreateVarDecl(theString, "String"));

    // let str = String.new(10)
    NodeList args;
    args.push_back(std::make_unique<IntConst>(cap));
    block->addStmt(astBuilder.CreateLet(
        theString,
        astBuilder.CreateFunctionCall("String", "new", std::move(args))));
  }
};

TEST_F(StringTest, StringLength) {
  // return str.length();
  block->addStmt(astBuilder.CreateReturn(
      astBuilder.CreateMethodCall(theString, "length")));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
}

TEST_F(StringTest, SetCharAt) {
  // There seems to be a bug in LLVM that I'm hitting IR is valid but the
  // DAGBuilder chokes
  // do str.setCharAt(0, 'a');
  NodeList args;
  args.push_back(std::make_unique<IntConst>(0));
  args.push_back(std::make_unique<CharConst>('a'));
  block->addStmt(
      astBuilder.CreateMethodCall(theString, "setCharAt", std::move(args)));

  // return str.charAt(0);
  args.clear();
  args.push_back(std::make_unique<IntConst>(0));
  block->addStmt(astBuilder.CreateReturn(
      astBuilder.CreateMethodCall(theString, "charAt", std::move(args))));

  CheckCodegen(std::move(rootClass));
  CheckExecution('a');
}

TEST_F(StringTest, AppendChar) {
  // do str.appendChar('a');
  auto i = 0;
  for (; i < 15; ++i) {
    NodeList args;
    args.push_back(std::make_unique<CharConst>('a'));
    block->addStmt(
        astBuilder.CreateMethodCall(theString, "appendChar", std::move(args)));
  }

  // return str.charAt(i-1);
  NodeList args;
  args.push_back(std::make_unique<IntConst>(i - 1));
  block->addStmt(astBuilder.CreateReturn(
      astBuilder.CreateMethodCall(theString, "charAt", std::move(args))));

  CheckCodegen(std::move(rootClass));
  CheckExecution('a');
}

TEST_F(StringTest, EraseLastChar) {
  NodeList args;
  // args.push_back(std::make_unique<IntConst>(i - 1));
  args.push_back(std::make_unique<CharConst>('a'));
  block->addStmt(
      astBuilder.CreateMethodCall(theString, "appendChar", std::move(args)));

  block->addStmt(astBuilder.CreateMethodCall(theString, "eraseLastChar"));
  block->addStmt(astBuilder.CreateReturn(
      astBuilder.CreateMethodCall(theString, "length")));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
}

TEST_F(StringTest, Dispose) {
  // do str.dispose();
  block->addStmt(astBuilder.CreateMethodCall(theString, "dispose"));

  // return 0;
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
}

TEST_F(LLVMFixture, UnresolvedCalls) {
  const std::string unresolved = "unresolved";

  int ExpValue = 10;
  std::string theType = "int";
  // SECTION("int") {
  ExpValue = 10;
  theType = "int";
  //}

  // SECTION("char") {
  ExpValue = 'a';
  theType = "char";
  //}

  // SECTION("bool") {
  ExpValue = 0;
  theType = "boolean";
  //}

  // Call the undefined function
  block->addStmt(astBuilder.CreateReturn(
      astBuilder.CreateFunctionCall(rootClass->getName(), unresolved)));

  // Define the undefined function
  auto undef = astBuilder.CreateStaticDecl(unresolved, theType);
  block = undef->getDefinition();

  block->addStmt(astBuilder.CreateReturn(ExpValue));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ExpValue);
}

TEST_F(LLVMFixture, CallsWithArguments) {
  const std::string theType = "int";
  const std::string unresolved = "unresolved";
  const int ExpValue = 10;

  ParamList params;
  params.push_back(astBuilder.CreateParameter("a", theType));
  params.push_back(astBuilder.CreateParameter("b", theType));
  params.push_back(astBuilder.CreateParameter("c", theType));
  auto prmFunc = astBuilder.CreateStaticDecl("add", theType, std::move(params));
  auto prmBlock = prmFunc->getDefinition();

  auto first =
      astBuilder.CreateArithmetic('+', RValue(astBuilder.CreateIdentifier("a")),
                                  RValue(astBuilder.CreateIdentifier("b")));
  prmBlock->addStmt(astBuilder.CreateReturn(astBuilder.CreateArithmetic(
      '+', RValue(astBuilder.CreateIdentifier("c")), std::move(first))));

  NodeList args;
  args.push_back(std::make_unique<IntConst>(3));
  args.push_back(std::make_unique<IntConst>(4));
  args.push_back(std::make_unique<IntConst>(3));
  block->addStmt(astBuilder.CreateReturn(astBuilder.CreateFunctionCall(
      rootClass->getName(), "add", std::move(args))));

  CheckCodegen(std::move(rootClass));
  CheckExecution(ExpValue);
}

class OutputBuiltinTest : public LLVMFixture {
protected:
  std::string theType = "int";
  NodeList args;
};

TEST_F(OutputBuiltinTest, PrintChar) {
  args.push_back(std::make_unique<CharConst>('a'));
  block->addStmt(
      astBuilder.CreateFunctionCall("Output", "printChar", std::move(args)));
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
  EXPECT_TRUE(TestOutput.str() == "a");
}

TEST_F(OutputBuiltinTest, PrintString) {
  // TODO(matt) need to overload functions if I want string constants and
  // strings to be different. For now, string constant will just be a string
  const std::string ExpStr = "A String to print";
  args.push_back(std::make_unique<StrConst>(ExpStr));
  block->addStmt(
      astBuilder.CreateFunctionCall("Output", "printString", std::move(args)));
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
  EXPECT_TRUE(TestOutput.str() == ExpStr);
}

TEST_F(OutputBuiltinTest, PrintInt) {
  const int ExpVal = 11020;
  args.push_back(std::make_unique<IntConst>(ExpVal));
  block->addStmt(
      astBuilder.CreateFunctionCall("Output", "printInt", std::move(args)));
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
  EXPECT_TRUE(TestOutput.str() == std::to_string(ExpVal));
}

TEST_F(OutputBuiltinTest, PrintLn) {
  block->addStmt(astBuilder.CreateFunctionCall("Output", "println"));
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
  EXPECT_TRUE(TestOutput.str() == "\n");
}

class KeyboardTest : public LLVMFixture {
protected:
  std::string msg = "This is a test message";
  NodeList args;
};

TEST_F(KeyboardTest, ReadLine) {
  const std::string input = "The input";
  TestInput.str(input);

  block->addStmt(astBuilder.CreateVarDecl("retStr", "String"));
  args.push_back(std::make_unique<StrConst>(msg));
  block->addStmt(astBuilder.CreateLet(
      "retStr",
      astBuilder.CreateFunctionCall("Keyboard", "readLine", std::move(args))));

  args.push_back(RValue(astBuilder.CreateIdentifier("retStr")));
  block->addStmt(
      astBuilder.CreateFunctionCall("Test", "inspectStr", std::move(args)));
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
  EXPECT_TRUE(TestOutput.str() == msg);
  // EXPECT_TRUE(builtin::inspect_runtime<std::string>::get() == input);
}

TEST_F(KeyboardTest, ReadInt) {
  const int inputInt = 1245;
  TestInput.str(std::to_string(inputInt));

  block->addStmt(astBuilder.CreateVarDecl("retInt", "int"));
  args.push_back(std::make_unique<StrConst>(msg));
  block->addStmt(astBuilder.CreateLet(
      "retInt",
      astBuilder.CreateFunctionCall("Keyboard", "readInt", std::move(args))));

  args.push_back(RValue(astBuilder.CreateIdentifier("retInt")));
  block->addStmt(
      astBuilder.CreateFunctionCall("Test", "inspectInt", std::move(args)));
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
  EXPECT_TRUE(TestOutput.str() == msg);
  // EXPECT_TRUE(builtin::inspect_runtime<int>::get() == input);
}

// TODO(matt) move to generator tests
TEST_F(LLVMFixture, BuiltinTypes) {
  // auto &Ctx =
  // const_cast<llvm::LLVMContext&>(JackRuntime().getContext());
  //     EXPECT_TRUE(generator.getTypeByName("void") ==
  //     llvm::Type::getVoidTy(Ctx)); EXPECT_TRUE(generator.getTypeByName("int")
  //     == llvm::Type::getInt32Ty(Ctx));
  //     EXPECT_TRUE(generator.getTypeByName("char") ==
  //     llvm::Type::getInt8Ty(Ctx));
  //     EXPECT_TRUE(generator.getTypeByName("boolean") ==
  //     llvm::Type::getInt1Ty(Ctx));
  EXPECT_TRUE(JackRuntime.module().getTypeByName("Array") != nullptr);
  EXPECT_TRUE(JackRuntime.module().getTypeByName("String") != nullptr);
}

TEST_F(LLVMFixture, StaticVariables) {}

TEST_F(LLVMFixture, EarlyReturns) {}

TEST_F(LLVMFixture, ASTUtilities) {
  const std::string theNode = "node";

  block->addStmt(astBuilder.CreateVarDecl(theNode, "ASTNode"));
  block->addStmt(astBuilder.CreateLet(
      theNode, astBuilder.CreateFunctionCall("ASTNode", "getRoot")));

  NodeList args;
  args.push_back(RValue(astBuilder.CreateIdentifier(theNode)));
  block->addStmt(
      astBuilder.CreateFunctionCall("ASTNode", "print", std::move(args)));
  block->addStmt(astBuilder.CreateReturn(0));

  CheckCodegen(std::move(rootClass));
  CheckExecution(0);
  std::cout << TestOutput.str();
}
