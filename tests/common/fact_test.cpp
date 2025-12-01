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

TEST(FactTest, ConstructAndGetPredicate) {
    Fact fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "mary"}
    });
    EXPECT_EQ(fact.predicate(), "parent");
}

TEST(FactTest, GetTerms) {
    Fact fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "mary"}
    });
    
    const auto& terms = fact.terms();
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0].type, TermType::CONSTANT);
    EXPECT_EQ(terms[0].value, "john");
    EXPECT_EQ(terms[1].type, TermType::CONSTANT);
    EXPECT_EQ(terms[1].value, "mary");
}

TEST(FactTest, ToString) {
    Fact fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "mary"}
    });
    EXPECT_EQ(fact.toString(), "parent(john, mary)");
}

TEST(FactTest, FactWithVariables) {
    Fact fact("parent", {
        Term{TermType::VARIABLE, "X"},
        Term{TermType::CONSTANT, "mary"}
    });
    EXPECT_EQ(fact.toString(), "parent(?X, mary)");
}

TEST(BindingSetTest, AddAndGet) {
    BindingSet bindings;
    bindings.add("X", "john");
    bindings.add("Y", "mary");
    
    EXPECT_EQ(bindings.get("X"), "john");
    EXPECT_EQ(bindings.get("Y"), "mary");
    EXPECT_EQ(bindings.get("Z"), "");  // Not found
}

TEST(BindingSetTest, Has) {
    BindingSet bindings;
    bindings.add("X", "john");
    
    EXPECT_TRUE(bindings.has("X"));
    EXPECT_FALSE(bindings.has("Y"));
}

TEST(BindingSetTest, ToString) {
    BindingSet bindings;
    bindings.add("X", "john");
    
    std::string str = bindings.toString();
    EXPECT_TRUE(str.find("X=john") != std::string::npos);
}

} // namespace
} // namespace kbgdb
