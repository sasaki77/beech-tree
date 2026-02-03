#pragma once
#include <behaviortree_cpp/behavior_tree.h>

#include "epics/ca/ca_pv.h"
#include "epics/ca/ca_pv_manager.h"
#include "epics/types.h"

namespace {
void printScalar(std::ostream& os, const bchtree::epics::PVScalarValue& s) {
    std::visit([&](auto v) { os << v; }, s);
}

}  // namespace

namespace bchtree {

template <typename T>
class CAGetNode : public BT::StatefulActionNode {
   public:
    static constexpr int kDefaultTimeoutMs = 1000;

    explicit CAGetNode(const std::string& name, const BT::NodeConfig& cfg,
                       std::shared_ptr<epics::ca::CAContextManager> ctx,
                       std::shared_ptr<epics::ca::PVManager> pv_manager)
        : BT::StatefulActionNode(name, cfg),
          ctx_(ctx),
          pv_manager_(pv_manager) {
        ctx_->EnsureAttached();
    }

    // Ports definition for BehaviorTree.CPP
    static BT::PortsList providedPorts() {
        using namespace BT;
        return {
            InputPort<std::string>("pv"),
            InputPort<int>("timeout"),
            OutputPort<T>("result"),
        };
    }

    // Lifecycle
    BT::NodeStatus onStart() override {
        cancelled_ = false;
        done_ = false;
        requested_ = false;

        if (!BT::TreeNode::getInput("pv", pv_name_)) {
            throw BT::RuntimeError("CAGetNode: missing required input [pv]");
        }
        BT::TreeNode::getInput("timeout", timeout_ms_);

        promise_ = std::promise<T>();
        future_ = promise_.get_future();

        if (!pv_) {
            pv_ = pv_manager_->Get(pv_name_);
            pv_->AddConnCB(
                [this](bool connected) { handleConnection(connected); });
        }

        connected_ = pv_->IsConnected();

        if (!connected_) {
            pv_->Connect();
        }

        if (connected_) {
            // Issue get
            bool status =
                pv_->GetCBAs<T>([this](T sample) { handleGetResult(sample); },
                                std::chrono::milliseconds(timeout_ms_));
            if (!status) {
                throw BT::RuntimeError("CAGetNode: failed to call getCB");
            }
            requested_ = true;
        }

        deadline_ = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms_);

        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        if (!requested_ && connected_) {
            // Issue get
            bool status =
                pv_->GetCBAs<T>([this](T sample) { handleGetResult(sample); },
                                std::chrono::milliseconds(timeout_ms_));
            if (!status) {
                throw BT::RuntimeError("CAGetNode: failed to call getCB");
            }
            requested_ = true;
        }

        // Check condition
        if (done_) {
            try {
                auto samp = future_.get();
                setOutput("result", samp);
                // printScalar(std::cout, samp.value);
                return BT::NodeStatus::SUCCESS;
            } catch (...) {
                return BT::NodeStatus::FAILURE;
            }
        }

        // timeout
        if (std::chrono::steady_clock::now() > deadline_) {
            cancelled_ = true;
            return BT::NodeStatus::FAILURE;
        }

        // Return RUNNING if it's not finished
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override { cancelled_ = true; }

    // Non-copyable / movable: node owns async state (promise/future) and EPICS
    CAGetNode(const CAGetNode&) = delete;
    CAGetNode& operator=(const CAGetNode&) = delete;
    CAGetNode(CAGetNode&&) noexcept = default;
    CAGetNode& operator=(CAGetNode&&) noexcept = default;

   private:
    void handleGetResult(T sample) {
        if (cancelled_) {
            return;
        }

        promise_.set_value(sample);
        done_ = true;
    }

    void handleConnection(bool connected) { connected_ = connected; }

    // EPICS CA PV handle
    std::shared_ptr<epics::ca::CAPV> pv_;
    std::shared_ptr<epics::ca::CAContextManager> ctx_;
    std::shared_ptr<epics::ca::PVManager> pv_manager_;

    // Execution flags
    std::atomic<bool> requested_{false};
    std::atomic<bool> done_{false};
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> connected_{false};

    // Result delivery: promise/future shared to allow repeated polls in
    // onRunning()
    std::promise<T> promise_;
    std::shared_future<T> future_;

    // Inputs (immutable during a single tick execution)
    std::string pv_name_;
    int timeout_ms_{kDefaultTimeoutMs};  // >= 0

    // Deadline for the current execution (set in onStart)
    std::chrono::steady_clock::time_point deadline_{};
};

}  // namespace bchtree
