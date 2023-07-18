/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Network topology
// //       10.1.1.0       11.1.1.0       192.168.1.0
// //       n0  n1  n2     r1     r2      n0  n1  n2
// //       |   |   |      _      _       |   |   |
// //       ==============|_|====|_|==============
// //                router
// //

#include "ns3/core-module.h"
#include "ns3/csma-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lan2Lan");

int
main (int argc, char *argv[])
{
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  CommandLine cmd;

  uint32_t n1 = 3;
  uint32_t n2 = 3;

  cmd.AddValue ("n1", "Number of LAN 1 nodes", n1);
  cmd.AddValue ("n2", "Number of LAN 2 nodes", n2);

  cmd.Parse (argc, argv);

  NodeContainer lan1_nodes;
  lan1_nodes.Create (3);

  NodeContainer lan2_nodes;
  lan2_nodes.Create (3);

  Ptr<Node> router1 = CreateObject<Node> ();
  Ptr<Node> router2 = CreateObject<Node> ();

  CsmaHelper csma1;
  csma1.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma1.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  lan1_nodes.Add (router1);

  NetDeviceContainer lan1_devs;
  lan1_devs = csma1.Install (lan1_nodes);

  CsmaHelper csma2;
  csma2.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma2.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  lan2_nodes.Add (router2);

  NetDeviceContainer lan2_devs;
  lan2_devs = csma2.Install (lan2_nodes);

  PointToPointHelper p2p_link1;
  p2p_link1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p_link1.SetChannelAttribute ("Delay", StringValue ("3ms"));
  NetDeviceContainer p2p_devs1 = p2p_link1.Install (router1, router2);

  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer lan1_interfaces = address.Assign (lan1_devs);
  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer lan2_interfaces = address.Assign (lan2_devs);
  address.SetBase ("11.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer link1_interface = address.Assign (p2p_devs1);

  NodeContainer serverNodes (lan2_nodes.Get (0), lan2_nodes.Get (1), lan2_nodes.Get (2));
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (serverNodes);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (lan2_interfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  NodeContainer clientNodes (lan1_nodes.Get (0), lan1_nodes.Get (1), lan1_nodes.Get (2));
  ApplicationContainer clientApps = echoClient.Install (clientNodes);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  for (uint32_t i = 0; i < clientApps.GetN (); i++)
    {
      Ptr<UdpEchoClient> client = DynamicCast<UdpEchoClient> (clientApps.Get (i));
      client->SetRemote (lan2_interfaces.GetAddress (i), 9);
      std::cout << "Echo Client " << i << " " << lan1_interfaces.GetAddress (i) << "\t"
                << lan2_interfaces.GetAddress (i) << std::endl;
    }

  csma1.EnablePcapAll ("L2L-Lan1");
  csma2.EnablePcapAll ("L2L-Lan2");
  p2p_link1.EnablePcapAll ("L2L-Router");

  Simulator::Run ();
  Simulator::Destroy ();
}