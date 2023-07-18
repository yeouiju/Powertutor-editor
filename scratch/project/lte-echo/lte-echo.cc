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
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"

using namespace ns3;

/**
 * LTE EPC udp echo : uE-eNodeB  <-> EPC(PGW) <-> RemoteHost 
 * UE(User Equipment) : 사용자 단말
 * eNodeB(Evolved Node B) : 기지국
 * EPC(Evolved Packet Core)
 *  - MME(Mobility Management Entity) : UE의 위치 정보를 관리, UE의 인증을 담당
 *  - SGW(Serving Gateway) : Handover를 담당
 *  - PGW(PDN Gateway) : UE의 PDN(Packet Data Network, 인터넷) 연결을 담당
 *  - HSS(Homogeneous Subscriber Service) : 가입자 정보
 */

NS_LOG_COMPONENT_DEFINE ("LTE-echo");

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet : RemoteHost <-> PGW
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevsPgwRemoteHost = p2ph.Install (pgw, remoteHost);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (1);
  ueNodes.Create (1);

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
  internet.Install (ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Assign IP address to UEs
  Ipv4InterfaceContainer ueIpIface;
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

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevsPgwRemoteHost);

  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.255.0"),
                                              1);

  // Start applications on UEs and remote host
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (remoteHost);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient1 (internetIpIfaces.GetAddress (1), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps1 = echoClient1.Install (ueNodes.Get (0));
  clientApps1.Start (Seconds (1.0));
  clientApps1.Stop (Seconds (7.0));

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  std::cout << "UE\t\t" << ueIpIface.GetAddress (0, 0) << std::endl;
  Ptr<Ipv4> ipv4 = remoteHost->GetObject<Ipv4> ();
  std::cout << "remote host\t" << internetIpIfaces.GetAddress (1) << std::endl;

  std::cout << "pgw # of netdevices" << pgw->GetNDevices () << std::endl;
  std::cout << " netdev 0\t" << pgw->GetObject<Ipv4> ()->GetAddress (0, 0).GetLocal () << std::endl;
  std::cout << " netdev 1\t" << pgw->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () << std::endl;
  std::cout << " netdev 2\t" << pgw->GetObject<Ipv4> ()->GetAddress (2, 0).GetLocal () << std::endl;
  std::cout << " netdev 3\t" << pgw->GetObject<Ipv4> ()->GetAddress (3, 0).GetLocal () << std::endl;
  std::cout << "default gateway\t" << epcHelper->GetUeDefaultGatewayAddress () << std::endl;

  Simulator::Stop (Seconds (10));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
