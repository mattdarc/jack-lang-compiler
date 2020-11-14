#include "JackLexer.hpp"

namespace jcc {

std::string Token::kindToString(Token::Kind k) {
  switch (k) {
    default:
      assert(false && "Invalid token");
      return "";
    case Token::Kind::KEYWORD:
      return "Keyword";
    case Token::Kind::SYMBOL:
      return "Keyword";
    case Token::Kind::INTEGER_CONSTANT:
      return "IntegerConstant";
    case Token::Kind::STRING_CONSTANT:
      return "StringConstant";
    case Token::Kind::IDENTIFIER:
      return "Identifier";
  }
}

std::ostream &operator<<(std::ostream &os, Token::Kind k) {
  return os << Token::kindToString(k);
}

std::ostream &operator<<(std::ostream &os, Keyword::Type type) {
  return os << Keyword::toString(type);
}

Token JackLexer::parse() {
  auto eof = [&] { return m_istream->eof(); };
  auto peek = [&] { return m_istream->peek(); };
  auto eat = [&] {
    auto c = m_istream->get();
    if (c == '\n') {
      ++m_lineNum;
      m_colNum = 1;
    } else if (c == '\t') {
      m_colNum += 2;
    } else {
      ++m_colNum;
    }
    return c;
  };
ReParse:

  if (eof()) return Token();

  // Skip whitespace
  while (std::isspace(peek())) {
    if (eof()) return Token();
    eat();
  }

  if (peek() == '/') {
    auto c = eat();
    if (peek() == '/') {
      // Single line comment
      while (eat() != '\n')
        if (eof()) return Token();
    } else if (peek() == '*') {
      // Multiline comment
      while (peek() != '/' || c != '*') {
        if (eof()) return Token();
        c = eat();
      }
      eat();
    } else {
      // Division
      return Symbol::fromChar(c);
    }
    goto ReParse;
  }

  // Parse string
  if (peek() == '"') {
    eat();
    std::string strconst;
    while (peek() != '"') {
      if (eof()) return Token();
      strconst.push_back(eat());
    }
    eat();
    return StringConstant(strconst);
  }

  // Detect a symbol
  if (const auto symbol = Symbol::fromChar(peek())) {
    eat();
    return symbol;
  }

  // Parse a string or digit
  std::string token;
  while (!std::isspace(peek()) && !Symbol::fromChar(peek())) {
    auto c = eat();
    if (c == ';' || eof()) break;
    token.push_back(c);
  }

  // Check for digits
  if (std::isdigit(token[0])) { return IntegerConstant(std::stol(token)); }

  // Check for a keyword
  if (const auto keyword = Keyword::fromString(token)) { return keyword; }

  return !token.empty() ? Identifier(token) : Token();
}

}  // namespace jcc
