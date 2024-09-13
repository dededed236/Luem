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
#define MAX_BULK_BYTES 100000
#define DDOS_RATE "1Mb/s"
#define MAX_SIMULATION_TIME 1

// Number of Bots for DDoS
#define NUMBER_OF_BOTS 100
#define NUMBER_OF_EXTRA_NODES 4  // จำนวนโหนด user ใหม่

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
    pp1.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pp1.SetChannelAttribute("Delay", StringValue("1ms"));

    pp2.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pp2.SetChannelAttribute("Delay", StringValue("1ms"));

    pp3.SetDeviceAttribute("DataRate", StringValue("10Mbps"));  // อัตราการส่งข้อมูลสำหรับการเชื่อมต่อใหม่
    pp3.SetChannelAttribute("Delay", StringValue("1ms"));

    // Install the Point-To-Point Connections between Nodes
    NetDeviceContainer d02, d12, d23, botDeviceContainer[NUMBER_OF_BOTS], extraDeviceContainer[NUMBER_OF_EXTRA_NODES];
    d02 = pp1.Install(nodes.Get(0), nodes.Get(1));  // โหนดที่ 0 ต่อกับโหนดที่ 1
    d12 = pp1.Install(nodes.Get(1), nodes.Get(2));  // โหนดที่ 1 ต่อกับโหนดที่ 2
    d23 = pp1.Install(nodes.Get(2), nodes.Get(3));  // โหนดที่ 2 ต่อกับโหนดที่ 3 (โหนดที่เพิ่มเข้ามาใหม่)

    // Bot nodes connect to legitimate node 1
    for (int i = 0; i < NUMBER_OF_BOTS; ++i)
    {
        botDeviceContainer[i] = pp2.Install(botNodes.Get(i), nodes.Get(1));
    }

    // Extra nodes connections (new connection between extra nodes and node2)
    PointToPointHelper pp4;
    pp4.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pp4.SetChannelAttribute("Delay", StringValue("1ms"));

    for (int i = 0; i < NUMBER_OF_EXTRA_NODES; ++i)
    {
        extraDeviceContainer[i] = pp4.Install(nodes.Get(2), extraNodes.Get(i));  // เชื่อมต่อกับโหนดที่ 2
    }

    // Enable packet capture for Wireshark
    pp1.EnablePcapAll("legitimate_traffic/legitimate");
    pp2.EnablePcapAll("ddos_traffic/ddos");
    pp4.EnablePcapAll("extra_traffic/extra");

    // Assign IP addresses to legitimate nodes and bots
    InternetStackHelper stack;
    stack.Install(nodes);
    stack.Install(botNodes);
    stack.Install(extraNodes);

    Ipv4AddressHelper ipv4_n;

    Ipv4AddressHelper a02, a12, a23, a45, a56;
    a02.SetBase(Ipv4Address("10.1.1.0"), "255.255.255.0");
    a12.SetBase(Ipv4Address("10.1.2.0"), "255.255.255.0");
    a23.SetBase(Ipv4Address("10.1.3.0"), "255.255.255.0");  // สำหรับการเชื่อมต่อระหว่างโหนดที่ 2 และโหนดที่ 3
    a45.SetBase(Ipv4Address("10.1.4.0"), "255.255.255.0");
    a56.SetBase(Ipv4Address("10.1.5.0"), "255.255.255.0");

    // Assign dynamic IP to bot nodes
    for (int j = 0; j < NUMBER_OF_BOTS; ++j)
    {
        std::string botNetworkAddress = "10.2." + std::to_string(j + 1) + ".0";
        ipv4_n.SetBase(Ipv4Address(botNetworkAddress.c_str()), "255.255.255.0");
        ipv4_n.Assign(botDeviceContainer[j]);
        ipv4_n.NewNetwork();
    }

    // Assign IP addresses to legitimate nodes
    Ipv4InterfaceContainer i02, i12, i23, i45, i56;
    i02 = a02.Assign(d02);
    i12 = a12.Assign(d12);
    i23 = a23.Assign(d23);  // กำหนด IP ให้กับโหนดที่ 3
    i45 = a45.Assign(extraDeviceContainer[0]);  // ใช้การกำหนด IP Address ให้กับโหนด user ใหม่
    i56 = a56.Assign(extraDeviceContainer[1]);

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
        onOffApp[k].Start(Seconds(0.0));
        onOffApp[k].Stop(Seconds(MAX_SIMULATION_TIME));
    }

    // Legitimate TCP traffic application (BulkSend) on node0
    BulkSendHelper bulkSend("ns3::TcpSocketFactory", InetSocketAddress(i23.GetAddress(1), TCP_SINK_PORT));
    bulkSend.SetAttribute("MaxBytes", UintegerValue(MAX_BULK_BYTES));
    ApplicationContainer bulkSendApp = bulkSend.Install(nodes.Get(0));
    bulkSendApp.Start(Seconds(0.0));
    bulkSendApp.Stop(Seconds(MAX_SIMULATION_TIME));

    // BulkSend on extra nodes to send TCP data to node3
    BulkSendHelper extraBulkSend("ns3::TcpSocketFactory", InetSocketAddress(i23.GetAddress(1), TCP_SINK_PORT));
    extraBulkSend.SetAttribute("MaxBytes", UintegerValue(MAX_BULK_BYTES));
    ApplicationContainer extraBulkSendApp[NUMBER_OF_EXTRA_NODES];

    for (int k = 0; k < NUMBER_OF_EXTRA_NODES; ++k)
    {
        extraBulkSendApp[k] = extraBulkSend.Install(extraNodes.Get(k));
        extraBulkSendApp[k].Start(Seconds(0.0));
        extraBulkSendApp[k].Stop(Seconds(MAX_SIMULATION_TIME));
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
                                  "GridWidth", UintegerValue(4), "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    mobility.Install(botNodes);
    mobility.Install(extraNodes);

    AnimationInterface anim("animation.xml");

    // Flow monitor for tracking packet statistics
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(MAX_SIMULATION_TIME));
    Simulator::Run();
    flowMonitor->SerializeToXmlFile("results.xml", true, true);
    Simulator::Destroy();

    return 0;
}
