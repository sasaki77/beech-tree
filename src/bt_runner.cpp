#include "bt_runner.h"

#include <behaviortree_cpp/xml_parsing.h>

#include "actions/caget_node.h"
#include "actions/print_node.h"

namespace {
int map_status_to_exit_code(BT::NodeStatus status) {
    switch (status) {
        case BT::NodeStatus::SUCCESS:
            return 0;
        case BT::NodeStatus::FAILURE:
            return 1;
        default:
            return 2;
    }
}
}  // namespace

namespace bchtree {

int BTRunner::run(const std::string& treePath) {
    BT::BehaviorTreeFactory factory;
    blackboard_ = BT::Blackboard::create();

    auto ctx = std::make_shared<epics::ca::CAContextManager>();
    ctx->Init();

    auto pv_manager = std::make_shared<epics::ca::PVManager>(ctx);
    factory.registerNodeType<CAGetNode<epics::PVData>>("CAGet", ctx,
                                                       pv_manager);
    factory.registerNodeType<CAGetNode<double>>("CAGetDouble", ctx, pv_manager);
    factory.registerNodeType<CAGetNode<int>>("CAGetInt", ctx, pv_manager);
    factory.registerNodeType<CAGetNode<std::string>>("CAGetString", ctx,
                                                     pv_manager);

    BT::Tree tree = factory.createTreeFromFile(treePath, blackboard_);
    tree_ = std::make_unique<BT::Tree>(std::move(tree));

    if (logger_) {
        logger_->info("Start Tree: " + treePath);
    }

    const BT::NodeStatus status = tree_->tickWhileRunning();

    if (logger_) {
        logger_->info(std::string("End Tree: status=") + toStr(status));
    }

    return map_status_to_exit_code(status);
}

void BTRunner::setLogger(Logger* logger) { logger_ = logger; }
}  // namespace bchtree
