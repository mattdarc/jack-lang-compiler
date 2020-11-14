#include "CompilationEngine.hpp"

#include "ErrorHandling.hpp"
#include "JackLexer.hpp"
#include "SymbolTable.hpp"

namespace jcc {

void reportError(const std::string &sourceFile, unsigned column, unsigned line,
                 const Token &actual, const Token &expected) {
  std::string msg =
      "Expected " + expected.print() + " but found " + actual.print();
  throw SyntaxError(sourceFile, column, line, msg);
}

void reportError(const std::string &sourceFile, unsigned column, unsigned line,
                 Token::Kind actual, Token::Kind expected) {
  std::string msg = "Expected token " + Token::kindToString(expected) +
                    " but found token " + Token::kindToString(actual);
  throw SyntaxError(sourceFile, column, line, msg);
}

std::unique_ptr<ast::ClassDecl> CompilationEngine::compileClass() {
  // class
  match(getTok(), Keyword(Keyword::Type::CLASS));
  m_tokenizer.advance();

  // className
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
  const auto clsName = m_tokenizer.getIdentifier();
  m_tokenizer.advance();
  auto clsAst = std::make_unique<ast::ClassDecl>(clsName);
  m_cls = clsAst.get();
  m_currentTable = &clsAst->getTable();

  // '{'
  match(getTok(), Symbol('{'));
  m_tokenizer.advance();

  // static | field | []
  while (getTok() == Keyword(Keyword::Type::STATIC) ||
         getTok() == Keyword(Keyword::Type::FIELD)) {
    auto vars = compileClassVarDec();
    std::for_each(std::make_move_iterator(vars.fields.begin()),
                  std::make_move_iterator(vars.fields.end()),
                  [&](std::unique_ptr<ast::VarDecl> p) {
                    clsAst->addField(std::move(p));
                  });
    std::for_each(std::make_move_iterator(vars.statics.begin()),
                  std::make_move_iterator(vars.statics.end()),
                  [&](std::unique_ptr<ast::VarDecl> p) {
                    clsAst->addStatic(std::move(p));
                  });
  }

  // constructor | function | method  | []
  while (getTok() == Keyword(Keyword::Type::CONSTRUCTOR) ||
         getTok() == Keyword(Keyword::Type::FUNCTION) ||
         getTok() == Keyword(Keyword::Type::METHOD)) {
    if (getTok() == Keyword(Keyword::Type::METHOD)) {
      clsAst->addMethod(compileSubroutineDec());
    } else {
      clsAst->addFunction(compileSubroutineDec());
    }
  }

  // '}'
  match(getTok(), Symbol('}'));
  m_tokenizer.advance();

  return clsAst;
}

ast::VarDecList CompilationEngine::compileClassVarDec() {
  ast::VarDecList vars;

  // TODO(matt): fields vs statics
  // 'static' | 'field', matched
  match(getTok(), Keyword(Keyword::Type::STATIC),
        Keyword(Keyword::Type::FIELD));
  const auto kind = m_tokenizer.getKeyword() == Keyword::Type::STATIC
                        ? sym::Kind::STATIC
                        : sym::Kind::FIELD;
  m_tokenizer.advance();

  // type
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER, Token::Kind::KEYWORD);
  const auto type = getTypeFromTok();
  m_tokenizer.advance();

  // varName
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
  auto name = m_tokenizer.getIdentifier();
  m_tokenizer.advance();

  // define the symbol in the table
  vars.push_back(CreateVarDecl(std::move(name), type), kind);

  // (',' varName)*
  const auto comma = Symbol(',');
  while (getTok() == comma) {
    // ','
    match(getTok(), comma);
    m_tokenizer.advance();

    // varName
    match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
    name = m_tokenizer.getIdentifier();
    m_tokenizer.advance();

    vars.push_back(CreateVarDecl(std::move(name), type), kind);
  }

  // ;
  match(getTok(), Symbol(';'));
  m_tokenizer.advance();
  return vars;
}

