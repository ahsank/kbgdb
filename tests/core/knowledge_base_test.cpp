#include "core/knowledge_base.h"
#include "storage/storage_provider.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/futures/Future.h>
#include <filesystem>
#include <fstream>

namespace kbgdb {
namespace {

// Mock storage provider for testing external storage
class MockStorageProvider : public StorageProvider {
public:
    MOCK_METHOD(bool, canHandle, (const Fact&), (override));
    MOCK_METHOD(folly::Future<std::vector<Fact>>, getFacts, (const Fact&), (override));
};

// Custom matcher for folly::Future
MATCHER_P(ReturnsFuture, value, "") {
    return true;  // Simplified matching for test purposes
}

class KnowledgeBaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test file
        test_file = std::filesystem::temp_directory_path() / "kb_test.pl";
        kb = std::make_unique<KnowledgeBase>("");
    }

    void TearDown() override {
        // Clean up test file
        if (std::filesystem::exists(test_file)) {
            std::filesystem::remove(test_file);
        }
    }

    void writeTestData(const std::string& content) {
        std::ofstream file(test_file);
        file << content;
        file.close();
    }

    std::filesystem::path test_file;
    std::unique_ptr<KnowledgeBase> kb;
};

TEST_F(KnowledgeBaseTest, AddAndRetrieveFact) {
    Fact fact("person", {Term{TermType::CONSTANT, "john"}});
    kb->addFact(fact);
    
    auto facts = kb->getMemoryFacts("person");
    ASSERT_EQ(facts.size(), 1);
    EXPECT_EQ(facts[0].toString(), "person(john)");
}

TEST_F(KnowledgeBaseTest, AddAndRetrieveMultipleFacts) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "mary"}}));
    kb->addFact(Fact("age", {Term{TermType::CONSTANT, "john"}, Term{TermType::NUMBER, "30"}}));
    
    auto personFacts = kb->getMemoryFacts("person");
    ASSERT_EQ(personFacts.size(), 2);
    
    auto ageFacts = kb->getMemoryFacts("age");
    ASSERT_EQ(ageFacts.size(), 1);
    EXPECT_EQ(ageFacts[0].toString(), "age(john, 30)");
}

TEST_F(KnowledgeBaseTest, SimpleQuery) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    
    bool queryCompleted = false;
    kb->query("person(?X)")
        .thenValue([&](std::vector<BindingSet> results) {
            EXPECT_EQ(results.size(), 1);
            EXPECT_EQ(results[0].get("X"), "john");
            queryCompleted = true;
        })
        .wait();
    
    EXPECT_TRUE(queryCompleted);
}

TEST_F(KnowledgeBaseTest, QueryWithMultipleResults) {
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "john"}}));
    kb->addFact(Fact("person", {Term{TermType::CONSTANT, "mary"}}));
    
    bool queryCompleted = false;
    kb->query("person(?X)")
        .thenValue([&](std::vector<BindingSet> results) {
            EXPECT_EQ(results.size(), 2);
            std::vector<std::string> names;
            for (const auto& binding : results) {
                names.push_back(binding.get("X"));
            }
            EXPECT_THAT(names, ::testing::UnorderedElementsAre("john", "mary"));
            queryCompleted = true;
        })
        .wait();
    
    EXPECT_TRUE(queryCompleted);
}

TEST_F(KnowledgeBaseTest, RuleEvaluation) {
    // Add facts and rule for testing
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "john"},
        Term{TermType::CONSTANT, "bob"}
    }));
    kb->addFact(Fact("parent", {
        Term{TermType::CONSTANT, "bob"},
        Term{TermType::CONSTANT, "alice"}
    }));
    
    writeTestData("grandparent(_X, _Z) :- parent(_X, _Y), parent(_Y, _Z).");
    kb->loadFromFile(test_file.string());
    
    bool queryCompleted = false;
    kb->query("grandparent(?X, ?Z)")
        .thenValue([&](std::vector<BindingSet> results) {
            ASSERT_EQ(results.size(), 1);
            EXPECT_EQ(results[0].get("X"), "john");
            EXPECT_EQ(results[0].get("Z"), "alice");
            queryCompleted = true;
        })
        .wait();
    
    EXPECT_TRUE(queryCompleted);
}

