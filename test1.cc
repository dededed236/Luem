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
#define MAX_BULK_BYTES 10000000
#define DDOS_RATE "1Mb/s"
#define MAX_SIMULATION_TIME 10

// Number of Bots for DDoS
#define NUMBER_OF_BOTS 100
#define NUMBER_OF_EXTRA_NODES 6  // Number of additional user nodes

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DDoSAttack");

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Creating Node containers
    NodeContainer nodes, botNodes, extraNodes;
    nodes.Create(4);  // Create 4 legitimate nodes
    botNodes.Create(NUMBER_OF_BOTS);  // Create bot nodes for DDoS
    extraNodes.Create(NUMBER_OF_EXTRA_NODES);  // Create extra user nodes

    // Setting up point-to-point links
    PointToPointHelper pp0_to_1, pp0_to_2, pp1_to_2;
    pp0_to_1.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pp0_to_1.SetChannelAttribute("Delay", StringValue("1ms"));

    pp0_to_2.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pp0_to_2.SetChannelAttribute("Delay", StringValue("1ms"));

    pp1_to_2.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    pp1_to_2.SetChannelAttribute("Delay", StringValue("1ms"));

    // Installing devices on nodes
    NetDeviceContainer d0_to_1 = pp0_to_1.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer d0_to_2 = pp0_to_2.Install(nodes.Get(0), nodes.Get(2));
    NetDeviceContainer d1_to_2 = pp1_to_2.Install(nodes.Get(1), nodes.Get(2));

    // Assigning IP addresses
    InternetStackHelper stack;
    stack.Install(nodes);
    stack.Install(botNodes);
    stack.Install(extraNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0_to_1 = address.Assign(d0_to_1);
    address.NewNetwork();
    Ipv4InterfaceContainer i0_to_2 = address.Assign(d0_to_2);
    address.NewNetwork();
    Ipv4InterfaceContainer i1_to_2 = address.Assign(d1_to_2);

    // DDoS Traffic configuration
    OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(i0_to_2.GetAddress(1), UDP_SINK_PORT)));
    onoff.SetConstantRate(DataRate(DDOS_RATE), 512);
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer apps = onoff.Install(botNodes);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(MAX_SIMULATION_TIME));

    // Legitimate Traffic configuration
    BulkSendHelper bulkSend("ns3::TcpSocketFactory", InetSocketAddress(i1_to_2.GetAddress(1), TCP_SINK_PORT));
    bulkSend.SetAttribute("MaxBytes", UintegerValue(MAX_BULK_BYTES));
    ApplicationContainer clientApps = bulkSend.Install(extraNodes);
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(MAX_SIMULATION_TIME));

    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), TCP_SINK_PORT));
    PacketSinkHelper udpSink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), UDP_SINK_PORT));
    ApplicationContainer sinkApps = tcpSink.Install(nodes.Get(3));
    sinkApps.Add(udpSink.Install(nodes.Get(3)));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(MAX_SIMULATION_TIME));

    // Enable global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure the NetAnim animator
    AnimationInterface anim("ddos-animation.xml");
    anim.SetConstantPosition(nodes.Get(0), 100, 50);
    anim.SetConstantPosition(nodes.Get(1), 150, 100);
    anim.SetConstantPosition(nodes.Get(2), 200, 50);

    // Setup the flow monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Run the simulation
    Simulator::Stop(Seconds(MAX_SIMULATION_TIME));
    Simulator::Run();
    Simulator::Destroy();

    // Save the flow monitor statistics
    monitor->SerializeToXmlFile("ddos-flowmon.xml", true, true);

    return 0;
}
