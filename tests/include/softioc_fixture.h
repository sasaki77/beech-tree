#include <gtest/gtest.h>

#include <filesystem>

#include "epics/ca/ca_context_manager.h"
#include "softioc_runner.h"

class SoftIocFixture : public ::testing::Test {
   protected:
    static inline SoftIocRunner runner_{};
    static inline std::shared_ptr<bchtree::epics::ca::CAContextManager> ctx_{
        std::make_shared<bchtree::epics::ca::CAContextManager>()};

    static void SetUpTestSuite();
    static void TearDownTestSuite();
};
