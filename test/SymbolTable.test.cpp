// #define CATCH_CONFIG_RUNNER
#include "JackAST.hpp"
#include "SymbolTable.hpp"
#include "gtest/gtest.h"

using namespace jcc::sym;

/* Public API */
TEST(SymbolTableTest, Basic) {
  auto table = std::make_unique<Table>("TopLevelClass");

  std::string theVar{"someVar"};
  std::string theType{"aType"};

  auto var = std::make_unique<jcc::ast::VarDecl>(theVar, theType);

  auto success = table->addValue(var.get());
  ASSERT_TRUE(success);

  const auto *ent = table->lookup(theVar);
  EXPECT_NE(ent, nullptr);
  EXPECT_EQ(ent->getType(), theType);
  EXPECT_EQ(ent->getName(), theVar);

  auto dup = std::make_unique<jcc::ast::VarDecl>(theVar, theType);
  success = table->addValue(dup.get());
  ASSERT_FALSE(success);
}
