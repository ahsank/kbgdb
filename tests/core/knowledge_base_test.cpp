#include "core/knowledge_base.h"
#include "query/query_parser.h"
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

TEST_F(KnowledgeBaseTest, UnifyVarVar1) {
    // Unifying simple(?X, ?Y) with simple(?Y, ?X)
    // Should bind X to Y (or Y to X)
    BindingSet existing;
    Fact goal("simple", {
        Term{TermType::VARIABLE, "X"},
        Term{TermType::VARIABLE, "Y"}
    });
    Fact fact("simple", {
        Term{TermType::VARIABLE, "Y"},  
        Term{TermType::VARIABLE, "X"}
    });
    
    auto result = Unifier::unify(goal, fact, existing);
    ASSERT_TRUE(result.has_value());
    
    // X should be bound to Y (as a variable)
    EXPECT_EQ(result->get("X"), "Y");
}

TEST_F(KnowledgeBaseTest, UnifyVarVarConst) {
    // Unifying simple(?X, ?Y, a) with simple(?Y, ?X, ?X)
    // Position 0: X unifies with Y -> X bound to Y
    // Position 1: Y unifies with X -> both refer to same, OK
    // Position 2: a (const) unifies with X -> X (and therefore Y) bound to "a"
    BindingSet existing;
    Fact goal("simple", {
        Term{TermType::VARIABLE, "X"},
        Term{TermType::VARIABLE, "Y"},
        Term{TermType::CONSTANT, "a"}
    });
    Fact fact("simple", {
        Term{TermType::VARIABLE, "Y"},
        Term{TermType::VARIABLE, "X"},
        Term{TermType::VARIABLE, "X"},
    });
    
    auto result = Unifier::unify(goal, fact, existing);
    ASSERT_TRUE(result.has_value());
    
    // After full resolution, X should resolve to "a"
    // because X->Y and Y->a (or similar chain)
    Term xTerm{TermType::VARIABLE, "X"};
    auto [resolvedX, isVarX] = Unifier::resolveWithType(xTerm, *result);
    EXPECT_EQ(resolvedX, "a");
    EXPECT_FALSE(isVarX);
}

TEST_F(KnowledgeBaseTest, OccursCheckNote) {
    // Note on occurs check:
    // In traditional Prolog, occurs check prevents infinite structures like X = f(X).
    // Since we have flat terms (atoms only, no compound structures like f(X)),
    // the occurs check mainly prevents direct self-reference.
    
    // This case: X -> Y already, unifying Y with X
    // This is fine - they're just unified to each other (both refer to same thing)
    BindingSet bindings;
    bindings.add("X", "Y");  // X -> Y
    
    Fact goal("test", {Term{TermType::VARIABLE, "Y"}});
    Fact fact("test", {Term{TermType::VARIABLE, "X"}});
    
    auto result = Unifier::unify(goal, fact, bindings);
    // This succeeds: X -> Y, and unifying Y with X means unifying Y with lookup(X)=Y
    // So Y = Y, which succeeds
    EXPECT_TRUE(result.has_value());
}

