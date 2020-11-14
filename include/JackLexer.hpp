#ifndef jcc_JackLexer_hpp
#define jcc_JackLexer_hpp

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <variant>

// undef symbols defined in boolean.h on mac
#if __APPLE__
#undef TRUE
#undef FALSE
#endif

using InputStream = std::unique_ptr<std::basic_istream<char>>;

namespace jcc {

struct Keyword {
  enum Type {
    CLASS = 0,
    CONSTRUCTOR,
    FUNCTION,
    METHOD,
    FIELD,
    STATIC,
    VAR,
    INT,
    CHAR,
    BOOLEAN,
    VOID,
    TRUE,
    FALSE,
    NIL,
    THIS,
    LET,
    DO,
    IF,
    ELSE,
    WHILE,
    RETURN,
    UNKNOWN
  };

  constexpr static const char *const Strings[] = {
      "class", "constructor", "function", "method",  "field", "static",
      "var",   "int",         "char",     "boolean", "void",  "true",
      "false", "null",        "this",     "let",     "do",    "if",
      "else",  "while",       "return",   "UNKNOWN"};

  Keyword() : Keyword{UNKNOWN} {}
  Keyword(Type type) : m_type{type} {}
  explicit operator bool() const { return m_type != UNKNOWN; }
  Type m_type;

  // Function to convert a string to a keyword type
  static Keyword fromString(const std::string &str) {
    return Keyword(static_cast<Type>(std::distance(
        std::begin(Strings),
        std::find(std::begin(Strings), std::end(Strings) - 1, str))));
  }

  static std::string toString(Keyword k) {
    return Strings[static_cast<size_t>(k.m_type)];
  }
};
inline bool operator==(const Keyword &lhs, const Keyword &rhs) {
  return lhs.m_type == rhs.m_type;
}

// Create an identifier token. Accepts a single string
// as an arguments
struct Identifier {
  Identifier() : Identifier{""} {}
  explicit Identifier(std::string str) : m_identifier{std::move(str)} {}
  std::string m_identifier;
};
inline bool operator==(const Identifier &lhs, const Identifier &rhs) {
  return lhs.m_identifier == rhs.m_identifier;
}

// Create an integer constant token. Accepts a single
// unsigned long as an argument.
struct IntegerConstant {
  IntegerConstant() : IntegerConstant{0} {}
  explicit IntegerConstant(long i) : m_integer{i} {}
  long m_integer;
};
inline bool operator==(const IntegerConstant lhs, const IntegerConstant rhs) {
  return lhs.m_integer == rhs.m_integer;
}

// Create a symbol token. Accepts a single character
// as the symbol
struct Symbol {
  enum Type {
    PLUS = 0,
    MINUS,
    MUL,
    DIV,
    EQ,
    LT,
    GT,
    L_PAREN,
    R_PAREN,
    L_BRACKET,
    R_BRACKET,
    L_CURLY,
    R_CURLY,
    AND,
    OR,
    SEMICOLON,
    COMMA,
    NOT,
    PERIOD,
    UNKNOWN
  };

  constexpr static char Chars[]{'+', '-', '*', '/', '=', '<', '>',
                                '(', ')', '[', ']', '{', '}', '&',
                                '|', ';', ',', '~', '.', '\0'};
  Symbol() : Symbol{UNKNOWN} {}
  Symbol(Type symbol) : m_symbol{symbol} {}

  // temporary until we can move out the instantiations
  explicit Symbol(char c) : m_symbol{fromChar(c).m_symbol} {}
  explicit operator bool() const { return m_symbol != UNKNOWN; }

  static Symbol fromChar(char c) {
    return Symbol(static_cast<Type>(
        std::distance(std::begin(Chars),
                      std::find(std::begin(Chars), std::end(Chars) - 1, c))));
  }

  static char toChar(Symbol s) {
    return Chars[static_cast<size_t>(s.m_symbol)];
  }

  static std::string toString(Symbol s) { return std::string(1, toChar(s)); }

  Type m_symbol;
};

inline bool operator==(const Symbol lhs, const Symbol rhs) {
  return lhs.m_symbol == rhs.m_symbol;
}

// Create a string constant token. Accepts the un-quoted
// string as an argument.
struct StringConstant {
  StringConstant() : StringConstant(std::string{}) {}
  explicit StringConstant(std::string str) : m_string{std::move(str)} {}
  std::string m_string;
};
inline bool operator==(const StringConstant &lhs, const StringConstant &rhs) {
  return lhs.m_string == rhs.m_string;
}

class Token {
public:
  enum Kind {
    KEYWORD = 0,
    SYMBOL,
    INTEGER_CONSTANT,
    STRING_CONSTANT,
    IDENTIFIER,
    EMPTY
  };

  static std::string kindToString(Kind k);

