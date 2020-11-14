#ifndef COMPILATIONENGINE_HPP
#define COMPILATIONENGINE_HPP

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "JackAST.hpp"
#include "JackLexer.hpp"
#include "JackWriter.hpp"

namespace jcc {

namespace detail {
// base case
template <typename T>
bool match(const T &) {
  return false;
}

template <typename T>
bool match_type(const T &) {
  return false;
}

template <typename... Ts>
bool match(const Token &actual, const Token &expected, Ts &&... tail) {
  if (actual == expected) {
    return true;
  } else {
    return detail::match(actual, std::forward<Ts>(tail)...);
  }
}

// matches only the kind of token, disregarding the rest of the info
template <typename... Ts>
bool match_type(Token::Kind actual, Token::Kind expected, Ts &&... tail) {
  if (actual == expected)
    return true;
  else
    return detail::match_type(actual, std::forward<Ts>(tail)...);
}
}  // namespace detail

void reportError(const std::string &sourceFile, unsigned column, unsigned line,
                 const Token &actual, const Token &expected);

void reportError(const std::string &sourceFile, unsigned column, unsigned line,
                 Token::Kind actual, Token::Kind expected);

// Should take a tokenizer and a writer and
class CompilationEngine {
public:
  CompilationEngine(InputStream input, std::string filename)
      : m_tokenizer{std::move(input)}, m_filename{std::move(filename)} {}

  CompilationEngine(InputStream input)
      : CompilationEngine(std::move(input), "") {}

  template <typename... Ts>
  void match(const Token &actual, const Token &expected, Ts &&... tail) {
    if (!detail::match(actual, expected, std::forward<Ts>(tail)...))
      reportError(m_filename, m_tokenizer.getColNumber(),
                  m_tokenizer.getLineNumber(), actual, expected);
  }

  template <typename... Ts>
  void match(Token::Kind actual, Token::Kind expected, Ts &&... tail) {
    if (!detail::match_type(actual, expected, std::forward<Ts>(tail)...))
      reportError(m_filename, m_tokenizer.getColNumber(),
                  m_tokenizer.getLineNumber(), actual, expected);
  }

  std::unique_ptr<ast::ClassDecl> compileClass();

private:
  ast::VarDecList compileClassVarDec();
  std::unique_ptr<ast::FunctionDecl> compileSubroutineDec();
  ast::ParamList compileParameterList();
  std::unique_ptr<ast::Block> compileBody();
  ast::NodeList compileVarDec();
  std::unique_ptr<ast::Node> compileLet();
  std::unique_ptr<ast::Node> compileIf();
  std::unique_ptr<ast::Node> compileWhile();
  std::unique_ptr<ast::Node> compileDo();
  std::unique_ptr<ast::ReturnStmt> compileReturn();
  std::unique_ptr<ast::Node> compileExpression();
  std::unique_ptr<ast::Node> compileTerm();
  ast::NodeList compileExpressionList();

  template <typename NamedValueType, typename... Ts>
  std::unique_ptr<ast::NamedValue> CreateNamedValue(Ts &&... args);
  std::unique_ptr<ast::VarDecl> CreateVarDecl(std::string, std::string);
  bool isNamedValue(const std::string &) const;

  const Token &getTok() const { return m_tokenizer.peek(); }

  std::string getTypeFromTok();

  JackLexer m_tokenizer;
  std::string m_filename;
  sym::Table *m_currentTable = nullptr;
  ast::FunctionDecl *m_currentFcn = nullptr;
  ast::ClassDecl *m_cls = nullptr;
};
}  // namespace jcc

#endif
