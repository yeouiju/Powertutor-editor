/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Jadavpur University, India
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
 * Author: Manoj Kumar Rana <manoj24.rana@gmail.com>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv6-static-routing.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/csma-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UE");

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (2);
  ueNodes.Create (2);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < 2; i++)
    {
      positionAlloc->Add (Vector (60.0 * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (enbNodes);
  mobility.Install (ueNodes);

  // Install the IP stack on the UEs
  InternetStackHelper internet;
  internet.Install (ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  Ipv4InterfaceContainer ueIpIface;
  // Assign IP address to UEs
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB

  lteHelper->Attach (ueLteDevs.Get (0), enbLteDevs.Get (0));
  lteHelper->Attach (ueLteDevs.Get (1), enbLteDevs.Get (1));

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  Ptr<Node> ueNode = ueNodes.Get (0);

  // ghost Node
  NodeContainer nodes;
  nodes.Create (1);
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer devices = csma.Install (nodes);

  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue ("UseLocal"));
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap0"));
  tapBridge.Install (nodes.Get (0), devices.Get (0));
  // ueNode->AddDevice (devices.Get (0));

  Ptr<Ipv4StaticRouting> ueStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

  // NS_LOG_UNCOND ("p2p address " << p2p_interfaces.GetAddress (0));
  // NS_LOG_UNCOND ("p2p address " << p2p_interfaces.GetAddress (1));
  // NS_LOG_UNCOND ("p2p address " << ghostDevices.Get (1)->GetAddress ());
  NS_LOG_UNCOND ("PGW " << pgw->GetId () << " addr:\t" << epcHelper->GetUeDefaultGatewayAddress ());
  NS_LOG_UNCOND ("UE " << ueNodes.Get (0)->GetId () << " addr:\t" << ueIpIface.GetAddress (0));
  NS_LOG_UNCOND ("UE " << ueNodes.Get (1)->GetId () << " addr:\t" << ueIpIface.GetAddress (1));

  // Start applications on UEs and remote host
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (ueNodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (100.0));

  UdpEchoClientHelper echoClient (ueIpIface.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (ueNodes.Get (0));
  clientApps.Start (Seconds (1.5));
  clientApps.Stop (Seconds (14.5));

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);

  internet.EnablePcapIpv4 ("LenaIpv6-Ue-Ue-Ue0.pcap", ueNodes.Get (0)->GetId (), 1, true);
  internet.EnablePcapIpv4 ("LenaIpv6-Ue-Ue-Ue1.pcap", ueNodes.Get (1)->GetId (), 1, true);

  // Simulator::Stop (Seconds (100));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}