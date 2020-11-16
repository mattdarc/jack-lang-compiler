// #define CATCH_CONFIG_RUNNER
#include <sstream>

#include "gtest/gtest.h"

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

TEST(JackLexerTest, StringConstants) {
  JackLexer lexer(TestInput("\"StringConstant\""));
  EXPECT_EQ(lexer.consume(), StringConstant("StringConstant"));
  EXPECT_FALSE(lexer.hasMoreTokens());

  lexer(TestInput("\"String\" \"Constant\""));
  EXPECT_EQ(lexer.consume(), StringConstant("String"));
  EXPECT_EQ(lexer.consume(), StringConstant("Constant"));
  EXPECT_FALSE(lexer.hasMoreTokens());
}

TEST(JackLexerTest, TrailingWhitespace) {
  JackLexer lexer{TestInput("  identifier \n \t")};
  EXPECT_EQ(lexer.consume(), Identifier("identifier"));
  EXPECT_FALSE(lexer.hasMoreTokens());

  lexer(TestInput("  identifier1 \t identifier2 \n \t"));
  EXPECT_EQ(lexer.consume(), Identifier("identifier1"));
  EXPECT_EQ(lexer.consume(), Identifier("identifier2"));
  EXPECT_FALSE(lexer.hasMoreTokens());
}

TEST(JackLexerTest, NonAlphabetic) {
  JackLexer lexer{TestInput("\"String Constant\"")};
  EXPECT_EQ(lexer.consume(), StringConstant("String Constant"));
  EXPECT_FALSE(lexer.hasMoreTokens());

  lexer(TestInput(
      "\"String Constant, with a class keyword and the number 420\""));
  EXPECT_EQ(lexer.consume(),
          StringConstant(
              "String Constant, with a class keyword and the number 420"));
  EXPECT_FALSE(lexer.hasMoreTokens());
}

TEST(JackLexerTest, Comments) {
  JackLexer lexer{
      TestInput("// Some misc words that should not be processed\n")};
  EXPECT_EQ(lexer.consume(), Token());
  EXPECT_FALSE(lexer.hasMoreTokens());

  lexer(TestInput("/* Some misc words that should not be processed */\n"));
  EXPECT_EQ(lexer.consume(), Token());
  EXPECT_FALSE(lexer.hasMoreTokens());

  lexer(TestInput(
      "/* Some misc \n words that \n should * / not be processed */\n"));
  EXPECT_EQ(lexer.consume(), Token());
  std::cerr << "token = " << lexer.peek();
  EXPECT_FALSE(lexer.hasMoreTokens());

  lexer(
      TestInput("// Some misc \n /// words // that should not be processed\n"));
  EXPECT_EQ(lexer.consume(), Token());
  EXPECT_FALSE(lexer.hasMoreTokens());
}

TEST(JackLexerTest, Symbols) {
  JackLexer lexer(TestInput(",+-"));
  EXPECT_EQ(lexer.consume(), Symbol(Symbol::COMMA));
  EXPECT_EQ(lexer.consume(), Symbol(Symbol::PLUS));
  EXPECT_EQ(lexer.consume(), Symbol(Symbol::MINUS));
  std::cerr << "token = " << lexer.peek();
  EXPECT_FALSE(lexer.hasMoreTokens());
}

TEST(JackLexerTest, Keywords) {
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

  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::CLASS);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::CONSTRUCTOR);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::FUNCTION);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::METHOD);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::FIELD);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::STATIC);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::VAR);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::INT);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::CHAR);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::BOOLEAN);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::VOID);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::TRUE);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::FALSE);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::NIL);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::THIS);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::LET);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::DO);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::IF);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::ELSE);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::WHILE);
  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::RETURN);
  std::cerr << "token = " << lexer.peek();
  EXPECT_FALSE(lexer.hasMoreTokens());
}

TEST(JackLexerTest, IntegerConstants) {
  JackLexer lexer{TestInput("420 069 23")};
  EXPECT_EQ(lexer.consume(), IntegerConstant(420));
  EXPECT_EQ(lexer.consume(), IntegerConstant(69));
  EXPECT_EQ(lexer.consume(), IntegerConstant(23));
}

TEST(JackLexerTest, PackedSymbols) {
  JackLexer lexer{TestInput("let x=x+y;")};

  EXPECT_EQ(lexer.consume_as<Keyword>(), Keyword::LET);
  EXPECT_EQ(lexer.consume_as<Identifier>(), Identifier("x"));
  EXPECT_EQ(lexer.consume_as<Symbol>(), Symbol::EQ);
  EXPECT_EQ(lexer.consume_as<Identifier>(), Identifier("x"));
  EXPECT_EQ(lexer.consume_as<Symbol>(), Symbol::PLUS);
  EXPECT_EQ(lexer.consume_as<Identifier>(), Identifier("y"));
  EXPECT_EQ(lexer.consume_as<Symbol>(), Symbol::SEMICOLON);
}

TEST(JackLexerTest, PublicAPI) {
  JackLexer lexer{
      TestInput("class ClassName {\n"
                "  var int value;\n"
                "  function int foo() {\n"
                "    return 10;"
                "  }\n"
                "}")};

  // Expect to get the correct tokens back in FIFO order
  EXPECT_EQ(lexer.tokenType(), Token::KEYWORD);
  EXPECT_EQ(lexer.getKeyword(), Keyword::CLASS);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::IDENTIFIER);
  EXPECT_EQ(lexer.getIdentifier(), "ClassName");

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::L_CURLY);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::KEYWORD);
  EXPECT_EQ(lexer.getKeyword(), Keyword::VAR);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::KEYWORD);
  EXPECT_EQ(lexer.getKeyword(), Keyword::INT);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::IDENTIFIER);
  EXPECT_EQ(lexer.getIdentifier(), "value");

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::SEMICOLON);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::KEYWORD);
  EXPECT_EQ(lexer.getKeyword(), Keyword::FUNCTION);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::KEYWORD);
  EXPECT_EQ(lexer.getKeyword(), Keyword::INT);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::IDENTIFIER);
  EXPECT_EQ(lexer.getIdentifier(), "foo");

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::L_PAREN);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::R_PAREN);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::L_CURLY);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::KEYWORD);
  EXPECT_EQ(lexer.getKeyword(), Keyword::RETURN);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::INTEGER_CONSTANT);
  EXPECT_EQ(lexer.getInt(), 10);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::SEMICOLON);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::R_CURLY);

  lexer.advance();
  EXPECT_EQ(lexer.tokenType(), Token::SYMBOL);
  EXPECT_EQ(lexer.getSymbol(), Symbol::R_CURLY);

  lexer.advance();
  EXPECT_FALSE(lexer.hasMoreTokens());
}
