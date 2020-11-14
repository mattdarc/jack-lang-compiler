// #define CATCH_CONFIG_RUNNER
#include <sstream>

#include "catch2/catch.hpp"

// this order needs to be maintained since TRUE is a macro that needs
// to be undef'd on mac
#include "JackLexer.hpp"

using namespace jcc;

namespace {
struct TestInput {
  TestInput(std::string &&input) : m_string(std::move(input)) {}
  operator InputStream() {
    return std::make_unique<std::istringstream>(m_string);
  }

private:
  std::string m_string;
};
}  // namespace

TEST_CASE("Quotes are parsed out of string constants", "[lexer]") {
  JackLexer lexer(TestInput("\"StringConstant\""));
  REQUIRE(lexer.consume() == StringConstant("StringConstant"));
  REQUIRE(!lexer.hasMoreTokens());

  lexer(TestInput("\"String\" \"Constant\""));
  REQUIRE(lexer.consume() == StringConstant("String"));
  REQUIRE(lexer.consume() == StringConstant("Constant"));
  REQUIRE(!lexer.hasMoreTokens());
}

TEST_CASE("Trailing and extra whitespace is removed", "[lexer]") {
  JackLexer lexer{TestInput("  identifier \n \t")};
  REQUIRE(lexer.consume() == Identifier("identifier"));
  REQUIRE(!lexer.hasMoreTokens());

  lexer(TestInput("  identifier1 \t identifier2 \n \t"));
  REQUIRE(lexer.consume() == Identifier("identifier1"));
  REQUIRE(lexer.consume() == Identifier("identifier2"));
  REQUIRE(!lexer.hasMoreTokens());
}

TEST_CASE("String constants can contain non alphabetic characters", "[lexer]") {
  JackLexer lexer{TestInput("\"String Constant\"")};
  REQUIRE(lexer.consume() == StringConstant("String Constant"));
  REQUIRE(!lexer.hasMoreTokens());

  lexer(TestInput(
      "\"String Constant, with a class keyword and the number 420\""));
  REQUIRE(lexer.consume() ==
          StringConstant(
              "String Constant, with a class keyword and the number 420"));
  REQUIRE(!lexer.hasMoreTokens());
}

TEST_CASE("We can have single and multiline comments", "[lexer]") {
  JackLexer lexer{
      TestInput("// Some misc words that should not be processed\n")};
  REQUIRE(lexer.consume() == Token());
  REQUIRE(!lexer.hasMoreTokens());

  lexer(TestInput("/* Some misc words that should not be processed */\n"));
  REQUIRE(lexer.consume() == Token());
  REQUIRE(!lexer.hasMoreTokens());

  lexer(TestInput(
      "/* Some misc \n words that \n should * / not be processed */\n"));
  REQUIRE(lexer.consume() == Token());
  INFO("token=" << lexer.peek());
  REQUIRE(!lexer.hasMoreTokens());

  lexer(
      TestInput("// Some misc \n /// words // that should not be processed\n"));
  REQUIRE(lexer.consume() == Token());
  REQUIRE(!lexer.hasMoreTokens());
}

TEST_CASE("Correctly identify symbols", "[lexer]") {
  JackLexer lexer(TestInput(",+-"));
  REQUIRE(lexer.consume() == Symbol(Symbol::COMMA));
  REQUIRE(lexer.consume() == Symbol(Symbol::PLUS));
  REQUIRE(lexer.consume() == Symbol(Symbol::MINUS));
  INFO("token=" << lexer.peek());
  REQUIRE(!lexer.hasMoreTokens());
}

TEST_CASE("Correctly identify all Jack keywords", "[lexer]") {
  JackLexer lexer{
      TestInput("class "
                "constructor "
                "function "
                "method "
                "field "
                "static "
                "var "
                "int "
                "char "
                "boolean "
                "void "
                "true "
                "false "
                "null "
                "this "
                "let "
                "do "
                "if "
                "else "
                "while "
                "return ")};

  REQUIRE(lexer.consume_as<Keyword>() == Keyword::CLASS);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::CONSTRUCTOR);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::FUNCTION);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::METHOD);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::FIELD);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::STATIC);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::VAR);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::INT);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::CHAR);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::BOOLEAN);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::VOID);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::TRUE);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::FALSE);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::NIL);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::THIS);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::LET);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::DO);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::IF);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::ELSE);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::WHILE);
  REQUIRE(lexer.consume_as<Keyword>() == Keyword::RETURN);
  INFO("token=" << lexer.peek());
  REQUIRE(!lexer.hasMoreTokens());
}

TEST_CASE("Identify integer constants", "[lexer]") {
  JackLexer lexer{TestInput("420 069 23")};
  REQUIRE(lexer.consume() == IntegerConstant(420));
  REQUIRE(lexer.consume() == IntegerConstant(69));
  REQUIRE(lexer.consume() == IntegerConstant(23));
}

TEST_CASE("Handles symbols without whitespace", "[lexer]") {
  JackLexer lexer{TestInput("let x=x+y;")};

  REQUIRE(lexer.consume_as<Keyword>() == Keyword::LET);
  REQUIRE(lexer.consume_as<Identifier>() == Identifier("x"));
  REQUIRE(lexer.consume_as<Symbol>() == Symbol::EQ);
  REQUIRE(lexer.consume_as<Identifier>() == Identifier("x"));
  REQUIRE(lexer.consume_as<Symbol>() == Symbol::PLUS);
  REQUIRE(lexer.consume_as<Identifier>() == Identifier("y"));
  REQUIRE(lexer.consume_as<Symbol>() == Symbol::SEMICOLON);
}

TEST_CASE("Lexer public API", "[lexer]") {
  JackLexer lexer{
      TestInput("class ClassName {\n"
                "  var int value;\n"
                "  function int foo() {\n"
                "    return 10;"
                "  }\n"
                "}")};

  // Expect to get the correct tokens back in FIFO order
  REQUIRE(lexer.tokenType() == Token::KEYWORD);
  REQUIRE(lexer.getKeyword() == Keyword::CLASS);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::IDENTIFIER);
  REQUIRE(lexer.getIdentifier() == "ClassName");

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::L_CURLY);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::KEYWORD);
  REQUIRE(lexer.getKeyword() == Keyword::VAR);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::KEYWORD);
  REQUIRE(lexer.getKeyword() == Keyword::INT);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::IDENTIFIER);
  REQUIRE(lexer.getIdentifier() == "value");

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::SEMICOLON);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::KEYWORD);
  REQUIRE(lexer.getKeyword() == Keyword::FUNCTION);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::KEYWORD);
  REQUIRE(lexer.getKeyword() == Keyword::INT);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::IDENTIFIER);
  REQUIRE(lexer.getIdentifier() == "foo");

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::L_PAREN);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::R_PAREN);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::L_CURLY);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::KEYWORD);
  REQUIRE(lexer.getKeyword() == Keyword::RETURN);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::INTEGER_CONSTANT);
  REQUIRE(lexer.getInt() == 10);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::SEMICOLON);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::R_CURLY);

  lexer.advance();
  REQUIRE(lexer.tokenType() == Token::SYMBOL);
  REQUIRE(lexer.getSymbol() == Symbol::R_CURLY);

  lexer.advance();
  REQUIRE(!lexer.hasMoreTokens());
}
