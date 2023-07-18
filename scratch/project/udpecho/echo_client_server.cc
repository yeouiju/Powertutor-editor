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

#include "ns3/core-module.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

// Turning on logging
NS_LOG_COMPONENT_DEFINE ("N-2-N Simulator");

int
main (int argc, char *argv[])
{
  bool verbose = true;

  NS_LOG_UNCOND ("Hello Main");
  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  // Turning on logging
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  /* Creating network topology */
  // Create a node container and 2 nodes
  NodeContainer nodes;
  nodes.Create (2);
  NS_LOG_INFO ("[DEBUG] Create " << nodes.GetN () << " nodes");

  NS_LOG_INFO ("[DEBUG] Create a channel & 2 PointToPointNetDevice.");
  // p2p link generation and install on the p2p nodes
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer net_devices;
  net_devices = pointToPoint.Install (nodes);

  // Install Internet stack on the nodes;
  InternetStackHelper stack;
  stack.Install (nodes);
  // Allocate IP address
  Ipv4AddressHelper ipv4_address;
  ipv4_address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2p_interfaces = ipv4_address.Assign (net_devices);

  /* Creating application */
  //  Setup echoServer and Install it on node1
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  NS_LOG_INFO ("[DEBUG] echoClient: MaxPackets=2, Interval=1s, PacketSize=1024");
  std::cout << std::endl;

  // Setup echoClient
  UdpEchoClientHelper echoClient (p2p_interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (2));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  // Install echoClient on node
  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("echo");
  /* Running simulation */
  Simulator::Run ();
  Simulator::Destroy ();

  /* Post-processing */
  return 0;
}
