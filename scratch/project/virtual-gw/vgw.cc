/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2017 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

#include "ns3/core-module.h"
#include "ns3/config-store.h"
#include "ns3/ipv4-address.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/nr-module.h"
#include "ns3/config-store-module.h"
#include "ns3/antenna-module.h"

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/lte-module.h"
#include "ns3/wifi-module.h"

#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"
#include <iostream>
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/core-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("vgw-echo");

void
PingRtt (std::string context, Time rtt)
{
  std::cout << context << " " << rtt << std::endl;
}

void
PrintRoutingTable (Ptr<Node> n)
{
  Ptr<Ipv4StaticRouting> routing = 0;
  Ipv4StaticRoutingHelper routingHelper;
  Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
  uint32_t nbRoutes = 0;
  Ipv4RoutingTableEntry route;

  routing = routingHelper.GetStaticRouting (ipv4);

  std::cout << "Routing table of " << n << " : " << std::endl;
  std::cout << "Destination "
            << "Gateway\t"
            << "Interface\t" << std::endl;

  nbRoutes = routing->GetNRoutes ();
  for (uint32_t i = 0; i < nbRoutes; i++)
    {
      route = routing->GetRoute (i);
      std::cout << route.GetDest () << "\t" << route.GetGateway () << "\t" << route.GetInterface ()
                << std::endl;
    }
}