  Token(StringConstant tok) : m_token{tok} {}
  Token(Keyword tok) : m_token{tok} {}
  Token(Symbol tok) : m_token{tok} {}
  Token(IntegerConstant tok) : m_token{tok} {}
  Token(Identifier tok) : m_token{tok} {}
  Token(std::monostate) : m_token{std::monostate{}} {}
  Token() : Token(std::monostate{}) {}

  inline std::string print() const;
  inline Kind getKind() const;

  template <typename T>
  T get() const {
    return std::get<T>(m_token);
  }

  bool isNull() const {
    return std::holds_alternative<std::monostate>(m_token);
  }

private:
  std::variant<std::monostate, Keyword, Identifier, StringConstant,
               IntegerConstant, Symbol>
      m_token;
  friend inline bool operator==(const Token &, const Token &);
};

inline bool operator==(const Token &lhs, const Token &rhs) {
  return lhs.m_token == rhs.m_token;
}
inline bool operator!=(const Token &lhs, const Token &rhs) {
  return !(lhs == rhs);
}

class JackLexer {
public:
  constexpr static size_t buffer_size = 512;
  JackLexer() : JackLexer{InputStream{}} {}
  explicit JackLexer(InputStream input)
      : m_istream{std::move(input)},
        m_colNum{1},
        m_lineNum{1},
        m_tok{parse()} {}

  void operator()(InputStream input) {
    m_istream = std::move(input);
    advance();
  }

  bool hasMoreTokens() const { return !m_istream->eof() || !m_tok.isNull(); }

  // Advance the current token
  void advance() {
    if (hasMoreTokens()) {
      m_tok = parse();
    } else {
      m_tok = Token();
    }
  }

  // Return the kind of token
  Token::Kind tokenType() const { return m_tok.getKind(); }

  // Get the type of keyword
  Keyword::Type getKeyword() const { return m_tok.get<Keyword>().m_type; }

  // Get the symbol from the token
  Symbol::Type getSymbol() const { return m_tok.get<Symbol>().m_symbol; }

  // Get the identifier string from the token
  std::string getIdentifier() const {
    return m_tok.get<Identifier>().m_identifier;
  }

  // Get the string constant from the token
  std::string getString() const { return m_tok.get<StringConstant>().m_string; }

  // Get the integer constant from the token
  long getInt() const { return m_tok.get<IntegerConstant>().m_integer; }

  // Get a handle to the current token
  const Token &peek() const { return m_tok; }
  Token consume() {
    auto tok = peek();
    advance();
    return tok;
  }

  template <typename T>
  T consume_as() {
    auto tok = peek();
    advance();
    return tok.get<T>();
  }

  // Return the current line number of the source file
  unsigned getLineNumber() const { return m_lineNum; }

  // Return the current column number of the source file
  unsigned getColNumber() const { return m_colNum; }

private:
  InputStream m_istream;
  unsigned m_colNum;
  unsigned m_lineNum;
  Token m_tok;

  Token parse();
};

namespace operations {
struct GetKind {
  Token::Kind operator()(const Keyword &) { return Token::Kind::KEYWORD; }
  Token::Kind operator()(const StringConstant &) {
    return Token::Kind::STRING_CONSTANT;
  }
  Token::Kind operator()(const Symbol &) { return Token::Kind::SYMBOL; }
  Token::Kind operator()(const IntegerConstant &) {
    return Token::Kind::INTEGER_CONSTANT;
  }
  Token::Kind operator()(const Identifier &) { return Token::Kind::IDENTIFIER; }
  Token::Kind operator()(const std::monostate &) { return Token::Kind::EMPTY; }
};

struct Printer {
  std::string operator()(const Keyword &k) {
    return "Keyword: " + Keyword::toString(k);
  }
  std::string operator()(const StringConstant &s) {
    return "StringConstant: " + s.m_string;
  }
  std::string operator()(const Symbol &s) {
    return "Symbol: " + Symbol::toString(s);
  }
  std::string operator()(const IntegerConstant &i) {
    return "IntegerConstant: " + std::to_string(i.m_integer);
  }
  std::string operator()(const Identifier &i) {
    return "Identifier: " + i.m_identifier;
  }
  std::string operator()(const std::monostate &) { return "INVALID"; }
};
}  // namespace operations

std::string Token::print() const {
  return std::visit(operations::Printer(), m_token);
}

Token::Kind Token::getKind() const {
  return std::visit(operations::GetKind(), m_token);
}

inline std::ostream &operator<<(std::ostream &os, const Token &tok) {
  return os << tok.print();
}

}  // namespace jcc

namespace std {
template <>
struct numeric_limits<jcc::Symbol> {
  constexpr static auto max() {
    using namespace jcc;
    return static_cast<std::underlying_type_t<Symbol::Type>>(Symbol::UNKNOWN);
  }

  constexpr static auto min() {
    using namespace jcc;
    return static_cast<std::underlying_type_t<Symbol::Type>>(Symbol::PLUS);
  }
};
}  // namespace std

#endif
