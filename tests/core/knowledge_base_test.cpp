#include "core/knowledge_base.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>

namespace kbgdb {
namespace {

class KnowledgeBaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        kb = std::make_unique<KnowledgeBase>();
        testFile = std::filesystem::temp_directory_path() / "kb_test.pl";
    }

    void TearDown() override {
        if (std::filesystem::exists(testFile)) {
            std::filesystem::remove(testFile);
        }
    }

    void writeTestFile(const std::string& content) {
        std::ofstream file(testFile);
        file << content;
        file.close();
    }

    std::unique_ptr<KnowledgeBase> kb;
    std::filesystem::path testFile;
};

// ============================================================================
// Basic Fact Tests
// ============================================================================

TEST_F(KnowledgeBaseTest, AddAndRetrieveFact) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    
    auto facts = kb->getFacts("person");
    ASSERT_EQ(facts.size(), 1);
    EXPECT_EQ(facts[0].toString(), "person(john)");
}

TEST_F(KnowledgeBaseTest, AddMultipleFacts) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "mary"}}));
    kb->addFact(Fact("age", {
        Term{TermType::CONSTANT, "john"}, 
        Term{TermType::NUMBER, "30"}
    }));
    
    auto personFacts = kb->getFacts("person");
    ASSERT_EQ(personFacts.size(), 2);
    
    auto ageFacts = kb->getFacts("age");
    ASSERT_EQ(ageFacts.size(), 1);
    EXPECT_EQ(ageFacts[0].toString(), "age(john, 30)");
}

// ============================================================================
// Simple Query Tests (Synchronous!)
// ============================================================================

TEST_F(KnowledgeBaseTest, SimpleQuerySingleResult) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    
    auto results = kb->query("person(?X)");
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get("X"), "john");
}

TEST_F(KnowledgeBaseTest, SimpleQueryMultipleResults) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "mary"}}));
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "bob"}}));
    
    auto results = kb->query("person(?X)");
    
    ASSERT_EQ(results.size(), 3);
    
    std::vector<std::string> names;
    for (const auto& binding : results) {
        names.push_back(binding.get("X"));
    }
    EXPECT_THAT(names, ::testing::UnorderedElementsAre("john", "mary", "bob"));
}

TEST_F(KnowledgeBaseTest, QueryWithBoundArgument) {
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "bob"}
    }));
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "mary"}
    }));
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "alice"},
        Term{TermType::CONSTANT, "tom"}
    }));
    
    // Query for children of john
    auto results = kb->query("parent(john, ?Child)");
    
    ASSERT_EQ(results.size(), 2);
    std::vector<std::string> children;
    for (const auto& binding : results) {
        children.push_back(binding.get("Child"));
    }
    EXPECT_THAT(children, ::testing::UnorderedElementsAre("bob", "mary"));
}

TEST_F(KnowledgeBaseTest, QueryNoResults) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    
    auto results = kb->query("animal(?X)");
    
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Rule Evaluation Tests (Synchronous!)
// ============================================================================

TEST_F(KnowledgeBaseTest, SimpleRuleEvaluation) {
    // Facts
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "bob"}
    }));
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "bob"},
        Term{TermType::CONSTANT, "alice"}
    }));
    
    // Rule: grandparent(X, Z) :- parent(X, Y), parent(Y, Z)
    kb->addRule(Rule(
        Fact("grandparent", {
            Term{TermType::VARIABLE, "X"},
            Term{TermType::VARIABLE, "Z"}
        }),
        {
            Fact("parent", {
                Term{TermType::VARIABLE, "X"},
                Term{TermType::VARIABLE, "Y"}
            }),
            Fact("parent", {
                Term{TermType::VARIABLE, "Y"},
                Term{TermType::VARIABLE, "Z"}
            })
        }
    ));
    
    auto results = kb->query("grandparent(?X, ?Z)");
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get("X"), "john");
    EXPECT_EQ(results[0].get("Z"), "alice");
}

TEST_F(KnowledgeBaseTest, RuleWithMultipleMatches) {
    // Facts: parent relationships
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "tom"},
        Term{TermType::CONSTANT, "john"}
    }));
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "tom"},
        Term{TermType::CONSTANT, "mary"}
    }));
    
    // Rule: sibling(X, Y) :- parent(Z, X), parent(Z, Y)
    kb->addRule(Rule(
        Fact("sibling", {
            Term{TermType::VARIABLE, "X"},
            Term{TermType::VARIABLE, "Y"}
        }),
        {
            Fact("parent", {
                Term{TermType::VARIABLE, "Z"},
                Term{TermType::VARIABLE, "X"}
            }),
            Fact("parent", {
                Term{TermType::VARIABLE, "Z"},
                Term{TermType::VARIABLE, "Y"}
            })
        }
    ));
    
    auto results = kb->query("sibling(?A, ?B)");
    
    // Should get (john, john), (john, mary), (mary, john), (mary, mary)
    EXPECT_GE(results.size(), 2);
}

