// #define CATCH_CONFIG_RUNNER
#include "JackAST.hpp"
#include "SymbolTable.hpp"
#include "catch2/catch.hpp"

using namespace jcc::sym;

/* Public API */
TEST_CASE("addEntry creates a new symbol in a table", "[basic]") {
  auto table = std::make_unique<Table>("TopLevelClass");

  std::string theVar{"someVar"};
  std::string theType{"aType"};

  auto var = std::make_unique<jcc::ast::VarDecl>(theVar, theType);

  auto success = table->addValue(var.get());
  CHECK(success);

  const auto *ent = table->lookup(theVar);
  REQUIRE(ent != nullptr);
  CHECK(ent->getType() == theType);
  CHECK(ent->getName() == theVar);

  auto dup = std::make_unique<jcc::ast::VarDecl>(theVar, theType);
  success = table->addValue(dup.get());
  CHECK(!success);
}