TEST_F(KnowledgeBaseTest, ExternalStorageProvider) {
    auto mockProvider = std::make_unique<MockStorageProvider>();
    auto* mockProviderPtr = mockProvider.get();
    
    // Set up mock expectations
    EXPECT_CALL(*mockProviderPtr, canHandle(::testing::_))
        .WillRepeatedly(::testing::Return(true));
    
    std::vector<Fact> mockFacts = {
        Fact("external_data", {Term{TermType::CONSTANT, "value1"}}),
        Fact("external_data", {Term{TermType::CONSTANT, "value2"}})
    };
    
    EXPECT_CALL(*mockProviderPtr, getFacts(::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [mockFacts](const Fact&) -> folly::Future<std::vector<Fact>> {
                return folly::makeFuture(mockFacts);
            }));
    
    
    kb->addExternalProvider(std::move(mockProvider));
    
    bool queryCompleted = false;
    kb->query("external_data(?X)")
        .thenValue([&](std::vector<BindingSet> results) {
            EXPECT_EQ(results.size(), 2);
            std::vector<std::string> values;
            for (const auto& binding : results) {
                values.push_back(binding.get("X"));
            }
            EXPECT_THAT(values, ::testing::UnorderedElementsAre("value1", "value2"));
            queryCompleted = true;
        })
        .wait();
    
    EXPECT_TRUE(queryCompleted);
}

TEST_F(KnowledgeBaseTest, UnificationTest) {
    Fact goal("person", {Term{TermType::VARIABLE, "X"}});
    Fact fact("person", {Term{TermType::CONSTANT, "john"}});
    BindingSet bindings;
    
    auto result = kb->unifyFact(goal, fact, bindings);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("X"), "john");
}

TEST_F(KnowledgeBaseTest, ComplexRuleEvaluation) {
    // Setup a more complex rule with multiple body goals
    writeTestData(R"(
        % Facts
        male(john).
        age(john, 25).
        has_license(john).
        
        % Rule
        can_drive(Person) :- male(Person), age(Person, Age), has_license(Person).
    )");
    
    kb = std::make_unique<KnowledgeBase>(test_file.string());
    
    bool queryCompleted = false;
    kb->query("can_drive(?X)")
        .thenValue([&](std::vector<BindingSet> results) {
            EXPECT_EQ(results.size(), 1);
            EXPECT_EQ(results[0].get("X"), "john");
            queryCompleted = true;
        })
        .wait();
    
    EXPECT_TRUE(queryCompleted);
}

TEST_F(KnowledgeBaseTest, InvalidQueries) {
    bool errorCaught = false;
    kb->query("invalid_predicate(")
        .thenError<std::exception>([&](const std::exception& e) {
            errorCaught = true;
            return folly::makeFuture<std::vector<BindingSet>>(std::vector<BindingSet>{});
        })
        .wait();
    
    EXPECT_TRUE(errorCaught);
}

TEST_F(KnowledgeBaseTest, LoadFromFile) {
    writeTestData(R"(
        person(john).
        person(mary).
        age(john, 30).
        parent(john, bob).
        parent(mary, alice).
        
        sibling(?X, ?Y) :- parent(?Z, ?X), parent(?Z, ?Y).
    )");
    
    kb = std::make_unique<KnowledgeBase>(test_file.string());
    
    // Test loaded facts
    auto personFacts = kb->getMemoryFacts("person");
    EXPECT_EQ(personFacts.size(), 2);
    
    // Test rule evaluation
    bool queryCompleted = false;
    kb->query("sibling(?X, ?Y)")
        .thenValue([&](std::vector<BindingSet> results) {
            EXPECT_GT(results.size(), 0);
            queryCompleted = true;
        })
        .wait();
    
    EXPECT_TRUE(queryCompleted);
}

} // namespace
} // namespace kbgdb
