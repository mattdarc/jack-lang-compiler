#ifndef jcc_ErrorHandling_hpp
#define jcc_ErrorHandling_hpp

#include <exception>
#include <string>
#include <variant>

namespace jcc {

class SyntaxError : virtual public std::exception {
public:
  SyntaxError(const std::string &file, unsigned column, unsigned line,
              const std::string &msg)
      : m_filename{file},
        m_colNumber{column},
        m_lineNumber{line},
        m_msg{"[\033[31;1merror\033[0m: " + file + ": " + std::to_string(line) +
              ":" + std::to_string(column) + "] " + msg} {}

  virtual const char *what() const noexcept override { return m_msg.c_str(); }
  unsigned getLine() const { return m_lineNumber; }
  unsigned getCol() const { return m_colNumber; }
  const std::string &getFile() const { return m_filename; }

private:
  const std::string m_filename;
  const unsigned m_colNumber;
  const unsigned m_lineNumber;
  const std::string m_msg;
};

struct Error {
  explicit Error(std::string msg) : m_msg{std::move(msg)} {}
  const char *message() const { return m_msg.c_str(); }

private:
  std::string m_msg;
};

template <typename T>
class Result {
public:
  Result(T t) : m_result{t} {}
  template <typename U>
  Result(U &&e) : m_result{Error(std::forward<U>(e))} {}
  Result<T> &operator=(T &&t) {
    m_result = std::move(t);
    return *this;
  }
  bool hasError() const { return m_result.index() == 1; }
  void reportError() const {
    printf("%s\n", std::get<Error>(m_result).message());
  }
  operator T &() {
    assert(m_result.index() == 0);
    return std::get<T>(m_result);
  }

  template <typename U>
  friend bool operator==(const Result<U> &lhs, const Result<U> &rhs);

private:
  std::variant<T, Error> m_result;
};

template <typename T>
bool operator==(const Result<T> &lhs, const Result<T> &rhs) {
  return lhs == rhs;
}

}  // namespace jcc

#endif /* jcc_ErrorHandling_hpp */
