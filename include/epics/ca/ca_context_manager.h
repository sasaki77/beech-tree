#pragma once
#include <cadef.h>

#include <memory>
#include <mutex>

namespace bchtree::epics::ca {
class CAContextManager {
   public:
    explicit CAContextManager() {}

    void Init();
    void EnsureAttached();
    void Shutdown();

   private:
    std::mutex mtx_;
    ca_client_context* ctx_;
    bool initialized_ = false;
};

}  // namespace bchtree::epics::ca
