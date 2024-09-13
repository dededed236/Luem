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

    // Create core nodes (routers/switches)
    NodeContainer coreNodes;
    coreNodes.Create(2);  // Core nodes (สองโหนดชั้น Core)

    // Edge nodes for legitimate connections (4 nodes)
    NodeContainer edgeNodes;
    edgeNodes.Create(4);  // โหนด edge ชั้น user 4 โหนด

    // Nodes for attack bots
    NodeContainer botNodes;
    botNodes.Create(NUMBER_OF_BOTS);

    // Nodes for extra users
    NodeContainer extraNodes;
    extraNodes.Create(NUMBER_OF_EXTRA_NODES);  // สร้างโหนด user ใหม่ 4 โหนด

    // Define the Point-To-Point Links and their Parameters
    PointToPointHelper coreP2P, edgeP2P, botP2P, extraP2P;
    coreP2P.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    coreP2P.SetChannelAttribute("Delay", StringValue("2ms"));

    edgeP2P.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    edgeP2P.SetChannelAttribute("Delay", StringValue("5ms"));

    botP2P.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    botP2P.SetChannelAttribute("Delay", StringValue("1ms"));

    extraP2P.SetDeviceAttribute("DataRate", StringValue("20Mbps"));
    extraP2P.SetChannelAttribute("Delay", StringValue("5ms"));

    // Connect Core nodes
    NetDeviceContainer coreDevices = coreP2P.Install(coreNodes.Get(0), coreNodes.Get(1));

    // Connect edge nodes to Core node 0
    NetDeviceContainer edgeDevices[4];
    for (int i = 0; i < 4; i++) {
        edgeDevices[i] = edgeP2P.Install(coreNodes.Get(0), edgeNodes.Get(i));  // Edge nodes ต่อกับ core node 0
    }

    // Bot nodes connect to core node 1
    NetDeviceContainer botDevices[NUMBER_OF_BOTS];
    for (int i = 0; i < NUMBER_OF_BOTS; i++) {
        botDevices[i] = botP2P.Install(coreNodes.Get(1), botNodes.Get(i));
    }

    // Extra nodes connections to Core node 1
    NetDeviceContainer extraDevices[NUMBER_OF_EXTRA_NODES];
    for (int i = 0; i < NUMBER_OF_EXTRA_NODES; i++) {
        extraDevices[i] = extraP2P.Install(coreNodes.Get(1), extraNodes.Get(i));  // Extra nodes ต่อกับ core node 1
    }

    // Enable packet capture for Wireshark
    coreP2P.EnablePcapAll("core_traffic");
    edgeP2P.EnablePcapAll("edge_traffic");
    botP2P.EnablePcapAll("ddos_traffic");
    extraP2P.EnablePcapAll("extra_traffic");

    // Assign IP addresses to all nodes
    InternetStackHelper stack;
    stack.Install(coreNodes);
    stack.Install(edgeNodes);
    stack.Install(botNodes);
    stack.Install(extraNodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    ipv4.Assign(coreDevices);

    Ipv4AddressHelper edgeAddresses;
    for (int i = 0; i < 4; i++) {
        edgeAddresses.SetBase("10.1." + std::to_string(i + 2) + ".0", "255.255.255.0");
        edgeAddresses.Assign(edgeDevices[i]);
    }

    Ipv4AddressHelper botAddresses;
    for (int i = 0; i < NUMBER_OF_BOTS; i++) {
        botAddresses.SetBase("10.2." + std::to_string(i + 1) + ".0", "255.255.255.0");
        botAddresses.Assign(botDevices[i]);
    }

    Ipv4AddressHelper extraAddresses;
    for (int i = 0; i < NUMBER_OF_EXTRA_NODES; i++) {
        extraAddresses.SetBase("10.3." + std::to_string(i + 1) + ".0", "255.255.255.0");
        extraAddresses.Assign(extraDevices[i]);
    }

    // Setup DDoS traffic using OnOff application
    OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(coreNodes.Get(1)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), UDP_SINK_PORT)));
    onoff.SetConstantRate(DataRate(DDOS_RATE));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer onOffApp[NUMBER_OF_BOTS];
    for (int i = 0; i < NUMBER_OF_BOTS; i++) {
        onOffApp[i] = onoff.Install(botNodes.Get(i));
        onOffApp[i].Start(Seconds(0.0));
        onOffApp[i].Stop(Seconds(MAX_SIMULATION_TIME));
    }

    // TCP Sink Application on an edge node
    PacketSinkHelper TCPsink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), TCP_SINK_PORT));
    ApplicationContainer TCPSinkApp = TCPsink.Install(edgeNodes.Get(0));
    TCPSinkApp.Start(Seconds(0.0));
    TCPSinkApp.Stop(Seconds(MAX_SIMULATION_TIME));

    // Simulation NetAnim configuration
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(coreNodes);
    mobility.Install(edgeNodes);
    mobility.Install(botNodes);
    mobility.Install(extraNodes);

    AnimationInterface anim("two_tier_ddos.xml");

    // Run simulation
    Simulator::Stop(Seconds(MAX_SIMULATION_TIME));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
