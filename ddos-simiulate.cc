#include "ns3/mobility-module.h"
#include "ns3/nstime.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#define TCP_SINK_PORT 9000
#define UDP_SINK_PORT 9001

// Experimental parameters
#define DATA_RATE "100Kb/s"
#define DDOS_RATE "1Mb/s"
#define MAX_SIMULATION_TIME 10

// Number of Bots for DDoS
#define NUMBER_OF_BOTS 100
#define NUMBER_OF_EXTRA_NODES 6  // จำนวนโหนด user ใหม่

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DDoSAttack");

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Legitimate connection nodes (4 nodes)
    NodeContainer nodes;
    nodes.Create(4);  // เพิ่มโหนดปกติใหม่ รวมทั้งหมดเป็น 4 โหนด

    // Nodes for attack bots
    NodeContainer botNodes;
    botNodes.Create(NUMBER_OF_BOTS);

    // Nodes for extra users
    NodeContainer extraNodes;
    extraNodes.Create(NUMBER_OF_EXTRA_NODES);  // สร้างโหนด user ใหม่ 4 โหนด

    // Define the Point-To-Point Links and their Parameters
    PointToPointHelper pp1, pp2, pp3;
    pp1.SetDeviceAttribute("DataRate", StringValue("100Kbps"));
    pp1.SetChannelAttribute("Delay", StringValue("10ms"));
    pp1.SetChannelAttribute("DropProbability", DoubleValue(0.5));  // 50% แพ็กเกจจะถูกสูญเสีย

    pp2.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pp2.SetChannelAttribute("Delay", StringValue("10ms"));
    pp2.SetChannelAttribute("DropProbability", DoubleValue(0.5));  // 50% แพ็กเกจจะถูกสูญเสีย

    pp3.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    pp3.SetChannelAttribute("Delay", StringValue("1ms"));
    pp3.SetChannelAttribute("DropProbability", DoubleValue(0.5));  // 50% แพ็กเกจจะถูกสูญเสีย

    // Install the Point-To-Point Connections between Nodes
    NetDeviceContainer d02, d12, d23, botDeviceContainer[NUMBER_OF_BOTS], extraDeviceContainer[NUMBER_OF_EXTRA_NODES];
    d02 = pp2.Install(nodes.Get(0), nodes.Get(1));  // โหนดที่ 0 ต่อกับโหนดที่ 1
    d12 = pp2.Install(nodes.Get(1), nodes.Get(2));  // โหนดที่ 1 ต่อกับโหนดที่ 2
    d23 = pp1.Install(nodes.Get(2), nodes.Get(3));  // โหนดที่ 2 ต่อกับโหนดที่ 3 (โหนดที่เพิ่มเข้ามาใหม่)

    // Bot nodes connect to legitimate node 0
    for (int i = 0; i < NUMBER_OF_BOTS; ++i)
    {
        botDeviceContainer[i] = pp3.Install(botNodes.Get(i), nodes.Get(0));
    }

    for (int i = 0; i < NUMBER_OF_EXTRA_NODES; ++i)
    {
        if (i < 2)
        {
            // Extra node 104, 105 connected to Node 0
            extraDeviceContainer[i] = pp2.Install(nodes.Get(0), extraNodes.Get(i));
        }
        else if (i >= 2 && i < 4)
        {
            // Extra node 106, 107 connected to Node 1
            extraDeviceContainer[i] = pp2.Install(nodes.Get(1), extraNodes.Get(i));
        }
        else if (i >= 4)
        {
            // Extra node 108, 109 connected to Node 2
            extraDeviceContainer[i] = pp2.Install(nodes.Get(2), extraNodes.Get(i));
        }
    }

    // Assign IP addresses to legitimate nodes and bots
    InternetStackHelper stack;
    stack.Install(nodes);
    stack.Install(botNodes);
    stack.Install(extraNodes);

    Ipv4AddressHelper ipv4_n;
    ipv4_n.SetBase("10.0.0.0", "255.255.255.252");

    Ipv4InterfaceContainer extraInterfaces[NUMBER_OF_EXTRA_NODES];
    
    for (int i = 0; i < NUMBER_OF_EXTRA_NODES; ++i)
    {
        extraInterfaces[i] = ipv4_n.Assign(extraDeviceContainer[i]);
        ipv4_n.NewNetwork();
    }

    Ipv4AddressHelper a02, a12, a23;
    a02.SetBase("10.1.1.0", "255.255.255.0");
    a12.SetBase("10.1.2.0", "255.255.255.0");
    a23.SetBase("10.1.3.0", "255.255.255.0");  // สำหรับการเชื่อมต่อระหว่างโหนดที่ 2 และโหนดที่ 3

    for (int j = 0; j < NUMBER_OF_BOTS; ++j)
    {
        ipv4_n.Assign(botDeviceContainer[j]);
        ipv4_n.NewNetwork();
    }

    // Assign IP addresses to legitimate nodes
    Ipv4InterfaceContainer i02, i12, i23;
    i02 = a02.Assign(d02);
    i12 = a12.Assign(d12);
    i23 = a23.Assign(d23);  // กำหนด IP ให้กับโหนดที่ 3

    // DDoS Application Behaviour
    OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(i23.GetAddress(1), UDP_SINK_PORT)));
    onoff.SetConstantRate(DataRate(DDOS_RATE));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=30]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer onOffApp[NUMBER_OF_BOTS];

    // Install application on all bots
    for (int k = 0; k < NUMBER_OF_BOTS; ++k)
    {
        onOffApp[k] = onoff.Install(botNodes.Get(k));
        onOffApp[k].Start(Seconds(1.0));
        onOffApp[k].Stop(Seconds(MAX_SIMULATION_TIME));
    }

    // BulkSend on extra nodes to send TCP data to node3
    OnOffHelper onoffTcp("ns3::TcpSocketFactory", Address(InetSocketAddress(i23.GetAddress(1), TCP_SINK_PORT)));
    onoffTcp.SetConstantRate(DataRate(DATA_RATE));
    ApplicationContainer OnOffApp[NUMBER_OF_EXTRA_NODES];

    for (int k = 0; k < NUMBER_OF_EXTRA_NODES; k++)
    {
        OnOffApp[k] = onoffTcp.Install(extraNodes.Get(k));
        OnOffApp[k].Start(Seconds(0.0));
        OnOffApp[k].Stop(Seconds(MAX_SIMULATION_TIME));
    }

    // UDPSink on the receiver side
    PacketSinkHelper UDPsink("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), UDP_SINK_PORT)));
    ApplicationContainer UDPSinkApp = UDPsink.Install(nodes.Get(3));  // ติดตั้งบนโหนดที่ 3
    UDPSinkApp.Start(Seconds(0.0));
    UDPSinkApp.Stop(Seconds(MAX_SIMULATION_TIME));

    // TCP Sink Application on the server side
    PacketSinkHelper TCPsink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), TCP_SINK_PORT));
    ApplicationContainer TCPSinkApp = TCPsink.Install(nodes.Get(3));  // ติดตั้งบนโหนดที่ 3
    TCPSinkApp.Start(Seconds(0.0));
    TCPSinkApp.Stop(Seconds(MAX_SIMULATION_TIME));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Simulation NetAnim configuration and node placement
    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(5), "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    mobility.Install(botNodes);
    mobility.Install(extraNodes);  // ติดตั้งตำแหน่งโหนด user ใหม่

    AnimationInterface anim("DDos-topology.xml");

    // Load icons into NetAnim
    uint32_t node0Icon = anim.AddResource("ns-allinone-3.42/ns-3.42/icon/internet.png");
    uint32_t node1Icon = anim.AddResource("ns-allinone-3.42/ns-3.42/icon/router.png");
    uint32_t node2Icon = anim.AddResource("ns-allinone-3.42/ns-3.42/icon/router.png");
    uint32_t node3Icon = anim.AddResource("ns-allinone-3.42/ns-3.42/icon/web_server.png");
    uint32_t botIcon = anim.AddResource("ns-allinone-3.42/ns-3.42/icon/bot.png");
    uint32_t extraNodeIcon = anim.AddResource("ns-allinone-3.42/ns-3.42/icon/computer.png");  // ไอคอนสำหรับโหนด user ใหม่

    // Assign icons to each node
    anim.UpdateNodeImage(nodes.Get(0)->GetId(), node0Icon);
    anim.UpdateNodeImage(nodes.Get(1)->GetId(), node1Icon);
    anim.UpdateNodeImage(nodes.Get(2)->GetId(), node2Icon);
    anim.UpdateNodeImage(nodes.Get(3)->GetId(), node3Icon);  // กำหนด icon ให้กับโหนดที่ 3

    // Assign icons to bot nodes
    for (int i = 0; i < NUMBER_OF_BOTS; ++i)
    {
        anim.UpdateNodeImage(botNodes.Get(i)->GetId(), botIcon);
    }

    // Assign icons to extra nodes
    for (int i = 0; i < NUMBER_OF_EXTRA_NODES; ++i)
    {
        anim.UpdateNodeImage(extraNodes.Get(i)->GetId(), extraNodeIcon);
    }

    // Set positions for the nodes
    ns3::AnimationInterface::SetConstantPosition(nodes.Get(0), 50, 45);
    ns3::AnimationInterface::SetConstantPosition(nodes.Get(1), 80, 45);
    ns3::AnimationInterface::SetConstantPosition(nodes.Get(2), 110, 45);
    ns3::AnimationInterface::SetConstantPosition(nodes.Get(3), 160, 45);  // วางตำแหน่งของโหนดที่ 3

    // Set positions for extra nodes
    for (int i = 0; i < NUMBER_OF_EXTRA_NODES; ++i)
    {
        ns3::AnimationInterface::SetConstantPosition(extraNodes.Get(i), 100 + (i * 10), 60);  // วางตำแหน่งของโหนด user ใหม่
    }

    // Flow Monitor setup
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(MAX_SIMULATION_TIME));

    // Run the simulation
    Simulator::Run();

    // Serialize Flow Monitor data to XML file
    flowMonitor->SerializeToXmlFile("flowmonitor_ddos.xml", true, true);

    Simulator::Destroy();
    return 0;
}