TEST_F(KnowledgeBaseTest, OccursCheckSelfReference) {
    // Direct self-reference: trying to unify X with X where X is bound to itself
    // This would be caught, but actually X=X just succeeds trivially
    BindingSet bindings;
    
    Fact goal("test", {Term{TermType::VARIABLE, "X"}});
    Fact fact("test", {Term{TermType::VARIABLE, "X"}});
    
    auto result = Unifier::unify(goal, fact, bindings);
    // X = X succeeds trivially (identical terms)
    EXPECT_TRUE(result.has_value());
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

// ============================================================================
// List and Compound Term Tests
// ============================================================================

TEST_F(KnowledgeBaseTest, ParseEmptyList) {
    QueryParser parser;
    parser.setRuleMode(false);
    
    Fact fact = parser.parse("test([])");
    ASSERT_EQ(fact.terms().size(), 1);
    EXPECT_TRUE(fact.terms()[0].isEmptyList());
}

TEST_F(KnowledgeBaseTest, ParseSimpleList) {
    QueryParser parser;
    parser.setRuleMode(false);
    
    Fact fact = parser.parse("test([a, b, c])");
    ASSERT_EQ(fact.terms().size(), 1);
    
    const Term& list = fact.terms()[0];
    EXPECT_TRUE(list.isList());
    EXPECT_TRUE(list.isConsList());
    
    // Check [a | [b | [c | []]]]
    EXPECT_EQ(list.head().value, "a");
    EXPECT_TRUE(list.tail().isConsList());
    EXPECT_EQ(list.tail().head().value, "b");
}

TEST_F(KnowledgeBaseTest, ParseListWithTail) {
    QueryParser parser;
    parser.setRuleMode(true);  // Rule mode for uppercase vars
    
    // [H | T] pattern
    Fact fact = parser.parse("test([H | T])");
    ASSERT_EQ(fact.terms().size(), 1);
    
    const Term& list = fact.terms()[0];
    EXPECT_TRUE(list.isConsList());
    EXPECT_TRUE(list.head().isVariable());
    EXPECT_EQ(list.head().value, "H");
    EXPECT_TRUE(list.tail().isVariable());
    EXPECT_EQ(list.tail().value, "T");
}

TEST_F(KnowledgeBaseTest, ParseListWithMultipleElementsAndTail) {
    QueryParser parser;
    parser.setRuleMode(true);
    
    // [a, b | T] pattern
    Fact fact = parser.parse("test([a, b | T])");
    ASSERT_EQ(fact.terms().size(), 1);
    
    const Term& list = fact.terms()[0];
    EXPECT_TRUE(list.isConsList());
    EXPECT_EQ(list.head().value, "a");
    EXPECT_EQ(list.tail().head().value, "b");
    EXPECT_TRUE(list.tail().tail().isVariable());
    EXPECT_EQ(list.tail().tail().value, "T");
}

TEST_F(KnowledgeBaseTest, UnifyListWithList) {
    // Unify [a, b] with [X, Y]
    Term list1 = Term::list({Term::constant("a"), Term::constant("b")});
    Term list2 = Term::list({Term::variable("X"), Term::variable("Y")});
    
    Fact goal("test", {list1});
    Fact fact("test", {list2});
    
    auto result = Unifier::unify(goal, fact, BindingSet{});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("X"), "a");
    EXPECT_EQ(result->get("Y"), "b");
}

TEST_F(KnowledgeBaseTest, UnifyListWithHeadTail) {
    // Unify [a, b, c] with [H | T]
    Term list1 = Term::list({Term::constant("a"), Term::constant("b"), Term::constant("c")});
    Term list2 = Term::cons(Term::variable("H"), Term::variable("T"));
    
    Fact goal("test", {list1});
    Fact fact("test", {list2});
    
    auto result = Unifier::unify(goal, fact, BindingSet{});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("H"), "a");
    
    // T should be bound to [b, c]
    auto tBinding = result->getTerm("T");
    ASSERT_TRUE(tBinding.has_value());
    EXPECT_TRUE(tBinding->isList());
    EXPECT_EQ(tBinding->toString(), "[b, c]");
}

TEST_F(KnowledgeBaseTest, UnifyEmptyListFails) {
    // Unify [] with [H | T] should fail
    Term empty = Term::emptyList();
    Term pattern = Term::cons(Term::variable("H"), Term::variable("T"));
    
    Fact goal("test", {empty});
    Fact fact("test", {pattern});
    
    auto result = Unifier::unify(goal, fact, BindingSet{});
    EXPECT_FALSE(result.has_value());
}

TEST_F(KnowledgeBaseTest, MemberPredicateBase) {
    // member(X, [X|_]).
    writeTestFile(R"(
member(X, [X|_]).
member(X, [_|T]) :- member(X, T).
)");
    
    kb->loadFromFile(testFile.string());
    
    // Query: member(a, [a, b, c])
    auto results = kb->query("member(a, [a, b, c])");
    EXPECT_GE(results.size(), 1);  // At least one match
}

TEST_F(KnowledgeBaseTest, MemberPredicateFindAll) {
    writeTestFile(R"(
member(X, [X|_]).
member(X, [_|T]) :- member(X, T).
)");
    
    kb->loadFromFile(testFile.string());
    
    // Query: member(?X, [a, b, c]) - find all members
    auto results = kb->query("member(?X, [a, b, c])");
    
    std::vector<std::string> members;
    for (const auto& binding : results) {
        members.push_back(binding.get("X"));
    }
    
    EXPECT_THAT(members, ::testing::UnorderedElementsAre("a", "b", "c"));
}

TEST_F(KnowledgeBaseTest, MemberPredicateNotFound) {
    writeTestFile(R"(
member(X, [X|_]).
member(X, [_|T]) :- member(X, T).
)");
    
    kb->loadFromFile(testFile.string());
    
    // Query: member(d, [a, b, c]) - d is not a member
    auto results = kb->query("member(d, [a, b, c])");
    EXPECT_TRUE(results.empty());
}