std::unique_ptr<ast::FunctionDecl> CompilationEngine::compileSubroutineDec() {
  // constructor | function | method
  match(getTok(), Keyword(Keyword::Type::CONSTRUCTOR),
        Keyword(Keyword::Type::FUNCTION), Keyword(Keyword::Type::METHOD));
  const auto functionType = m_tokenizer.getKeyword();
  m_tokenizer.advance();

  // type | void
  const auto returnType = getTypeFromTok();
  m_tokenizer.advance();

  // subroutineName
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
  const auto name = m_tokenizer.getIdentifier();
  m_tokenizer.advance();

  // (
  match(getTok(), Symbol('('));
  m_tokenizer.advance();

  // parameterList
  auto params = compileParameterList();

  // )
  match(getTok(), Symbol(')'));
  m_tokenizer.advance();

  std::unique_ptr<ast::FunctionDecl> fcn;
  switch (functionType) {
    case Keyword::Type::CONSTRUCTOR:
      fcn = std::make_unique<ast::ConstructorDecl>(
          std::move(name), std::move(returnType), std::move(params));
      break;
    case Keyword::Type::FUNCTION:
      fcn = std::make_unique<ast::StaticDecl>(
          std::move(name), std::move(returnType), std::move(params));
      break;
    case Keyword::Type::METHOD:
      fcn = std::make_unique<ast::MethodDecl>(
          std::move(name), std::move(returnType), std::move(params));
      break;
    default:
      assert(false);
  }

  // subroutineBody
  m_currentFcn = fcn.get();
  m_currentTable = &fcn->getTable();
  fcn->addDefinition(compileBody());

  // TODO (matt) validate function if it is a constructor to make sure that
  // this is returned, and we have a return statement

  return fcn;
}

ast::ParamList CompilationEngine::compileParameterList() {
  ast::ParamList params;

  if (getTok() == Symbol(')')) {
    // Empty parameter list
    return params;
  }

  // type
  auto type = getTypeFromTok();
  m_tokenizer.advance();

  // varName
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
  auto name = m_tokenizer.getIdentifier();
  m_tokenizer.advance();

  // Define symbols in the symbol table
  params.push_back(CreateVarDecl(name, type));

  // (',' varName)*
  const Token comma = Symbol(',');
  while (getTok() == comma) {
    // ','
    match(getTok(), comma);
    m_tokenizer.advance();

    // type
    type = getTypeFromTok();
    m_tokenizer.advance();

    // varName
    match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
    name = m_tokenizer.getIdentifier();
    m_tokenizer.advance();

    params.push_back(CreateVarDecl(name, type));
  }

  return params;
}

std::unique_ptr<ast::Block> CompilationEngine::compileBody() {
  auto expr = std::make_unique<ast::Block>();

  // {
  match(getTok(), Symbol('{'));
  m_tokenizer.advance();

  // var
  while (getTok() == Keyword(Keyword::Type::VAR)) {
    auto vars = compileVarDec();
    std::for_each(
        std::make_move_iterator(vars.begin()),
        std::make_move_iterator(vars.end()),
        [&](std::unique_ptr<ast::Node> p) { expr->addStmt(std::move(p)); });
  }

  // statement
  if (getTok() != Symbol('}')) {
    while (m_tokenizer.tokenType() == Token::Kind::KEYWORD) {
      match(m_tokenizer.tokenType(), Token::Kind::KEYWORD);
      switch (m_tokenizer.getKeyword()) {
        case Keyword::Type::LET:
          expr->addStmt(compileLet());
          break;
        case Keyword::Type::IF:
          expr->addStmt(compileIf());
          break;
        case Keyword::Type::WHILE:
          expr->addStmt(compileWhile());
          break;
        case Keyword::Type::DO:
          expr->addStmt(compileDo());
          break;
        case Keyword::Type::RETURN:
          expr->addStmt(compileReturn());
          break;
        default:
          assert(false);
      }
    }
  }

  // }
  match(getTok(), Symbol('}'));
  m_tokenizer.advance();

  return expr;
}

