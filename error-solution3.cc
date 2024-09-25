Ptr<Ipv4> firewallIpv4 = firewallNode->GetObject<Ipv4>();
Ptr<Ipv4RoutingProtocol> routingProtocol = firewallIpv4->GetRoutingProtocol();
Ptr<Ipv4StaticRouting> staticRouting = DynamicCast<Ipv4StaticRouting>(routingProtocol);
