#pragma once
#include <behaviortree_cpp/behavior_tree.h>

#include "epics/ca/ca_pv.h"
#include "epics/ca/ca_pv_manager.h"
#include "epics/types.h"

namespace bchtree {

template <typename T>
class CAPutNode : public BT::StatefulActionNode {
   public:
    static constexpr int kDefaultTimeoutMs = 1000;

    explicit CAPutNode(const std::string& name, const BT::NodeConfig& cfg,
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
            InputPort<T>("value"),
            InputPort<int>("timeout"),
            InputPort<bool>("force_write"),
        };
    }

    // Lifecycle
    BT::NodeStatus onStart() override {
        cancelled_ = false;
        done_ = false;
        requested_ = false;

        if (!BT::TreeNode::getInput("pv", pv_name_)) {
            throw BT::RuntimeError("CAPutNode: missing required input [pv]");
        }
        if (!BT::TreeNode::getInput("value", value_)) {
            throw BT::RuntimeError("CAPutNode: missing required input [value]");
        }
        BT::TreeNode::getInput("timeout", timeout_ms_);
        BT::TreeNode::getInput("force_write", force_write_);

        deadline_ = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms_);

        if (!pv_) {
            pv_ = pv_manager_->Get(pv_name_);
            pv_->AddConnCB(
                [this](bool connected) { handleConnection(connected); });
        }

        connected_ = pv_->IsConnected();

        if (!connected_) {
            pv_->Connect();
            return BT::NodeStatus::RUNNING;
        }

        if (!force_write_) {
            T current_val = pv_->GetAs<T>();
            if (value_ == current_val) {
                return BT::NodeStatus::SUCCESS;
            }
        }

        // Issue put
        bool status = pv_->PutCB(
            value_, [this](bool success) { handlePutResult(success); });
        if (!status) {
            throw BT::RuntimeError("CAPutNode: failed to call PutCB");
        }
        requested_ = true;

        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        if (!requested_ && connected_) {
            // Issue put
            bool status = pv_->PutCB(
                value_, [this](bool success) { handlePutResult(success); });
            if (!status) {
                throw BT::RuntimeError("CAPutNode: failed to call PutCB");
            }
            requested_ = true;
        }

        // Check condition
        if (done_) {
            return BT::NodeStatus::SUCCESS;
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
    CAPutNode(const CAPutNode&) = delete;
    CAPutNode& operator=(const CAPutNode&) = delete;
    CAPutNode(CAPutNode&&) noexcept = default;
    CAPutNode& operator=(CAPutNode&&) noexcept = default;

   private:
    void handlePutResult(bool success) {
        if (cancelled_) {
            return;
        }

        done_ = success;
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

    // Inputs (immutable during a single tick execution)
    std::string pv_name_;
    int timeout_ms_{kDefaultTimeoutMs};  // >= 0
    T value_;
    bool force_write_{false};

    // Deadline for the current execution (set in onStart)
    std::chrono::steady_clock::time_point deadline_{};
};

}  // namespace bchtree