ast::NodeList CompilationEngine::compileVarDec() {
  ast::NodeList vars;

  // var
  match(getTok(), Keyword(Keyword::Type::VAR));
  m_tokenizer.advance();

  // type
  const auto type = getTypeFromTok();
  m_tokenizer.advance();

  // varName
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
  auto name = m_tokenizer.getIdentifier();
  m_tokenizer.advance();

  vars.push_back(CreateVarDecl(name, type));

  // (, varName)*
  while (getTok() == Symbol(',')) {
    // ,
    match(getTok(), Symbol(','));
    m_tokenizer.advance();

    // varName
    match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
    name = m_tokenizer.getIdentifier();
    m_tokenizer.advance();

    vars.push_back(CreateVarDecl(name, type));
  }

  // ;
  match(getTok(), Symbol(';'));
  m_tokenizer.advance();

  return vars;
}

std::unique_ptr<ast::Node> CompilationEngine::compileExpression() {
  // term
  auto expr = compileTerm();

  while (m_tokenizer.tokenType() == Token::Kind::SYMBOL) {
    const Symbol::Type op = m_tokenizer.getSymbol();
    if (op != Symbol::PLUS && op != Symbol::MINUS && op != Symbol::MUL &&
        op != Symbol::DIV && op != Symbol::AND && op != Symbol::OR &&
        op != Symbol::GT && op != Symbol::LT && op != Symbol::EQ) {
      break;
    }
    m_tokenizer.advance();

    // term
    expr = std::make_unique<ast::BinaryOp>(Symbol::toChar(op), std::move(expr),
                                           compileTerm());
  }

  return expr;
}

std::unique_ptr<ast::Node> CompilationEngine::compileLet() {
  // let
  match(getTok(), Keyword(Keyword::Type::LET));
  m_tokenizer.advance();

  // varName
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
  const auto varName = m_tokenizer.getIdentifier();
  m_tokenizer.advance();

  // ?[ (array)
  std::unique_ptr<ast::NamedValue> lhs;
  if (getTok() == Symbol('[')) {
    m_tokenizer.advance();

    // expression
    lhs = CreateNamedValue<ast::IndexExpr>(varName, compileExpression());

    // ]
    match(getTok(), Symbol(']'));
    m_tokenizer.advance();
  } else {
    lhs = CreateNamedValue<ast::Identifier>(varName);
  }

  // =
  match(getTok(), Symbol('='));
  m_tokenizer.advance();

  // expression
  auto expr =
      std::make_unique<ast::LetStmt>(std::move(lhs), compileExpression());

  // ;
  match(getTok(), Symbol(';'));
  m_tokenizer.advance();

  return expr;
}

std::unique_ptr<ast::Node> CompilationEngine::compileIf() {
  // if
  match(getTok(), Keyword(Keyword::Type::IF));
  m_tokenizer.advance();

  // (
  match(getTok(), Symbol('('));
  m_tokenizer.advance();

  // expression
  auto condition = compileExpression();

  // )
  match(getTok(), Symbol(')'));
  m_tokenizer.advance();

  // { statements }
  auto ifBranch = compileBody();

  // ?else
  std::unique_ptr<ast::Block> elseBranch;
  if (getTok() == Keyword(Keyword::Type::ELSE)) {
    m_tokenizer.advance();
    elseBranch = compileBody();
  }

  return std::make_unique<ast::IfStmt>(
      std::move(condition), std::move(ifBranch), std::move(elseBranch));
}

std::unique_ptr<ast::Node> CompilationEngine::compileWhile() {
  // while
  match(getTok(), Keyword(Keyword::Type::WHILE));
  m_tokenizer.advance();

  // (
  match(getTok(), Symbol('('));
  m_tokenizer.advance();

  // expression
  auto condition = compileExpression();

  // )
  match(getTok(), Symbol(')'));
  m_tokenizer.advance();

  // { statements }
  return std::make_unique<ast::WhileStmt>(std::move(condition), compileBody());
}

