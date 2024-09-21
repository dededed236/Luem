// Install the Point-To-Point Connections between Nodes
NetDeviceContainer d02, d12, d23, d33, d43, botDeviceContainer[NUMBER_OF_BOTS], extraDeviceContainer[NUMBER_OF_EXTRA_NODES];
d02 = pp2.Install(nodes.Get(0), nodes.Get(1));  // โหนดที่ 0 ต่อกับโหนดที่ 1
d12 = pp2.Install(nodes.Get(0), nodes.Get(2));  // โหนดที่ 0 ต่อกับโหนดที่ 2  (เพิ่มการเชื่อมต่อ Node 0 ไปยัง Node 2)
d23 = pp1.Install(nodes.Get(1), nodes.Get(3));  // โหนดที่ 1 ต่อกับโหนดที่ 3
d33 = pp1.Install(nodes.Get(2), nodes.Get(3));  // โหนดที่ 2 ต่อกับโหนดที่ 3
d43 = pp1.Install(nodes.Get(1), nodes.Get(2));  // โหนดที่ 1 ต่อกับโหนดที่ 2
// Bot nodes connect to legitimate node 0
for (int i = 0; i < NUMBER_OF_BOTS; ++i)
{
    botDeviceContainer[i] = pp3.Install(botNodes.Get(i), nodes.Get(0));
}

// Configure IPs
Ipv4AddressHelper a12;
a12.SetBase("10.1.2.0", "255.255.255.0");
i12 = a12.Assign(d12);  // Assign IPs to the Node 0 to Node 2 connection

// Add new OnOff application for traffic from Node 0 to Node 2
OnOffHelper onoffToNode2("ns3::UdpSocketFactory", Address(InetSocketAddress(i12.GetAddress(1), UDP_SINK_PORT)));
onoffToNode2.SetConstantRate(DataRate(DATA_RATE));
ApplicationContainer appToNode2 = onoffToNode2.Install(nodes.Get(0)); // ติดตั้งแอปพลิเคชันบนโหนด 0

// Start the new application
appToNode2.Start(Seconds(0.0));
appToNode2.Stop(Seconds(MAX_SIMULATION_TIME));
