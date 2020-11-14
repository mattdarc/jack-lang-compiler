#ifndef JACKWRITER_HPP
#define JACKWRITER_HPP

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace jcc::io {

using OutputStream = std::unique_ptr<std::basic_ostream<char>>;

class JackWriter {
public:
  JackWriter(OutputStream out) : m_ostream{std::move(out)} {}
  JackWriter() : JackWriter{OutputStream{}} {}

  JackWriter(const JackWriter &) = delete;
  JackWriter &operator=(const JackWriter &) = delete;
  JackWriter(JackWriter &&) = default;
  JackWriter &operator=(JackWriter &&) = default;

  void writePush(std::string segment, const size_t offset);
  void writePop(std::string segment, const size_t offset);
  void writeCall(std::string routine, const size_t nArgs);
  void writeLabel(std::string labelName);
  void writeGoto(std::string labelName);
  void writeIf(std::string labelName);
  void writeFunction(std::string functionName, const size_t nLocals);
  void writeReturn() { write("return"); }
  void writeUnaryOp(const char op);
  void writeBinOp(const char op);
  const OutputStream &getOutput() const { return m_ostream; }

private:
  void write(const std::string &toWrite);

  OutputStream m_ostream;
};

}  // namespace jcc::io

#endif