std::unique_ptr<ast::Node> CompilationEngine::compileDo() {
  std::unique_ptr<ast::Node> call;

  // do
  match(getTok(), Keyword(Keyword::Type::DO));
  m_tokenizer.advance();

  // subroutineName | className | varName
  match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
  std::string identifier = m_tokenizer.getIdentifier();
  m_tokenizer.advance();

  std::unique_ptr<ast::NamedValue> callee = nullptr;
  if (getTok() == Symbol('[')) {
    // the identifier is the name of an array
    m_tokenizer.advance();

    // expression
    callee = CreateNamedValue<ast::IndexExpr>(identifier, compileExpression());

    // ]
    match(getTok(), Symbol(']'));
    m_tokenizer.advance();
  } else if (isNamedValue(identifier)) {
    callee = CreateNamedValue<ast::Identifier>(identifier);
  }

  // .
  std::function<std::unique_ptr<ast::Node>(ast::NodeList)> CreateCall;
  if (getTok() == Symbol('.')) {
    m_tokenizer.advance();

    // subroutineName
    match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);
    std::string routine = m_tokenizer.getIdentifier();
    m_tokenizer.advance();

    if (callee) {
      // varName.subroutine == method
      CreateCall = [routine, &callee](ast::NodeList args) {
        return std::make_unique<ast::MethodCall>(std::move(callee), routine,
                                                 std::move(args));
      };
    } else {
      // cls.subroutine == function
      CreateCall = [routine, &identifier](ast::NodeList args) {
        return std::make_unique<ast::FunctionCall>(identifier, routine,
                                                   std::move(args));
      };
    }
  } else {
    // subroutine == method
    CreateCall = [&](ast::NodeList args) {
      return std::make_unique<ast::MethodCall>(nullptr, identifier,
                                               std::move(args));
    };
  }

  // (
  match(getTok(), Symbol('('));
  m_tokenizer.advance();

  // expressionList
  auto args = compileExpressionList();

  // )
  match(getTok(), Symbol(')'));
  m_tokenizer.advance();

  // ;
  match(getTok(), Symbol(';'));
  m_tokenizer.advance();

  return CreateCall(std::move(args));
}

std::unique_ptr<ast::ReturnStmt> CompilationEngine::compileReturn() {
  // return
  match(getTok(), Keyword(Keyword::Type::RETURN));
  m_tokenizer.advance();

  // ?expression
  auto expr = std::make_unique<ast::ReturnStmt>(
      getTok() != Symbol(';') ? compileExpression()
                              : std::make_unique<ast::EmptyNode>());

  // ;
  match(getTok(), Symbol(';'));
  m_tokenizer.advance();

  return expr;
}

std::unique_ptr<ast::Node> CompilationEngine::compileTerm() {
  std::unique_ptr<ast::Node> expr;
  // identifier, integer constant, or string constant
  switch (m_tokenizer.tokenType()) {
    case Token::Kind::IDENTIFIER: {
      // varName | subroutineCall

      std::string identifier = m_tokenizer.getIdentifier();
      m_tokenizer.advance();

      std::unique_ptr<ast::NamedValue> namedValue = nullptr;
      if (getTok() == Symbol('[')) {
        // the identifier is the name of an array
        m_tokenizer.advance();

        // expression
        namedValue =
            CreateNamedValue<ast::IndexExpr>(identifier, compileExpression());

        // ]
        match(getTok(), Symbol(']'));
        m_tokenizer.advance();
      } else if (isNamedValue(identifier)) {
        namedValue = CreateNamedValue<ast::Identifier>(identifier);
      }

      if (getTok() == Symbol('.')) {
        // the identifier is a function call for another class or a
        // static method
        m_tokenizer.advance();
        match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER);

        // subroutineName
        const std::string subroutine = m_tokenizer.getIdentifier();
        m_tokenizer.advance();

        // begin function call
        match(getTok(), Symbol('('));
        m_tokenizer.advance();

        auto args = compileExpressionList();

        // end function call
        match(getTok(), Symbol(')'));
        m_tokenizer.advance();

        if (namedValue) {
          // callee (?[ expr ]) . subroutine
          expr = std::make_unique<ast::MethodCall>(
              std::move(namedValue), std::move(subroutine), std::move(args));
        } else {
          // type . subroutine
          expr = std::make_unique<ast::FunctionCall>(
              std::move(identifier), std::move(subroutine), std::move(args));
        }

      } else if (getTok() == Symbol('(')) {
        // the identifier is the name of a function of the current
        // object

        // begin function call
        match(getTok(), Symbol('('));
        m_tokenizer.advance();

        auto args = compileExpressionList();

        // end function call
        match(getTok(), Symbol(')'));
        m_tokenizer.advance();

        // subroutine
        expr = std::make_unique<ast::MethodCall>(nullptr, std::move(identifier),
                                                 std::move(args));

      } else {
        // just a varName
        expr = RValue(std::move(namedValue));
      }
    } break;
    case Token::Kind::SYMBOL: {
      // '(' expression ')'
      match(getTok(), Symbol('('), Symbol('~'), Symbol('-'));
      const Symbol::Type sym = m_tokenizer.getSymbol();
      m_tokenizer.advance();
      switch (sym) {
        default:
          assert(false && "unreachable!");
        case Symbol::L_PAREN:
          expr = compileExpression();
          match(getTok(), Symbol(sym));
          m_tokenizer.advance();
          break;
        case Symbol::NOT:
        case Symbol::MINUS:
          expr = std::make_unique<ast::UnaryOp>(sym, compileTerm());
      }
    } break;
    case Token::Kind::INTEGER_CONSTANT: {
      expr = std::make_unique<ast::IntConst>(m_tokenizer.getInt());
      m_tokenizer.advance();
    } break;
    case Token::Kind::STRING_CONSTANT: {
      // create new string
      expr = std::make_unique<ast::StrConst>(m_tokenizer.getString());
      m_tokenizer.advance();
    } break;
    case Token::Kind::KEYWORD: {
      match(getTok(), Keyword(Keyword::Type::TRUE),
            Keyword(Keyword::Type::FALSE), Keyword(Keyword::Type::NIL),
            Keyword(Keyword::Type::THIS));
      switch (m_tokenizer.getKeyword()) {
        case Keyword::Type::TRUE:
          expr = ast::Constant::getTrue();
          break;
        case Keyword::Type::FALSE:
          expr = ast::Constant::getFalse();
          break;
        case Keyword::Type::NIL:
          expr = std::make_unique<ast::IntConst>(0);
          break;
        case Keyword::Type::THIS:
          expr = RValue(ast::Constant::getThis());
          break;
        default:
          assert(false);
      }
      m_tokenizer.advance();
    } break;
    default:
      assert(false);  // won't get here
  }
  assert(expr);
  return expr;
}