int
multi_proto ()
{
  std::cout << "\nNetwork Topology setup" << std::endl;
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // Mobiltiy
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (10.0), "MinY",
                                 DoubleValue (10.0), "DeltaX", DoubleValue (5.0), "DeltaY",
                                 DoubleValue (2.0), "GridWidth", UintegerValue (5), "LayoutType",
                                 StringValue ("RowFirst"));

  // NR  =======================================================================================================S
  double simTime = 10; // seconds
  double centralFrequency = 7e9;
  double bandwidth = 100e6;

  // setup the nr simulation
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

  CcBwpCreator nr_ccBwpCreator;
  const uint8_t numCcPerBand = 1; // in this example, both bands have a single CC
  BandwidthPartInfo::Scenario nr_scenario = BandwidthPartInfo::RMa_LoS;
  CcBwpCreator::SimpleOperationBandConf nr_bandConf (centralFrequency, bandwidth, numCcPerBand,
                                                     nr_scenario);
  OperationBandInfo band = nr_ccBwpCreator.CreateOperationBandContiguousCc (nr_bandConf);
  nrHelper->InitializeOperationBand (&band);
  BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps ({band});

  // Create EPC helper
  Ptr<NrPointToPointEpcHelper> nr_epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  nrHelper->SetEpcHelper (nr_epcHelper);

  // Initialize nrHelper
  nrHelper->Initialize ();

  /*
   *  Create the gNB and UE nodes according to the network topology
   */
  NodeContainer nr_gNbNodes;
  NodeContainer nr_ueNodes;

  MobilityHelper nr_mobility;
  nr_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> nr_bsPositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> nr_utPositionAlloc = CreateObject<ListPositionAllocator> ();

  nr_gNbNodes.Create (1);
  nr_ueNodes.Create (1);

  // NR Mobility
  const double gNbHeight = 10;
  const double ueHeight = 1.5;
  nr_bsPositionAlloc->Add (Vector (0.0, 0.0, gNbHeight));
  nr_utPositionAlloc->Add (Vector (0.0, 30.0, ueHeight));
  nr_mobility.SetPositionAllocator (nr_bsPositionAlloc);
  nr_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  nr_mobility.Install (nr_gNbNodes);
  nr_mobility.SetPositionAllocator (nr_utPositionAlloc);
  nr_mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds",
                                RectangleValue (Rectangle (-50, 50, -25, 50)));
  nr_mobility.Install (nr_ueNodes);

  // Install nr net devices
  NetDeviceContainer nr_gNbNetDev = nrHelper->InstallGnbDevice (nr_gNbNodes, allBwps);
  NetDeviceContainer nr_ueNetDev = nrHelper->InstallUeDevice (nr_ueNodes, allBwps);

  // When all the configuration is done, explicitly call UpdateConfig ()

  for (auto it = nr_gNbNetDev.Begin (); it != nr_gNbNetDev.End (); ++it)
    {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

  for (auto it = nr_ueNetDev.Begin (); it != nr_ueNetDev.End (); ++it)
    {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }

  // create the internet and install the IP stack on the UEs
  Ptr<Node> pgw = nr_epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // connect a remoteHost to pgw. Setup routing too
  PointToPointHelper nr_p2ph;
  nr_p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  nr_p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  nr_p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
  NetDeviceContainer nr_internetDevices = nr_p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper nr_ipv4h;
  nr_ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer nr_internetIpIfaces = nr_ipv4h.Assign (nr_internetDevices);
  Ipv4StaticRoutingHelper nr_ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      nr_ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  internet.Install (nr_ueNodes);

  Ipv4InterfaceContainer nr_ueIpIface =
      nr_epcHelper->AssignUeIpv4Address (NetDeviceContainer (nr_ueNetDev));

  // Set the default gateway for the UEs
  for (uint32_t j = 0; j < nr_ueNodes.GetN (); ++j)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          nr_ipv4RoutingHelper.GetStaticRouting (nr_ueNodes.Get (j)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (nr_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (nr_ueNetDev, nr_gNbNetDev);

  // Start applications on UEs and remote host
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (remoteHost);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper nr_echoClient (nr_internetIpIfaces.GetAddress (1), 9);
  nr_echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  nr_echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  nr_echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = nr_echoClient.Install (nr_ueNodes.Get (0));

  std::cout << "5G UE\t\t" << nr_ueIpIface.GetAddress (0, 0) << std::endl;
  Ptr<Ipv4> ipv4 = remoteHost->GetObject<Ipv4> ();
  std::cout << "remote host\t" << nr_internetIpIfaces.GetAddress (1) << std::endl;
  std::cout << "GNB # of netdevices " << nr_gNbNodes.Get (0)->GetNDevices () << std::endl;
  std::cout << "GNB\t" << nr_gNbNodes.Get (0)->GetObject<Ipv4> ()->GetAddress (0, 0).GetLocal ()
            << std::endl;
  std::cout << "GNB\t" << nr_gNbNodes.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
            << std::endl;

  std::cout << "pgw # of netdevices " << pgw->GetNDevices () << std::endl;
  for (uint32_t i = 0; i < pgw->GetNDevices (); ++i)
    {
      std::cout << "netdev " << i << "\t" << pgw->GetObject<Ipv4> ()->GetAddress (i, 0).GetLocal ()
                << std::endl;
    }

  std::cout << "default gateway\t" << nr_epcHelper->GetUeDefaultGatewayAddress () << std::endl;
  // NR  =======================================================================================================E

  // LTE =======================================================================================================S
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> lte_epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (lte_epcHelper);
  Ptr<Node> lte_pgw = lte_epcHelper->GetPgwNode ();

  InternetStackHelper lte_internet;

  // Create the Internet : RemoteHost <-> PGW
  PointToPointHelper lte_p2ph;
  lte_p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  lte_p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  lte_p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer lte_internetDevsPgwRemoteHost = lte_p2ph.Install (lte_pgw, remoteHost);

  NodeContainer lte_ueNodes;
  NodeContainer lte_enbNodes;
  lte_enbNodes.Create (1);
  lte_ueNodes.Create (1);

  // Install Mobility Model
  Ptr<ListPositionAllocator> lte_positionAlloc = CreateObject<ListPositionAllocator> ();
  // for (uint16_t i = 0; i < 2; i++)
  //   {
  //     lte_positionAlloc->Add (Vector (60.0 * i, 0, 0));
  //   }

  MobilityHelper lte_mobility;
  lte_mobility.SetPositionAllocator (nr_bsPositionAlloc);
  lte_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  lte_mobility.Install (lte_enbNodes);
  lte_mobility.SetPositionAllocator (nr_utPositionAlloc);
  lte_mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds",
                                 RectangleValue (Rectangle (-50, 50, -25, 50)));
  lte_mobility.Install (lte_ueNodes);

  // lte_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // lte_mobility.SetPositionAllocator (lte_positionAlloc);
  // lte_mobility.Install (lte_enbNodes);
  // lte_mobility.Install (lte_ueNodes);

  // Install the IP stack on the UEs
  lte_internet.Install (lte_ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer lte_enbLteDevs = lteHelper->InstallEnbDevice (lte_enbNodes);
  NetDeviceContainer lte_ueLteDevs = lteHelper->InstallUeDevice (lte_ueNodes);

  // Assign IP address to UEs
  Ipv4InterfaceContainer lte_ueIpIface;
  lte_ueIpIface = lte_epcHelper->AssignUeIpv4Address (NetDeviceContainer (lte_ueLteDevs));

  Ipv4StaticRoutingHelper lte_ipv4RoutingHelper;
  for (uint32_t u = 0; u < lte_ueNodes.GetN (); ++u)
    {
      Ptr<Node> lte_ueNode = lte_ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> lte_ueStaticRouting =
          lte_ipv4RoutingHelper.GetStaticRouting (lte_ueNode->GetObject<Ipv4> ());
      lte_ueStaticRouting->SetDefaultRoute (lte_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  lteHelper->Attach (lte_ueLteDevs.Get (0), lte_enbLteDevs.Get (0));

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("2.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer lte_internetIpIfaces = ipv4h.Assign (lte_internetDevsPgwRemoteHost);

  remoteHostStaticRouting = lte_ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("107.0.0.0"), Ipv4Mask ("255.255.255.0"),
                                              2);

  // Start applications on UEs and remote host
  // UdpEchoServerHelper echoServer (9);
  // ApplicationContainer serverApps = echoServer.Install (remoteHost);
  // serverApps.Start (Seconds (1.0));
  // serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient1 (lte_internetIpIfaces.GetAddress (1), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

  clientApps.Add (echoClient1.Install (lte_ueNodes.Get (0)));

  std::cout << "\nLTE UE\t\t" << lte_ueIpIface.GetAddress (0, 0) << std::endl;
  std::cout << "remote host\t" << lte_internetIpIfaces.GetAddress (1) << std::endl;

  std::cout << "pgw # of netdevices" << lte_pgw->GetNDevices () << std::endl;
  for (uint32_t i = 0; i < lte_pgw->GetNDevices (); ++i)
    {
      std::cout << "netdev " << i << "\t"
                << lte_pgw->GetObject<Ipv4> ()->GetAddress (i, 0).GetLocal () << std::endl;
    }
  std::cout << "default gateway\t" << lte_epcHelper->GetUeDefaultGatewayAddress () << std::endl;
  // LTE =======================================================================================================E

  // wifi=======================================================================================================S
  Ssid ssid = Ssid ("ns-3-ssid");

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_STANDARD_80211n);

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue ("HtMcs7"), "ControlMode",
                                      StringValue ("HtMcs7"));

  NodeContainer wifi_allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (1);

  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  wifi_allNodes.Add (wifiApNode);
  wifi_allNodes.Add (wifiStaNodes);

  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifiHelper.Install (wifiPhy, wifiMac, wifiApNode.Get (0));
  NetDeviceContainer wifi_devices = apDevice;
  // setup STA
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevice = wifiHelper.Install (wifiPhy, wifiMac, wifiStaNodes.Get (0));
  wifi_devices.Add (staDevice);

  MobilityHelper wifi_mobility;
  // wifi_mobility.Install (wifiApNode);
  // wifi_mobility.Install (wifiStaNodes);

  wifi_mobility.SetPositionAllocator (nr_bsPositionAlloc);
  wifi_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  wifi_mobility.Install (wifiApNode);
  wifi_mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds",
                                  RectangleValue (Rectangle (-50, 50, -25, 50)));
  wifi_mobility.SetPositionAllocator (nr_utPositionAlloc);

  wifi_mobility.Install (wifiStaNodes);

  InternetStackHelper wifi_internet;
  wifi_internet.Install (wifi_allNodes);
  Ipv4AddressHelper wifi_ipv4;
  wifi_ipv4.SetBase ("50.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer wifi_ipv4Address = wifi_ipv4.Assign (wifi_devices);

  PointToPointHelper wifi_p2ph;
  wifi_p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  wifi_p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  wifi_p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
  NetDeviceContainer wifi_p2p_internetDevices = wifi_p2ph.Install (wifiApNode.Get (0), remoteHost);
  Ipv4AddressHelper wifi_ipv4h;
  wifi_ipv4h.SetBase ("3.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer wifi_p2pIpIfaces = wifi_ipv4h.Assign (wifi_p2p_internetDevices);

  Ptr<Ipv4StaticRouting> staticRouting;
  staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting> (
      wifiStaNodes.Get (0)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("50.0.0.1", 1);

  staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting> (
      remoteHost->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->AddNetworkRouteTo (Ipv4Address ("50.0.0.0"), Ipv4Mask ("255.255.255.0"), 3);

  /* Populate routing table */
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // V4PingHelper ping = V4PingHelper (wifi_internetIpIfaces.GetAddress (1));
  // V4PingHelper ping = V4PingHelper (Ipv4Address ("3.0.0.2"));
  // NodeContainer pingers;
  // // pingers.Add (wifiApNode.Get (0));
  // pingers.Add (wifiStaNodes.Get (0));
  // ApplicationContainer pingApp;
  // pingApp = ping.Install (pingers);
  // pingApp.Start (Seconds (0.0));
  // pingApp.Stop (Seconds (5.0));

  UdpEchoClientHelper wifi_echoClient (wifi_p2pIpIfaces.GetAddress (1), 9);
  wifi_echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  wifi_echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  wifi_echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  clientApps.Add (wifi_echoClient.Install (wifiStaNodes.Get (0)));

  std::cout << "\nwifi AP\t" << wifi_ipv4Address.GetAddress (0) << std::endl;
  std::cout << "Station\t" << wifi_ipv4Address.GetAddress (1) << std::endl;
  std::cout << "remote host\t" << wifi_p2pIpIfaces.GetAddress (1) << std::endl << std::endl;

  // Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::V4Ping/Rtt", MakeCallback (&PingRtt));
  // wifi======================================================================================================E
  std::cout << "remoteHost # of netdevices " << remoteHost->GetNDevices () << std::endl;
  for (uint32_t i = 0; i < remoteHost->GetNDevices (); ++i)
    {
      std::cout << "netdev " << i << "\t"
                << remoteHost->GetObject<Ipv4> ()->GetAddress (i, 0).GetLocal () << std::endl;
    }
  // std::cout << "default gateway\t" << lte_epcHelper->GetUeDefaultGatewayAddress () << std::endl;

  // PrintRoutingTable (remoteHost);
  // PrintRoutingTable (pgw);
  // PrintRoutingTable (lte_pgw);

  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (8.0));

  std::cout << "\nStart Simulation" << std::endl;
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  Simulator::Destroy ();
  std::cout << "End Simulation" << std::endl;
  return 0;
}

//-----------------------------------------------------------------------------------------------------------------/
// Temporary code for convenience
int
nr_5g ()
{
  // enable logging or not
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  double simTime = 10; // seconds
  double centralFrequency = 7e9;
  double bandwidth = 100e6;

  // setup the nr simulation
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

  CcBwpCreator nr_ccBwpCreator;
  const uint8_t numCcPerBand = 1; // in this example, both bands have a single CC
  BandwidthPartInfo::Scenario nr_scenario = BandwidthPartInfo::RMa_LoS;
  CcBwpCreator::SimpleOperationBandConf nr_bandConf (centralFrequency, bandwidth, numCcPerBand,
                                                     nr_scenario);
  OperationBandInfo band = nr_ccBwpCreator.CreateOperationBandContiguousCc (nr_bandConf);
  nrHelper->InitializeOperationBand (&band);
  BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps ({band});

  // Create EPC helper
  Ptr<NrPointToPointEpcHelper> nr_epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  nrHelper->SetEpcHelper (nr_epcHelper);

  // Initialize nrHelper
  nrHelper->Initialize ();

  /*
   *  Create the gNB and UE nodes according to the network topology
   */
  NodeContainer nr_gNbNodes;
  NodeContainer nr_ueNodes;

  MobilityHelper nr_mobility;
  nr_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> nr_bsPositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> nr_utPositionAlloc = CreateObject<ListPositionAllocator> ();

  const double gNbHeight = 10;
  const double ueHeight = 1.5;

  nr_gNbNodes.Create (1);
  nr_ueNodes.Create (1);

  nr_mobility.Install (nr_gNbNodes);
  nr_mobility.Install (nr_ueNodes);
  nr_bsPositionAlloc->Add (Vector (0.0, 0.0, gNbHeight));
  nr_utPositionAlloc->Add (Vector (0.0, 30.0, ueHeight));

  nr_mobility.SetPositionAllocator (nr_bsPositionAlloc);
  nr_mobility.Install (nr_gNbNodes);

  nr_mobility.SetPositionAllocator (nr_utPositionAlloc);
  nr_mobility.Install (nr_ueNodes);

  // Install nr net devices
  NetDeviceContainer nr_gNbNetDev = nrHelper->InstallGnbDevice (nr_gNbNodes, allBwps);
  NetDeviceContainer nr_ueNetDev = nrHelper->InstallUeDevice (nr_ueNodes, allBwps);

  // When all the configuration is done, explicitly call UpdateConfig ()

  for (auto it = nr_gNbNetDev.Begin (); it != nr_gNbNetDev.End (); ++it)
    {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

  for (auto it = nr_ueNetDev.Begin (); it != nr_ueNetDev.End (); ++it)
    {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }

  // create the internet and install the IP stack on the UEs
  Ptr<Node> pgw = nr_epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // connect a remoteHost to pgw. Setup routing too
  PointToPointHelper nr_p2ph;
  nr_p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  nr_p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  nr_p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
  NetDeviceContainer nr_internetDevices = nr_p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper nr_ipv4h;
  nr_ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer nr_internetIpIfaces = nr_ipv4h.Assign (nr_internetDevices);
  Ipv4StaticRoutingHelper nr_ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      nr_ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  internet.Install (nr_ueNodes);

  Ipv4InterfaceContainer nr_ueIpIface =
      nr_epcHelper->AssignUeIpv4Address (NetDeviceContainer (nr_ueNetDev));

  // Set the default gateway for the UEs
  for (uint32_t j = 0; j < nr_ueNodes.GetN (); ++j)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          nr_ipv4RoutingHelper.GetStaticRouting (nr_ueNodes.Get (j)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (nr_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (nr_ueNetDev, nr_gNbNetDev);

  // Start applications on UEs and remote host
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (remoteHost);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient1 (nr_internetIpIfaces.GetAddress (1), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps1 = echoClient1.Install (nr_ueNodes.Get (0));
  clientApps1.Start (Seconds (1.0));
  clientApps1.Stop (Seconds (7.0));

  // Ipv4GlobalRoutingHelper::RecomputeRoutingTables ();

  std::cout << "5G UE\t\t" << nr_ueIpIface.GetAddress (0, 0) << std::endl;
  Ptr<Ipv4> ipv4 = remoteHost->GetObject<Ipv4> ();
  std::cout << "remote host\t" << nr_internetIpIfaces.GetAddress (1) << std::endl;
  std::cout << "GNB # of netdevices " << nr_gNbNodes.Get (0)->GetNDevices () << std::endl;
  std::cout << "GNB\t" << nr_gNbNodes.Get (0)->GetObject<Ipv4> ()->GetAddress (0, 0).GetLocal ()
            << std::endl;
  std::cout << "GNB\t" << nr_gNbNodes.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
            << std::endl;

  std::cout << "pgw # of netdevices " << pgw->GetNDevices () << std::endl;
  std::cout << " netdev 0\t" << pgw->GetObject<Ipv4> ()->GetAddress (0, 0).GetLocal () << std::endl;
  std::cout << " netdev 1\t" << pgw->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () << std::endl;
  std::cout << " netdev 2\t" << pgw->GetObject<Ipv4> ()->GetAddress (2, 0).GetLocal () << std::endl;
  std::cout << " netdev 3\t" << pgw->GetObject<Ipv4> ()->GetAddress (3, 0).GetLocal () << std::endl;
  std::cout << "default gateway\t" << nr_epcHelper->GetUeDefaultGatewayAddress () << std::endl;

  std::cout << "\nstart simulation" << std::endl;
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}

int
lte ()
{

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> lte_epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (lte_epcHelper);
  Ptr<Node> lte_pgw = lte_epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper lte_internet;
  lte_internet.Install (remoteHostContainer);

  // Create the Internet : RemoteHost <-> PGW
  PointToPointHelper lte_p2ph;
  lte_p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  lte_p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  lte_p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer lte_internetDevsPgwRemoteHost = lte_p2ph.Install (lte_pgw, remoteHost);

  NodeContainer lte_ueNodes;
  NodeContainer lte_enbNodes;
  lte_enbNodes.Create (1);
  lte_ueNodes.Create (1);

  // Install Mobility Model
  Ptr<ListPositionAllocator> lte_positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < 2; i++)
    {
      lte_positionAlloc->Add (Vector (60.0 * i, 0, 0));
    }
  MobilityHelper lte_mobility;
  lte_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  lte_mobility.SetPositionAllocator (lte_positionAlloc);
  lte_mobility.Install (lte_enbNodes);
  lte_mobility.Install (lte_ueNodes);

  // Install the IP stack on the UEs
  lte_internet.Install (lte_ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer lte_enbLteDevs = lteHelper->InstallEnbDevice (lte_enbNodes);
  NetDeviceContainer lte_ueLteDevs = lteHelper->InstallUeDevice (lte_ueNodes);

  // Assign IP address to UEs
  Ipv4InterfaceContainer lte_ueIpIface;
  lte_ueIpIface = lte_epcHelper->AssignUeIpv4Address (NetDeviceContainer (lte_ueLteDevs));

  Ipv4StaticRoutingHelper lte_ipv4RoutingHelper;
  for (uint32_t u = 0; u < lte_ueNodes.GetN (); ++u)
    {
      Ptr<Node> lte_ueNode = lte_ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> lte_ueStaticRouting =
          lte_ipv4RoutingHelper.GetStaticRouting (lte_ueNode->GetObject<Ipv4> ());
      lte_ueStaticRouting->SetDefaultRoute (lte_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  lteHelper->Attach (lte_ueLteDevs.Get (0), lte_enbLteDevs.Get (0));

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer lte_internetIpIfaces = ipv4h.Assign (lte_internetDevsPgwRemoteHost);

  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      lte_ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.255.0"),
                                              1);

  // Start applications on UEs and remote host
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (remoteHost);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient1 (lte_internetIpIfaces.GetAddress (1), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps1 = echoClient1.Install (lte_ueNodes.Get (0));
  clientApps1.Start (Seconds (1.0));
  clientApps1.Stop (Seconds (7.0));

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  std::cout << "LTE UE\t\t" << lte_ueIpIface.GetAddress (0, 0) << std::endl;
  Ptr<Ipv4> ipv4 = remoteHost->GetObject<Ipv4> ();
  std::cout << "remote host\t" << lte_internetIpIfaces.GetAddress (1) << std::endl;

  std::cout << "pgw # of netdevices" << lte_pgw->GetNDevices () << std::endl;
  std::cout << " netdev 0\t" << lte_pgw->GetObject<Ipv4> ()->GetAddress (0, 0).GetLocal ()
            << std::endl;
  std::cout << " netdev 1\t" << lte_pgw->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
            << std::endl;
  std::cout << " netdev 2\t" << lte_pgw->GetObject<Ipv4> ()->GetAddress (2, 0).GetLocal ()
            << std::endl;
  std::cout << " netdev 3\t" << lte_pgw->GetObject<Ipv4> ()->GetAddress (3, 0).GetLocal ()
            << std::endl;
  std::cout << "default gateway\t" << lte_epcHelper->GetUeDefaultGatewayAddress () << std::endl;

  Simulator::Stop (Seconds (10));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}

int
wifi ()
{
  double simulationTime = 10;
  std::string phyRate = "HtMcs7";

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  Ssid ssid = Ssid ("ns-3-ssid");

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_STANDARD_80211n);

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue ("HtMcs7"), "ControlMode",
                                      StringValue ("HtMcs7"));

  NodeContainer wifi_allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (1);

  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  wifi_allNodes.Add (wifiApNode);
  wifi_allNodes.Add (wifiStaNodes);

  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifiHelper.Install (wifiPhy, wifiMac, wifiApNode.Get (0));
  NetDeviceContainer wifi_devices = apDevice;
  // setup STA
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevice = wifiHelper.Install (wifiPhy, wifiMac, wifiStaNodes.Get (0));
  wifi_devices.Add (staDevice);

  MobilityHelper wifi_mobility;
  wifi_mobility.Install (wifiApNode);
  wifi_mobility.Install (wifiStaNodes);

  InternetStackHelper wifi_internet;
  wifi_internet.Install (wifi_allNodes);
  Ipv4AddressHelper wifi_ipv4;
  wifi_ipv4.SetBase ("50.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipv4Address = wifi_ipv4.Assign (wifi_devices);

  NodeContainer c;
  c.Create (1);
  InternetStackHelper internet;
  internet.Install (c);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer p2p_devices = p2p.Install (wifiApNode.Get (0), c.Get (0));
  Ipv4AddressHelper nr_ipv4h;
  nr_ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer nr_internetIpIfaces = nr_ipv4h.Assign (p2p_devices);

  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Ptr<Ipv4StaticRouting> staticRouting;
  staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting> (
      wifiStaNodes.Get (0)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("50.0.0.1", 1);

  staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting> (
      c.Get (0)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("1.0.0.1", 1);

  /* Creating application */
  // remotehost to sta
  // UdpEchoServerHelper echoServer (9);
  // ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (0));
  // serverApps.Start (Seconds (1.0));
  // serverApps.Stop (Seconds (5.0));
  // UdpEchoClientHelper wifi_echoClient (Ipv4Address ("50.0.0.2"), 9);
  // wifi_echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  // wifi_echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  // wifi_echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  // ApplicationContainer wifi_clientApps = wifi_echoClient.Install (c.Get (0));
  // wifi_clientApps.Start (Seconds (1.0));
  // wifi_clientApps.Stop (Seconds (6.0));

  // sta to p2p = OK
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (c.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (5.0));
  UdpEchoClientHelper wifi_echoClient (Ipv4Address ("1.0.0.2"), 9);
  wifi_echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  wifi_echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  wifi_echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer wifi_clientApps = wifi_echoClient.Install (wifiStaNodes.Get (0));
  wifi_clientApps.Start (Seconds (1.0));
  wifi_clientApps.Stop (Seconds (6.0));

  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  std::cout << "AP\t" << ipv4Address.GetAddress (0) << std::endl;
  std::cout << "Station\t" << ipv4Address.GetAddress (1) << std::endl;
  std::cout << "p2p\t" << nr_internetIpIfaces.GetAddress (0) << std::endl;
  std::cout << "p2p\t" << nr_internetIpIfaces.GetAddress (1) << std::endl;

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}

int
main (int argc, char **argv)
{
  multi_proto ();
  // wifi ();
  // nr_5g ();
  // lte ();
  return 0;
}
