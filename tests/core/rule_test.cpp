#include "core/rule.h"
#include <gtest/gtest.h>

namespace kbgdb {
namespace {

class RuleTest : public ::testing::Test {
protected:
    Fact createHead() {
        return Fact("grandparent", {
            Term{TermType::VARIABLE, "X"},
            Term{TermType::VARIABLE, "Z"}
        });
    }
    
    std::vector<Fact> createBody() {
        return {
            Fact("parent", {
                Term{TermType::VARIABLE, "X"},
                Term{TermType::VARIABLE, "Y"}
            }),
            Fact("parent", {
                Term{TermType::VARIABLE, "Y"},
                Term{TermType::VARIABLE, "Z"}
            })
        };
    }
};

TEST_F(RuleTest, Construction) {
    Rule rule(createHead(), createBody());
    
    EXPECT_EQ(rule.getHead().getPredicate(), "grandparent");
    ASSERT_EQ(rule.getBody().size(), 2);
    EXPECT_EQ(rule.getBody()[0].getPredicate(), "parent");
    EXPECT_EQ(rule.getBody()[1].getPredicate(), "parent");
}

TEST_F(RuleTest, ToString) {
    Rule rule(createHead(), createBody());
    EXPECT_EQ(rule.toString(), "grandparent(?X, ?Z) :- parent(?X, ?Y), parent(?Y, ?Z).");
}

TEST_F(RuleTest, IsValid) {
    Rule validRule(createHead(), createBody());
    EXPECT_TRUE(validRule.isValid());
    
    // Test invalid rule with empty head predicate
    Rule invalidHead(
        Fact("", {Term{TermType::VARIABLE, "X"}}),
        createBody()
    );
    EXPECT_FALSE(invalidHead.isValid());
    
    // Test invalid rule with empty body
    Rule invalidBody(createHead(), {});
    EXPECT_FALSE(invalidBody.isValid());
}

TEST_F(RuleTest, ComplexRule) {
    // Testing a more complex rule with multiple body goals and mixed terms
    Fact head("can_drive", {
        Term{TermType::VARIABLE, "Person"},
        Term{TermType::CONSTANT, "car"}
    });
    
    std::vector<Fact> body = {
        Fact("person", {Term{TermType::VARIABLE, "Person"}}),
        Fact("age", {
            Term{TermType::VARIABLE, "Person"},
            Term{TermType::VARIABLE, "Age"}
        }),
        Fact("has_license", {Term{TermType::VARIABLE, "Person"}})
    };
    
    Rule rule(head, body);
    EXPECT_TRUE(rule.isValid());
    EXPECT_EQ(rule.toString(), 
        "can_drive(?Person, car) :- person(?Person), age(?Person, ?Age), has_license(?Person).");
}

} // namespace
} // namespace kbgdb
