// test/kb_test.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "knowledge_base.h"

// Mock class for any external dependencies
class MockStorage {
public:
    MOCK_METHOD(bool, store, (const std::string& key, const std::string& value), (override));
    MOCK_METHOD(std::string, retrieve, (const std::string& key), (override));
    MOCK_METHOD(bool, remove, (const std::string& key), (override));
};

class KnowledgeBaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockStorage = std::make_shared<MockStorage>();
        kb = std::make_unique<KnowledgeBase>(mockStorage);
    }

    void TearDown() override {
        kb.reset();
        mockStorage.reset();
    }

    std::shared_ptr<MockStorage> mockStorage;
    std::unique_ptr<KnowledgeBase> kb;
};

TEST_F(KnowledgeBaseTest, AddEntry) {
    // Arrange
    const std::string key = "test_key";
    const std::string value = "test_value";
    
    // Expect the storage to be called
    EXPECT_CALL(*mockStorage, store(key, value))
        .WillOnce(testing::Return(true));

    // Act
    bool result = kb->addEntry(key, value);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(KnowledgeBaseTest, GetEntry) {
    // Arrange
    const std::string key = "test_key";
    const std::string expected_value = "test_value";
    
    EXPECT_CALL(*mockStorage, retrieve(key))
        .WillOnce(testing::Return(expected_value));

    // Act
    std::string result = kb->getEntry(key);

    // Assert
    EXPECT_EQ(result, expected_value);
}

TEST_F(KnowledgeBaseTest, RemoveEntry) {
    // Arrange
    const std::string key = "test_key";
    
    EXPECT_CALL(*mockStorage, remove(key))
        .WillOnce(testing::Return(true));

    // Act
    bool result = kb->removeEntry(key);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(KnowledgeBaseTest, GetNonExistentEntry) {
    // Arrange
    const std::string key = "non_existent_key";
    
    EXPECT_CALL(*mockStorage, retrieve(key))
        .WillOnce(testing::Return(""));

    // Act & Assert
    EXPECT_THROW(kb->getEntry(key), std::runtime_error);
}

// Main function to run all tests
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
