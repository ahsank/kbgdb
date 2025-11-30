#include "query/query_parser.h"
#include <gtest/gtest.h>

namespace kbgdb {
namespace {

class QueryParserTest : public ::testing::Test {
protected:
    QueryParser parser;
};

TEST_F(QueryParserTest, SimpleQuery) {
    Fact fact = parser.parse("parent(john, mary)");
    
    EXPECT_EQ(fact.predicate_, "parent");
    const auto& terms = fact.terms_;
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::CONSTANT);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, QueryWithVariables) {
    parser.setRuleMode(false); // Query mode
    Fact fact = parser.parse("parent(?X, mary)");

    EXPECT_EQ(fact.predicate_, "parent");
    const auto& terms = fact.terms_;
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::VARIABLE);
    EXPECT_EQ(terms[0].value, "X");
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, RuleWithVariables) {
    parser.setRuleMode(true); // Rule mode
    Fact fact = parser.parse("parent(X, mary)");

    EXPECT_EQ(fact.predicate_, "parent");
    const auto& terms = fact.terms_;
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::VARIABLE);
    EXPECT_EQ(terms[0].value, "X");
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, QueryWithNumbers) {
    Fact fact = parser.parse("age(john, 42)");

    EXPECT_EQ(fact.predicate_, "age");
    const auto& terms = fact.terms_;
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::CONSTANT);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].type, TermType::NUMBER);
    EXPECT_EQ(terms[1].value, "42");
}

TEST_F(QueryParserTest, QueryWithWhitespace) {
    Fact fact = parser.parse("  parent  (  john  ,  mary  )  ");

    EXPECT_EQ(fact.predicate_, "parent");
    const auto& terms = fact.terms_;
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, InvalidQueries) {
    EXPECT_THROW(parser.parse("parent("), std::runtime_error);
    EXPECT_THROW(parser.parse("parent)"), std::runtime_error);
    // EXPECT_THROW(parser.parse("parent(john,)"), std::runtime_error);
    EXPECT_THROW(parser.parse("parent(,mary)"), std::runtime_error);
    EXPECT_THROW(parser.parse("parent((john,mary)"), std::runtime_error);
}

TEST_F(QueryParserTest, UnderscoreVariable) {
    parser.setRuleMode(true);
    Fact fact = parser.parse("parent(_X, _)");
    
    const auto& terms = fact.terms_;
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::VARIABLE);
    EXPECT_EQ(terms[0].value, "_X");
    EXPECT_EQ(terms[1].type, TermType::VARIABLE);
    EXPECT_EQ(terms[1].value, "_");
}

} // namespace
} // namespace kbgdb
