/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008-2009 Strasbourg University
 *
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
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 *         Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

// Network topology
// //
// //             n0   r    n1
// //             |    _    |
// //             ====|_|====
// //                router
// //
// // - Tracing of queues and packet receptions to file "simple-routing-ping6.tr"

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/ipv6-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("N1-R1-N1");

int
main (int argc, char **argv)
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> node0 = CreateObject<Node> ();
  Ptr<Node> router = CreateObject<Node> ();
  Ptr<Node> node2 = CreateObject<Node> ();

  NodeContainer net1_nodes (node0, router);
  NodeContainer net2_nodes (router, node2);
  NodeContainer net_all (node0, router, node2);

  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma_p2p;
  csma_p2p.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma_p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  // NetDeviceContainer net_devs1 = csma_p2p.Install (net1_nodes);
  // NetDeviceContainer net_devs2 = csma_p2p.Install (net2_nodes);
  NetDeviceContainer net_devs_all = csma_p2p.Install (net_all);

  NS_LOG_INFO ("Create IPv4 Internet Stack");
  InternetStackHelper stack;
  stack.Install (node0);
  stack.Install (router);
  stack.Install (node2);

  NS_LOG_INFO ("Create networks and assign IPv4 Addresses.");
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1 = address.Assign (net_devs_all);

  // stackHelper.PrintRoutingTable (n0);
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (net_all.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (i1.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (net_all.Get (2));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  csma_p2p.EnablePcapAll ("N-G-N");

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
