#include "epics/ca/ca_pv_manager.h"

namespace bchtree::epics::ca {

std::shared_ptr<CAPV> PVManager::Get(const std::string& pv_name) {
    std::shared_ptr<CAPV> pv;

    std::lock_guard<std::mutex> lock(mtx_);
    auto it = registry_.find(pv_name);
    if (it != registry_.end()) {
        pv = it->second.lock();
        if (!pv) {
            // expired -> erase entry so we can recreate
            registry_.erase(it);
        }
    }
    if (!pv) {
        pv = std::make_shared<CAPV>(ctx_, pv_name);
        registry_.emplace(pv_name, pv);
    }

    return pv;
}

void PVManager::Remove(const std::string& pv_name) {
    std::lock_guard<std::mutex> lock(mtx_);
    registry_.erase(pv_name);
}

void PVManager::Shutdown() {
    // Keep it simple: just clear the registry.
    // CAPV instances will be destroyed when all external shared_ptrs are
    // released.
    std::lock_guard<std::mutex> lock(mtx_);
    registry_.clear();
}

size_t PVManager::RegistrySize() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return registry_.size();
}

size_t PVManager::CollectGarbage() {
    std::lock_guard<std::mutex> lock(mtx_);
    size_t erased = 0;
    for (auto it = registry_.begin(); it != registry_.end();) {
        if (it->second.expired()) {
            it = registry_.erase(it);
            ++erased;
        } else {
            ++it;
        }
    }
    return erased;
}

}  // namespace bchtree::epics::ca
