// Define the Point-To-Point Links and their Parameters
PointToPointHelper pp1, pp2, pp3;
pp1.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
pp1.SetChannelAttribute("Delay", StringValue("10ms"));
pp1.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("10p")));  // Reduce queue size

pp2.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
pp2.SetChannelAttribute("Delay", StringValue("10ms"));
pp2.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("10p")));  // Reduce queue size

pp3.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
pp3.SetChannelAttribute("Delay", StringValue("1ms"));
pp3.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("10p")));  // Reduce queue size
