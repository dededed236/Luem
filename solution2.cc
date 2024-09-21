// Create a lossy channel with 50% packet loss
pp1.SetChannelAttribute("Delay", StringValue("10ms"));
pp1.SetChannelAttribute("LossModel", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"));
pp1.SetChannelAttribute("DropProbability", DoubleValue(0.5));  // 50% packet loss

pp2.SetChannelAttribute("Delay", StringValue("10ms"));
pp2.SetChannelAttribute("LossModel", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"));
pp2.SetChannelAttribute("DropProbability", DoubleValue(0.5));  // 50% packet loss

pp3.SetChannelAttribute("Delay", StringValue("1ms"));
pp3.SetChannelAttribute("LossModel", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"));
pp3.SetChannelAttribute("DropProbability", DoubleValue(0.5));  // 50% packet loss
