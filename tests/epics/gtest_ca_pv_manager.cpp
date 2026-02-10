#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "epics/ca/ca_pv_manager.h"

using namespace bchtree::epics::ca;

class PVManagerTest : public ::testing::Test {
   protected:
    static inline auto ctx_{std::make_shared<CAContextManager>()};
    static inline auto manager_{std::make_unique<PVManager>(ctx_)};
};

TEST_F(PVManagerTest, ReturnsSameInstanceWhileAlive) {
    auto p1 = manager_->Get("TEST:PV1");
    auto p2 = manager_->Get("TEST:PV1");

    EXPECT_EQ(p1.get(), p2.get());
    EXPECT_EQ(manager_->RegistrySize(), 1u);
}

TEST_F(PVManagerTest, RecreatesAfterLastExternalReleased) {
    {
        auto p = manager_->Get("TEST:PV2");
        EXPECT_EQ(manager_->RegistrySize(), 1u);
    }  // p goes out of scope -> only weak_ptr remains

    // GC should remove the expired entry
    EXPECT_EQ(manager_->CollectGarbage(), 1u);
    EXPECT_EQ(manager_->RegistrySize(), 0u);

    // A new instance should be created
    auto p2 = manager_->Get("TEST:PV2");
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(manager_->RegistrySize(), 1u);
}

TEST_F(PVManagerTest, CollectGarbageCountsExpiredOnly) {
    auto alive = manager_->Get("TEST:PV3");
    {
        auto temp = manager_->Get("TEST:PV4");
        EXPECT_EQ(manager_->RegistrySize(), 2u);
        // After temp destroyed, PV4 loses all external refs
    }

    // GC removes PV4 but keeps PV3
    EXPECT_EQ(manager_->CollectGarbage(), 1u);
    EXPECT_EQ(manager_->RegistrySize(), 1u);
}

TEST_F(PVManagerTest, RemoveErasesRegistryEvenIfAlive) {
    auto p = manager_->Get("TEST:PV5");
    EXPECT_EQ(manager_->RegistrySize(), 1u);

    manager_->Remove("TEST:PV5");
    EXPECT_EQ(manager_->RegistrySize(), 0u);

    // External shared_ptr 'p' remains valid
    EXPECT_NE(p, nullptr);

    // Get again should create a new instance (different pointer)
    auto p2 = manager_->Get("TEST:PV5");
    EXPECT_NE(p.get(), p2.get());
    EXPECT_EQ(manager_->RegistrySize(), 1u);
}

TEST_F(PVManagerTest, ShutdownClearsRegistry) {
    auto p = manager_->Get("TEST:PV6");
    EXPECT_EQ(manager_->RegistrySize(), 1u);

    manager_->Shutdown();
    EXPECT_EQ(manager_->RegistrySize(), 0u);

    // External shared_ptr remains valid
    EXPECT_NE(p, nullptr);
}

TEST_F(PVManagerTest, ConcurrentGetSingleCreation) {
    const int N = 8;
    std::vector<std::shared_ptr<CAPV>> results(N);
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back(
            [&, i] { results[i] = manager_->Get("TEST:PV7"); });
    }
    for (auto& t : threads) t.join();

    // All threads should obtain the same instance
    auto* first = results[0].get();
    for (int i = 1; i < N; ++i) {
        EXPECT_EQ(first, results[i].get());
    }

    EXPECT_EQ(manager_->RegistrySize(), 1u);
}

TEST_F(PVManagerTest, DifferentNamesAreDifferentInstances) {
    auto a = manager_->Get("TEST:PV8A");
    auto b = manager_->Get("TEST:PV8B");

    EXPECT_NE(a.get(), b.get());
    EXPECT_EQ(manager_->RegistrySize(), 2u);
}
