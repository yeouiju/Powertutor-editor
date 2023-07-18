ns-3 doc (개발에 필요한 개념/knowhow 기록)

Key Concepts
1. Node
 - Host, end system in the Internet
 - Basic computing device abstraction
 - Represented in C++ by the class Node
2. Application
 - An Application generates packets
 - A user program that generates some activity to be simulated
 - NS-3 applications run on ns-3 Nodes to drive simulations
 - Represented in C++ by the class Application
   Ex) OnOffApplication,UdpEchoClientApplication 
3. Channel
 - Medium connected by nodes over which data flows
 - Represented in C++ by the class Channel
   Ex) CsmaChannel,PointToPointChannel,WifiChannel
4. Net device
 - Like a specific kind of network cable and a hardware device
 - Network Interface Cards; NICs
 - NICs are controlled using the software driver, net devices
 - Represented in C++ by the class NetDevice
 - Provides methods for managing connection to Node and Channel 
   Ex) CsmaNetDevice (work with CsmaChannel), WifiNetDevice (work with WifiChannel) 
5. Topology helpers
 - Topology helpers make ns-3 core operations as easy as possible
 - Create a NetDevice, add an address, install that net device on a Node, 
   configure the node’s protocol stack and connect the NetDevice to a Channel

ns-3 Simulation Process

1. Turning on logging
 NS_LOG_COMPONENT_DEFINE ("N-2-N Simulator");
 LogComponentEnable("UdpEchoClientApplication",LOG_LEVEL_INFO);

 NS LOG ERROR — Log error messages;
 NS LOG WARN — Log warning messages;
 NS LOG DEBUG — Log relatively rare debugging messages;
 NS LOG INFO — Log informational messages about program progress;
 NS LOG FUNCTION — Log a message describing each function called;
 NS LOG LOGIC – Log messages describing logical flow within a function;
 NS LOG ALL — Log everything.
 NS LOG UNCOND – Log the associated message unconditionally

 NS_LOG="N-2-N Simulator"=info ./ns3 run n2n

2. Creating network topology
 NodeContainer nodes; 
 nodes.Create (2);

 PointToPointHelper pointToPoint;
 pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); 
 pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
 
 NetDeviceContainer devices;
 devices = pointToPoint.Install (nodes);

 InternetStackHelper stack;
 stack.Install (nodes);

 Ipv4AddressHelper address;
 address.SetBase ("10.1.1.0", "255.255.255.0");
 Ipv4InterfaceContainer interfaces = address.Assign (devices);

3. Creating application 
 UdpEchoServerHelper echoServer (9);
 ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
 serverApps.Start (Seconds (1.0));
 serverApps.Stop (Seconds (10.0));

 UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
 echoClient.SetAttribute ("MaxPackets", UintegerValue (2));
 echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
 echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

 ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
 clientApps.Start (Seconds (2.0));
 clientApps.Stop (Seconds (10.0));

4. Running simulator 
  Simulator::Run ();
  Simulator::Destroy ();

5. Post-processing
Trace
pointToPoint.EnablePcapAll (“example”);
tcpdump -nn -tt -r s3_inclass_pcap-0-0.pcap