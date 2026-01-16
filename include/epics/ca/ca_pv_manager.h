#pragma once
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "epics/ca/ca_context_manager.h"
#include "epics/ca/ca_pv.h"

namespace bchtree::epics::ca {

class PVManager {
   public:
    explicit PVManager(std::shared_ptr<CAContextManager> ctx)
        : ctx_(std::move(ctx)) {}

    std::shared_ptr<CAPV> Get(const std::string& pv_name);

    void Remove(const std::string& pv_name);
    void Shutdown();
    size_t CollectGarbage();
    size_t RegistrySize() const;

   private:
    std::shared_ptr<CAContextManager> ctx_;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, std::weak_ptr<CAPV>> registry_;
};

}  // namespace bchtree::epics::ca
