#include "query/query_parser.h"
#include <gtest/gtest.h>

namespace kbgdb {
namespace {

class QueryParserTest : public ::testing::Test {
protected:
    QueryParser parser;
};

TEST_F(QueryParserTest, SimpleQueryWithConstants) {
    Fact fact = parser.parse("parent(john, mary)");
    
    EXPECT_EQ(fact.predicate(), "parent");
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::CONSTANT);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, QueryWithVariables) {
    parser.setRuleMode(false);  // Query mode: ?X
    Fact fact = parser.parse("parent(?X, mary)");

    EXPECT_EQ(fact.predicate(), "parent");
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::VARIABLE);
    EXPECT_EQ(terms[0].value, "X");  // Note: '?' is stripped
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, RuleWithUppercaseVariables) {
    parser.setRuleMode(true);  // Rule mode: X
    Fact fact = parser.parse("parent(X, mary)");

    EXPECT_EQ(fact.predicate(), "parent");
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::VARIABLE);
    EXPECT_EQ(terms[0].value, "X");
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, RuleWithUnderscoreVariable) {
    parser.setRuleMode(true);
    Fact fact = parser.parse("parent(_X, _)");
    
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::VARIABLE);
    EXPECT_EQ(terms[0].value, "_X");
    EXPECT_EQ(terms[1].type, TermType::VARIABLE);
    EXPECT_EQ(terms[1].value, "_");
}

TEST_F(QueryParserTest, QueryWithNumbers) {
    Fact fact = parser.parse("age(john, 42)");

    EXPECT_EQ(fact.predicate(), "age");
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::CONSTANT);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].type, TermType::NUMBER);
    EXPECT_EQ(terms[1].value, "42");
}

TEST_F(QueryParserTest, QueryWithWhitespace) {
    Fact fact = parser.parse("  parent  (  john  ,  mary  )  ");

    EXPECT_EQ(fact.predicate(), "parent");
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(QueryParserTest, InvalidQueries) {
    EXPECT_THROW(parser.parse("parent("), std::runtime_error);
    EXPECT_THROW(parser.parse("parent)"), std::runtime_error);
    EXPECT_THROW(parser.parse("parent(,mary)"), std::runtime_error);
}

TEST_F(QueryParserTest, MultipleVariables) {
    parser.setRuleMode(false);
    Fact fact = parser.parse("grandparent(?X, ?Y, ?Z)");
    
    EXPECT_EQ(fact.predicate(), "grandparent");
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 3);
    EXPECT_TRUE(terms[0].isVariable());
    EXPECT_TRUE(terms[1].isVariable());
    EXPECT_TRUE(terms[2].isVariable());
}

} // namespace
} // namespace kbgdb
