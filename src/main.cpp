#include <behaviortree_cpp/bt_factory.h>

static const char *xml_text = R"(

<root BTCPP_format="4">
  <BehaviorTree ID="root">
    <Sequence name="root_sequence">
      <Sleep msec="5000" />
    </Sequence>
  </BehaviorTree>
</root>
 )";

int main(int argc, char **argv)
{
    BT::BehaviorTreeFactory factory;
    auto tree = factory.createTreeFromText(xml_text);
    tree.tickWhileRunning();

    return 0;
}