// Returns the number of arguments
ast::NodeList CompilationEngine::compileExpressionList() {
  ast::NodeList list;
  if (getTok() != Symbol(')')) {
    // expression
    list.push_back(compileExpression());

    // (, expression)?
    while (getTok() != Symbol(')')) {
      match(getTok(), Symbol(','));
      m_tokenizer.advance();

      list.push_back(compileExpression());
    }
  }
  return list;
}

std::string CompilationEngine::getTypeFromTok() {
  switch (m_tokenizer.tokenType()) {
    case Token::Kind::IDENTIFIER:
      // user-defined type
      return m_tokenizer.getIdentifier();
    case Token::Kind::KEYWORD:
      // built-in type;
      match(getTok(), Keyword(Keyword::Type::INT), Keyword(Keyword::Type::CHAR),
            Keyword(Keyword::Type::BOOLEAN), Keyword(Keyword::Type::VOID));
      return Keyword::toString(m_tokenizer.getKeyword());
    default:
      match(m_tokenizer.tokenType(), Token::Kind::IDENTIFIER,
            Token::Kind::KEYWORD);
      return std::string{};
  }
}

template <typename NamedValueType, typename... Ts>
std::unique_ptr<ast::NamedValue> CompilationEngine::CreateNamedValue(
    Ts &&... args) {
  auto v =
      std::make_unique<NamedValueType>(std::forward<Ts>(args)..., m_currentFcn);

  // Check for the existance of the identifier. If it does not exist, the we
  // should throw an error
  assert(m_currentFcn->getTable().lookup(v->getName()) ||
         m_cls->getTable().lookup(v->getName()));

  return v;
}

std::unique_ptr<ast::VarDecl> CompilationEngine::CreateVarDecl(
    std::string name, std::string type) {
  auto var = std::make_unique<ast::VarDecl>(std::move(name), std::move(type));
  m_currentTable->addValue(var.get());
  return var;
}

bool CompilationEngine::isNamedValue(const std::string &name) const {
  return m_currentFcn->getTable().lookup(name) ||
         m_cls->getTable().lookup(name);
}

}  // namespace jcc