TEST_F(KnowledgeBaseTest, AppendPredicate) {
    writeTestFile(R"(
append([], L, L).
append([H|T1], L, [H|T2]) :- append(T1, L, T2).
)");
    
    kb->loadFromFile(testFile.string());
    
    // Query: append([a, b], [c, d], ?Result)
    auto results = kb->query("append([a, b], [c, d], ?R)");
    
    ASSERT_GE(results.size(), 1);
    auto rBinding = results[0].getTerm("R");
    ASSERT_TRUE(rBinding.has_value());
    EXPECT_EQ(rBinding->toString(), "[a, b, c, d]");
}

TEST_F(KnowledgeBaseTest, AppendPredicateSplit) {
    writeTestFile(R"(
append([], L, L).
append([H|T1], L, [H|T2]) :- append(T1, L, T2).
)");
    
    kb->loadFromFile(testFile.string());
    
    // Query: append(?X, ?Y, [a, b, c]) - find all ways to split the list
    auto results = kb->query("append(?X, ?Y, [a, b, c])");
    
    // Should find:
    // X=[], Y=[a,b,c]
    // X=[a], Y=[b,c]
    // X=[a,b], Y=[c]
    // X=[a,b,c], Y=[]
    EXPECT_EQ(results.size(), 4);
}

TEST_F(KnowledgeBaseTest, ParseCompoundTerm) {
    QueryParser parser;
    parser.setRuleMode(false);
    
    Fact fact = parser.parse("test(point(1, 2))");
    ASSERT_EQ(fact.terms().size(), 1);
    
    const Term& compound = fact.terms()[0];
    EXPECT_TRUE(compound.isCompound());
    EXPECT_EQ(compound.functor, "point");
    ASSERT_EQ(compound.args.size(), 2);
    EXPECT_EQ(compound.args[0].value, "1");
    EXPECT_EQ(compound.args[1].value, "2");
}

TEST_F(KnowledgeBaseTest, UnifyCompoundTerms) {
    // Unify point(X, 2) with point(1, Y)
    Term t1 = Term::compound("point", {Term::variable("X"), Term::constant("2")});
    Term t2 = Term::compound("point", {Term::constant("1"), Term::variable("Y")});
    
    Fact goal("test", {t1});
    Fact fact("test", {t2});
    
    auto result = Unifier::unify(goal, fact, BindingSet{});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("X"), "1");
    EXPECT_EQ(result->get("Y"), "2");
}

TEST_F(KnowledgeBaseTest, UnifyCompoundDifferentFunctors) {
    // point(1, 2) doesn't unify with vec(1, 2)
    Term t1 = Term::compound("point", {Term::constant("1"), Term::constant("2")});
    Term t2 = Term::compound("vec", {Term::constant("1"), Term::constant("2")});
    
    Fact goal("test", {t1});
    Fact fact("test", {t2});
    
    auto result = Unifier::unify(goal, fact, BindingSet{});
    EXPECT_FALSE(result.has_value());
}

TEST_F(KnowledgeBaseTest, NestedCompoundTerms) {
    QueryParser parser;
    parser.setRuleMode(false);
    
    // tree(node(a, leaf, leaf))
    Fact fact = parser.parse("tree(node(a, leaf, leaf))");
    ASSERT_EQ(fact.terms().size(), 1);
    
    const Term& tree = fact.terms()[0];
    EXPECT_TRUE(tree.isCompound());
    EXPECT_EQ(tree.functor, "node");
    ASSERT_EQ(tree.args.size(), 3);
    EXPECT_EQ(tree.args[0].value, "a");
    EXPECT_EQ(tree.args[1].value, "leaf");
    EXPECT_EQ(tree.args[2].value, "leaf");
}

TEST_F(KnowledgeBaseTest, ListToString) {
    Term list = Term::list({Term::constant("a"), Term::constant("b"), Term::constant("c")});
    EXPECT_EQ(list.toString(), "[a, b, c]");
    
    Term empty = Term::emptyList();
    EXPECT_EQ(empty.toString(), "[]");
    
    // [a | X] where X is variable
    Term improper = Term::cons(Term::constant("a"), Term::variable("X"));
    EXPECT_EQ(improper.toString(), "[a | ?X]");
}

} // namespace
} // namespace kbgdb