// ============================================================================
// File Loading Tests
// ============================================================================

TEST_F(KnowledgeBaseTest, LoadFactsFromFile) {
    writeTestFile(R"(
person(john).
person(mary).
age(john, 30).
)");
    
    kb->loadFromFile(testFile.string());
    
    auto personFacts = kb->getFacts("person");
    EXPECT_EQ(personFacts.size(), 2);
    
    auto ageFacts = kb->getFacts("age");
    EXPECT_EQ(ageFacts.size(), 1);
}

TEST_F(KnowledgeBaseTest, LoadRulesFromFile) {
    writeTestFile(R"(
parent(john, bob).
parent(bob, alice).

grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
)");
    
    kb->loadFromFile(testFile.string());
    
    auto results = kb->query("grandparent(?X, ?Z)");
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get("X"), "john");
    EXPECT_EQ(results[0].get("Z"), "alice");
}

TEST_F(KnowledgeBaseTest, LoadWithComments) {
    writeTestFile(R"(
% This is a comment
person(john).
% Another comment
person(mary).
)");
    
    kb->loadFromFile(testFile.string());
    
    auto facts = kb->getFacts("person");
    EXPECT_EQ(facts.size(), 2);
}

// ============================================================================
// Unification Tests
// ============================================================================

TEST_F(KnowledgeBaseTest, UnificationBasic) {
    Fact goal("parent", {
        Term{TermType::VARIABLE, "X"},
        Term{TermType::CONSTANT, "mary"}
    });
    Fact fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "mary"}
    });
    
    auto result = Unifier::unify(goal, fact, BindingSet{});
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("X"), "john");
}

TEST_F(KnowledgeBaseTest, UnificationFails) {
    Fact goal("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "mary"}
    });
    Fact fact("parent", {
        Term{TermType::CONSTANT, "alice"},
        Term{TermType::CONSTANT, "mary"}
    });
    
    auto result = Unifier::unify(goal, fact, BindingSet{});
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(KnowledgeBaseTest, UnificationWithExistingBindings) {
    BindingSet existing;
    existing.add("X", "john");
    
    Fact goal("parent", {
        Term{TermType::VARIABLE, "X"},
        Term{TermType::VARIABLE, "Y"}
    });
    Fact fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "mary"}
    });
    
    auto result = Unifier::unify(goal, fact, existing);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("X"), "john");
    EXPECT_EQ(result->get("Y"), "mary");
}

TEST_F(KnowledgeBaseTest, UnificationConflictingBindings) {
    BindingSet existing;
    existing.add("X", "alice");  // X is already bound to alice
    
    Fact goal("parent", {
        Term{TermType::VARIABLE, "X"},
        Term{TermType::CONSTANT, "mary"}
    });
    Fact fact("parent", {
        Term{TermType::CONSTANT, "john"},  // But fact has john
        Term{TermType::CONSTANT, "mary"}
    });
    
    auto result = Unifier::unify(goal, fact, existing);
    
    EXPECT_FALSE(result.has_value());  // Should fail
}

// ============================================================================
// Complex Rule Tests
// ============================================================================

TEST_F(KnowledgeBaseTest, TransitiveRule) {
    // ancestor(X, Y) :- parent(X, Y).
    // ancestor(X, Z) :- parent(X, Y), ancestor(Y, Z).
    
    writeTestFile(R"(
parent(a, b).
parent(b, c).
parent(c, d).

ancestor(X, Y) :- parent(X, Y).
ancestor(X, Z) :- parent(X, Y), ancestor(Y, Z).
)");
    
    kb->loadFromFile(testFile.string());
    
    // Query: who is ancestor of d?
    auto results = kb->query("ancestor(?X, d)");
    
    // Should find a, b, c as ancestors of d
    std::vector<std::string> ancestors;
    for (const auto& binding : results) {
        ancestors.push_back(binding.get("X"));
    }
    
    EXPECT_THAT(ancestors, ::testing::UnorderedElementsAre("a", "b", "c"));
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(KnowledgeBaseTest, InvalidQuerySyntax) {
    EXPECT_THROW(kb->query("invalid("), std::runtime_error);
}

TEST_F(KnowledgeBaseTest, NonexistentFile) {
    EXPECT_THROW(kb->loadFromFile("nonexistent.pl"), std::runtime_error);
}

} // namespace
} // namespace kbgdb
