#include "epics/ca/ca_context_manager.h"

#include <iostream>
#include <string>

namespace bchtree::epics::ca {

void CAContextManager::Init() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (initialized_) return;

    int st = ca_context_create(ca_enable_preemptive_callback);
    if (st != ECA_NORMAL) {
        throw std::runtime_error(std::string("ca_context_create failed: ") +
                                 ca_message(st));
    }

    ctx_ = ca_current_context();
    if (!ctx_) {
        ca_context_destroy();
        throw std::runtime_error(
            "ca_current_context() returned null after create");
    }
    initialized_ = true;
}

void CAContextManager::EnsureAttached() {
    if (!initialized_) {
        Init();
    }

    // Nothing done if this thread is already attached
    void* cur = ca_current_context();
    if (cur == ctx_) {
        return;
    }

    // Attach this thread
    int st = ca_attach_context(ctx_);
    if (st != ECA_NORMAL) {
        throw std::runtime_error(std::string("ca_attach_context failed: ") +
                                 ca_message(st));
    }
}

void CAContextManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!initialized_) return;

    ca_context_destroy();
    ctx_ = nullptr;
    initialized_ = false;
}
}  // namespace bchtree::epics::ca
