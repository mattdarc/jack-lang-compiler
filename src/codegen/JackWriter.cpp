#include "JackWriter.hpp"

#include <cassert>

namespace jcc::io {

void JackWriter::writePush(std::string segment, const size_t offset) {
  write("push " + std::move(segment) + " " + std::to_string(offset));
}

void JackWriter::writePop(std::string segment, const size_t offset) {
  write("pop " + std::move(segment) + " " + std::to_string(offset));
}

void JackWriter::writeCall(std::string routine, const size_t nArgs) {
  write("call " + std::move(routine) + " " + std::to_string(nArgs));
}

void JackWriter::writeLabel(std::string labelName) {
  write("label " + std::move(labelName));
}

void JackWriter::writeGoto(std::string labelName) {
  write("goto " + std::move(labelName));
}

void JackWriter::writeIf(std::string labelName) {
  write("if-goto " + std::move(labelName));
}

void JackWriter::writeFunction(std::string functionName, const size_t nLocals) {
  write("function " + std::move(functionName) + " " + std::to_string(nLocals));
}

void JackWriter::writeUnaryOp(const char op) {
  switch (op) {
    case '-':
      write("neg");
      break;
    case '~':
      write("not");
      break;
    default:
      assert(false);
  }
}

void JackWriter::writeBinOp(const char op) {
  switch (op) {
    case '+':
      write("add");
      break;
    case '-':
      write("sub");
      break;
    case '&':
      write("and");
      break;
    case '|':
      write("or");
      break;
    case '<':
      write("lt");
      break;
    case '>':
      write("gt");
      break;
    case '=':
      write("eq");
      break;
    case '/':
      writeCall("Math.divide", 2);
      break;
    case '*':
      writeCall("Math.multiply", 2);
      break;
    default:
      assert(false);
  }
}

void JackWriter::write(const std::string &toWrite) {
  (*m_ostream) << toWrite << '\n';
}

}  // namespace jcc::io
