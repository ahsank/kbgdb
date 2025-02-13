#include "common/fact.h"
#include <gtest/gtest.h>

namespace kbgdb {
namespace {

TEST(TermTest, VariableToString) {
    Term term{TermType::VARIABLE, "X"};
    EXPECT_EQ(term.toString(), "?X");
}

TEST(TermTest, ConstantToString) {
    Term term{TermType::CONSTANT, "john"};
    EXPECT_EQ(term.toString(), "john");
}

TEST(TermTest, NumberToString) {
    Term term{TermType::NUMBER, "42"};
    EXPECT_EQ(term.toString(), "42");
}

TEST(TermTest, IsVariable) {
    Term variable{TermType::VARIABLE, "X"};
    Term constant{TermType::CONSTANT, "john"};
    
    EXPECT_TRUE(variable.isVariable());
    EXPECT_FALSE(constant.isVariable());
}

class FactTest : public ::testing::Test {
protected:
    Fact createSampleFact() {
        return Fact("parent", {
            Term{TermType::CONSTANT, "john"},
            Term{TermType::CONSTANT, "mary"}
        });
    }
};

TEST_F(FactTest, ConstructAndGetPredicate) {
    auto fact = createSampleFact();
    EXPECT_EQ(fact.getPredicate(), "parent");
}

TEST_F(FactTest, GetTerms) {
    auto fact = createSampleFact();
    const auto& terms = fact.getTerms();
    
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::CONSTANT);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST_F(FactTest, ToString) {
    auto fact = createSampleFact();
    EXPECT_EQ(fact.toString(), "parent(john, mary)");
}

TEST_F(FactTest, FactSource) {
    auto fact = createSampleFact();
    EXPECT_EQ(fact.getSource(), FactSource::MEMORY);
    
    fact.setSource(FactSource::ROCKSDB);
    EXPECT_EQ(fact.getSource(), FactSource::ROCKSDB);
}

TEST_F(FactTest, FactWithVariables) {
    Fact fact("parent", {
        Term{TermType::VARIABLE, "X"},
        Term{TermType::CONSTANT, "mary"}
    });
    
    EXPECT_EQ(fact.toString(), "parent(?X, mary)");
}

TEST_F(FactTest, FactWithMixedTerms) {
    Fact fact("age", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::NUMBER, "42"}
    });
    
    EXPECT_EQ(fact.toString(), "age(john, 42)");
}

} // namespace
} // namespace kbgdb
