#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include <cstdio>
#include <fstream>
#include <future>
#include <string>
#include <thread>

#include "CompilationEngine.hpp"
#include "ErrorHandling.hpp"
#include "JackAST.hpp"
#include "JackJIT.hpp"
#include "LLVMGenerator.hpp"
#include "PrettyPrinter.hpp"
#include "Runtime.hpp"

namespace {
using namespace jcc;
enum class PathType { File, Directory, Unknown };

Result<PathType> getPathType(const std::string &path) {
  struct stat s {};
  if (stat(path.c_str(), &s) == 0) {
    if (s.st_mode & S_IFREG) {
      return PathType::File;
    } else if (s.st_mode & S_IFDIR) {
      return PathType::Directory;
    } else {
      return PathType::Unknown;
    }
  }
  return Error("Error stat'ing path");
}

std::vector<std::string> getDirFiles(const std::string &path) {
  std::vector<std::string> files;
  DIR *dir = nullptr;
  if ((dir = opendir(path.c_str())) != nullptr) {
    struct dirent *ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
      if (getPathType(path + "/" + ent->d_name) == PathType::File) {  // NOLINT
        files.emplace_back(ent->d_name);
      }
    }
  }
  return files;
}

using ast::NodePtr;

static NodePtr compileFile(const std::string &file) {
  printf("Compiling file %s ...\n", file.c_str());
  auto in = std::make_unique<std::fstream>(file.c_str());

  jcc::CompilationEngine compEngine{std::move(in), std::string(file)};
  return compEngine.compileClass();
}

}  // namespace

int main(int argc, char *argv[]) {
  using namespace jcc;
  std::vector<std::string> inputs;
  if (argc < 2) {
    printf("Expected using: jcc file1.jack [file2.jack ...]");
    printf("\n\t\tjcc directory\n");
    exit(1);
  } else {
    std::generate_n(std::back_inserter(inputs), argc - 1,
                    [&, i = 1]() mutable { return argv[i++]; });  // NOLINT
  }

  jcc::Runtime::instance().reset();

  std::vector<std::future<Result<NodePtr>>> futs;
  std::vector<std::string> fileList;
  for (const auto &input : inputs) {
    Result<PathType> type = getPathType(input);
    if (type.hasError()) {
      type.reportError();
      exit(1);
    } else if (type == PathType::File) {
      jcc::Runtime::instance().addAST(compileFile(input));
    } else if (type == PathType::Directory) {
      printf("Compiling directory %s ...\n", input.c_str());
      fileList = getDirFiles(input);

      auto compileSingle = [](const std::string &fullname) -> Result<NodePtr> {
        Result<NodePtr> result{Error("Uninitialized result")};
        try {
          assert(getPathType(fullname) == PathType::File);
          result = compileFile(fullname);
        } catch (const jcc::SyntaxError &err) {
          result = Error(err.what());
        } catch (const std::exception &ex) {
          result = Error(std::string("Caught Exception: ") + ex.what());
        }
        return result;
      };

      for (const auto &file : fileList) {
        if (file.find(".jack") != std::string::npos) {
          futs.push_back(std::async(std::launch::async, compileSingle,
                                    input + "/" + file));
        }
      }
    } else if (type == PathType::Unknown) {
      printf("Unknown path %s\n", input.c_str());
      exit(1);
    }
  }

  // auto resultItr = futs.begin();
  bool hadError = false;
  for (auto &fut : futs) {
    // Collect ASTs
    auto result = fut.get();
    if (!result.hasError()) {
      NodePtr &ast = result;
      jcc::Runtime::instance().addAST(std::move(ast));
    } else {
      result.reportError();
      hadError = true;
    }
  }

  if (!hadError) {
    // Generate code
    jcc::Runtime::instance().codegen();

    // JIT
    printf("Running Main.main ...\n");
    return jcc::Runtime::instance().run();
  }

  return 1;
}
