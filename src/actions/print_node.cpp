#include <actions/print_node.h>

namespace bchtree {
BT::PortsList PrintNode::providedPorts() {
    return {BT::InputPort<std::string>("message")};
}

BT::NodeStatus PrintNode::tick() {
    BT::Expected<std::string> msg = getInput<std::string>("message");
    if (!msg) {
        throw BT::RuntimeError("missing required input [message]: ",
                               msg.error());
    }
    std::cout << msg.value() << std::endl;
    return BT::NodeStatus::SUCCESS;
}

}  // namespace bchtree
