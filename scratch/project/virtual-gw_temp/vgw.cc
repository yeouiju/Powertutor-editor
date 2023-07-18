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

  double simTime = 3; // seconds


    // make nodes in nodecontainers  ===================================================S
    std::cout << "\n(vgw.cc) make nodeContainers and declare variables of nodes...";
    NodeContainer remoteHostContainer;
    NodeContainer gwNodeContainer;
    NodeContainer boatClientContainer;
    NodeContainer nrNodeContainer;
    NodeContainer nrGnbNodeContainer;
    NodeContainer lteNodeContainer;
    NodeContainer lteEnbNodeContainer;

    remoteHostContainer.Create(1);
    gwNodeContainer.Create(1);
    boatClientContainer.Create(1);
    nrGnbNodeContainer.Create(1);
    lteEnbNodeContainer.Create(1);

    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    Ptr<Node> gw = gwNodeContainer.Get (0);
    Ptr<Node> boatClient = boatClientContainer.Get (0);

    Ptr<NrPointToPointEpcHelper> nrP2pEpcHpr = CreateObject<NrPointToPointEpcHelper>();
    Ptr<NrHelper> nrHpr = CreateObject<NrHelper> ();
    nrHpr->SetEpcHelper(nrP2pEpcHpr);
    nrHpr->Initialize();
    Ptr<Node> nr_PGW = nrP2pEpcHpr->GetPgwNode();
    Ptr<Node> nr_SGW = nrP2pEpcHpr->GetSgwNode();
    Ptr<Node> nr_MME = nrP2pEpcHpr->GetMmeNode();
    Ptr<Node> nr_gNb = nrGnbNodeContainer.Get(0);
    nrNodeContainer.Add(nr_PGW);
    nrNodeContainer.Add(nr_SGW);
    nrNodeContainer.Add(nr_MME);

    Ptr<LteHelper> lteHpr = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper> lteEpcHpr = CreateObject<PointToPointEpcHelper>();
    lteHpr->SetEpcHelper(lteEpcHpr);
    lteHpr->Initialize();
    Ptr<Node> lte_PGW = lteEpcHpr->GetPgwNode();
    Ptr<Node> lte_SGW = lteEpcHpr->GetSgwNode();
    Ptr<Node> lte_MME = lteEpcHpr->GetMmeNode();
    Ptr<Node> lte_eNb = lteEnbNodeContainer.Get(0);
    lteNodeContainer.Add(lte_PGW);
    lteNodeContainer.Add(lte_SGW);
    lteNodeContainer.Add(lte_MME);

    std::cout << "Done" << std::endl;
    // =================================================================================E



    // set mobilities of nodes =========================================================S
    std::cout << "(vgw.cc) install mobilities...";
    MobilityHelper mobilityHpr;
    Ptr<ListPositionAllocator> nrEpcPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> lteEpcPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> gwPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> boatClientPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> remoteHostPositionAlloc = CreateObject<ListPositionAllocator> ();

    nrEpcPositionAlloc->Add(Vector(15.0, 13.0, 10.0));      // nr_gNb
    nrEpcPositionAlloc->Add(Vector(20.0, 10.0, 10.0));      // nr_PGW
    nrEpcPositionAlloc->Add(Vector(20.0, 13.0, 10.0));      // nr_SGW
    nrEpcPositionAlloc->Add(Vector(22.0, 16.0, 10.0));      // nr_MME
    lteEpcPositionAlloc->Add(Vector (15.0, 28.0, 10.0));    // lte_eNb
    lteEpcPositionAlloc->Add(Vector (20.0, 25.0, 10.0));    // lte_PGW
    lteEpcPositionAlloc->Add(Vector (20.0, 28.0, 10.0));    // lte_SGW
    lteEpcPositionAlloc->Add(Vector (22.0, 31.0, 10.0));    // lte_MME
    gwPositionAlloc->Add(Vector (5.0, 25.0, 10.0));         // gw
    boatClientPositionAlloc->Add(Vector (0.0, 25.0, 10.0)); // boatClient
    remoteHostPositionAlloc->Add(Vector(30.0, 25.0, 10.0)); // remoteHost

    mobilityHpr.SetPositionAllocator (nrEpcPositionAlloc);
    mobilityHpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityHpr.Install (nr_gNb);
    mobilityHpr.Install (nr_PGW);
    mobilityHpr.Install (nr_SGW);
    mobilityHpr.Install (nr_MME);
    mobilityHpr.SetPositionAllocator (lteEpcPositionAlloc);
    mobilityHpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityHpr.Install (lte_eNb);
    mobilityHpr.Install (lte_PGW);
    mobilityHpr.Install (lte_SGW);
    mobilityHpr.Install (lte_MME);
    mobilityHpr.SetPositionAllocator (gwPositionAlloc);
    mobilityHpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityHpr.Install (gw);
    mobilityHpr.SetPositionAllocator (boatClientPositionAlloc);
    mobilityHpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityHpr.Install (boatClient);
    mobilityHpr.SetPositionAllocator (remoteHostPositionAlloc);
    mobilityHpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityHpr.Install (remoteHost);

    std::cout << "Done" << std::endl;
    // =================================================================================E



    // install Internet Stack to nodes =================================================S
    std::cout << "(vgw.cc) install Internet Stack...";

    InternetStackHelper internetStackHpr;
    internetStackHpr.Install (remoteHostContainer);
    internetStackHpr.Install (gwNodeContainer);
    internetStackHpr.Install (boatClientContainer);
    // internetStackHpr.Install (nrGnbNodeContainer);
    // internetStackHpr.Install (lteEnbNodeContainer);
    // internetStackHpr.Install (nrNodeContainer);
    // internetStackHpr.Install (lteNodeContainer);

    std::cout << "Done" << std::endl;
    // =================================================================================E


  

    // install netdevices to nodes =====================================================S
    std::cout << "(vgw.cc) install Netdevices...";
    PointToPointHelper nrToRemoteP2pHpr;
    PointToPointHelper lteToRemoteP2pHpr;
    PointToPointHelper boatClientToGwP2pHpr;

    nrToRemoteP2pHpr.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    nrToRemoteP2pHpr.SetDeviceAttribute ("Mtu", UintegerValue (2500));
    nrToRemoteP2pHpr.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
    NetDeviceContainer nrToRemoteNetDevices = nrToRemoteP2pHpr.Install(nr_PGW, remoteHost);       // NR PGW와 remotehost에 각각 netdevice 설치하고 연결 (pointTopoint 연결)
    NetDeviceContainer lteToRemoteNetDevices = lteToRemoteP2pHpr.Install(lte_PGW, remoteHost);    // LTE PGW와 remotehost에 각각 netdevice 설치하고 연결 (pointTopoint 연결)
    NetDeviceContainer boatClientToGwNetDevices = boatClientToGwP2pHpr.Install(gw, boatClient);   // Boat Client와 GW에 각각 netdevice 설치하고 연결 (pointTopoint 연결)

    // NR Carrier 설정
    double centralFrequency = 7e9;
    double bandwidth = 100e6;
    CcBwpCreator nr_ccBwpCreator;
    const uint8_t numCcPerBand = 1; // in this example, both bands have a single CC
    BandwidthPartInfo::Scenario nr_scenario = BandwidthPartInfo::RMa_LoS;
    CcBwpCreator::SimpleOperationBandConf nr_bandConf (centralFrequency, bandwidth, numCcPerBand, nr_scenario);
    OperationBandInfo band = nr_ccBwpCreator.CreateOperationBandContiguousCc (nr_bandConf);
    nrHpr->InitializeOperationBand (&band);
    BandwidthPartInfoPtrVector nr_allBwps = CcBwpCreator::GetAllBwps ({band});

    NetDeviceContainer gwNrUeDevices = nrHpr->InstallUeDevice(gwNodeContainer, nr_allBwps);       // GW node에 NR UE netdevice 설치
    NetDeviceContainer nrGnbNetDevices = nrHpr->InstallGnbDevice(nrGnbNodeContainer, nr_allBwps); // NR gNb node에 netdevice 설치

    for (auto it = nrGnbNetDevices.Begin (); it != nrGnbNetDevices.End (); ++it) {
        DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

    for (auto it = gwNrUeDevices.Begin (); it != gwNrUeDevices.End (); ++it) {
        DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }
    // NR netdevice 설치
    nrHpr->AttachToEnb(gwNrUeDevices.Get(0), nrGnbNetDevices.Get(0));                             // GW node(NR UE) <-> NR gNb 연결

    // LTE netdevice 설치
    NetDeviceContainer gwLteUeDevices = lteHpr->InstallUeDevice(gwNodeContainer);                 // GW node에 LTE UE netdevice 설치
    NetDeviceContainer lteEnbNetDevices = lteHpr->InstallEnbDevice(lteEnbNodeContainer);          // LTE eNb node에 netdevice 설치
                                  // GW node(LTE UE) <-> LTE eNb 연결

    std::cout << "Done" << std::endl;
    // =================================================================================E


    // set IPv4 Address to nodes =======================================================S
    std::cout << "(vgw.cc) install IPv4...";
    Ipv4AddressHelper nrAddressHpr;
    nrAddressHpr.SetBase ("1.0.0.0", "255.255.255.0");
    Ipv4AddressHelper lteAddressHpr;
    lteAddressHpr.SetBase ("2.0.0.0", "255.255.255.0");
    Ipv4AddressHelper boatClientAddressHpr;
    boatClientAddressHpr.SetBase ("9.0.0.0", "255.255.255.0");

    Ipv4InterfaceContainer gwNrUeInterfaces = nrAddressHpr.Assign (gwNrUeDevices);                // 0:GW(NR UE)
    Ipv4InterfaceContainer nrGnbInterfaces = nrAddressHpr.Assign (nrGnbNetDevices);               // 0:nr_gNb
    Ipv4InterfaceContainer nrToRemoteInterfaces = nrAddressHpr.Assign (nrToRemoteNetDevices);     // 0:nr_PGW, 1:remoteHost
    // Ipv4InterfaceContainer gwLteUeInterfaces = lteAddressHpr.Assign (gwLteUeDevices);             // 0:GW(LTE UE)
    Ipv4InterfaceContainer gwLteUeInterfaces  = lteEpcHpr->AssignUeIpv4Address (gwLteUeDevices);

    // Ipv4InterfaceContainer lteEnbInterfaces = lteAddressHpr.Assign (lteEnbNetDevices);            // 0:nr_eNb
    Ipv4InterfaceContainer lteToRemoteInterfaces = lteAddressHpr.Assign (lteToRemoteNetDevices);  // 0:lte_PGW, 1:remoteHost
    Ipv4InterfaceContainer boatToGwInterfaces = boatClientAddressHpr.Assign (boatClientToGwNetDevices);  // 0:GW, 1:boatClient


    std::cout << "Done" << std::endl;
    // =================================================================================E

    // set routing =====================================================================S
    
    Ipv4StaticRoutingHelper lte_ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> lte_ueStaticRouting = lte_ipv4RoutingHelper.GetStaticRouting (gwLteUeInterfaces.Get(0).first->GetObject<Ipv4> ());
    lte_ueStaticRouting->SetDefaultRoute (lteEpcHpr->GetUeDefaultGatewayAddress (), 0);
    
    lteHpr->Attach(gwLteUeDevices.Get(0), lteEnbNetDevices.Get(0)); 
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = lte_ipv4RoutingHelper.GetStaticRouting (lteToRemoteInterfaces.Get(1).first->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("107.0.0.0"), Ipv4Mask ("255.255.255.0"),1);

    // Ipv4StaticRoutingHelper lteRoutingHpr;
    // lteRoutingHpr.AddMulticastRoute(remoteHost, gwLteUeInterfaces.GetAddress(0), Ipv4Address("224.0.0.1"), nrToRemoteNetDevices.Get(1), nrToRemoteNetDevices.Get(1));
    // =================================================================================E

    // Install applications into nodes =================================================S
    std::cout << "(vgw.cc) install Apps...";
    // UdpEchoServerHelper   echoServer_forNR (1);   // port: 1
    UdpEchoServerHelper   echoServer_forLTE (2);  // port: 2

    //remotehost에 Echo서버 2개 설치
    ApplicationContainer remoteServerApps = echoServer_forLTE.Install (remoteHost);
    // remoteServerApps.Add((echoServer_forNR.Install (remoteHost)));


    // Time simulation_time("1s");
    // Ptr<UdpEchoClient> gwToNrEchoClient = CreateObject<UdpEchoClient> ();
    // gwToNrEchoClient->SetRemote(nrToRemoteInterfaces.GetAddress(1), 1);
    // gwToNrEchoClient->SetFill(1, 1);
    // gwToNrEchoClient->SetStartTime(simulation_time);
    // gwToNrEchoClient->SetNode(gw);

    // UdpEchoClientHelper gwToNrEchoClientHpr(nrToRemoteInterfaces.GetAddress(1),2);
    // gwToNrEchoClientHpr.SetAttribute ("MaxPackets", UintegerValue (1));
    // gwToNrEchoClientHpr.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    // gwToNrEchoClientHpr.SetAttribute ("PacketSize", UintegerValue (111));
    UdpEchoClientHelper gwToLteEchoClientHpr(lteToRemoteInterfaces.GetAddress(1), 2);
    gwToLteEchoClientHpr.SetAttribute ("MaxPackets", UintegerValue (1));
    gwToLteEchoClientHpr.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    gwToLteEchoClientHpr.SetAttribute ("PacketSize", UintegerValue (222));


    ApplicationContainer gwClientApps = gwToLteEchoClientHpr.Install (gw);
    // ApplicationContainer gwClientApps = Add(gwToNrEchoClientHpr.Install (gw));

    std::cout << "Done" << std::endl;
  // =================================================================================E



  // std::cout << "remoteHost # of netdevices " << remoteHost->GetNDevices () << std::endl;
  // for (uint32_t i = 0; i < remoteHost->GetNDevices (); ++i)
  //   {
  //     std::cout << "netdev " << i << "\t"
  //               << remoteHost->GetObject<Ipv4> ()->GetAddress (i, 0).GetLocal () << std::endl;
  //   }
  // // std::cout << "default gateway\t" << lte_epcHelper->GetUeDefaultGatewayAddress () << std::endl;

  // // PrintRoutingTable (remoteHost);
  // // PrintRoutingTable (pgw);
  // // PrintRoutingTable (lte_pgw);

  // // fill data in packets
  // uint8_t nrClientFillData = 1;
  // uint32_t nrClientDataLength = 111;
  // nr_echoClient.SetFill(clientApps.Get(0), nrClientFillData, nrClientDataLength);

  // uint8_t lteClientFillData = 2;
  // uint32_t lteClientDataLength = 222;
  // lte_echoClient.SetFill(clientApps.Get(1), lteClientFillData, lteClientDataLength);

  // uint8_t wifiClientFillData = 3;
  // uint32_t wifiClientDataLength = 333;
  // wifi_echoClient.SetFill(clientApps.Get(2), wifiClientFillData, wifiClientDataLength);

  // start applications
  // boatClientApp.Start (Seconds (2.0));
  // boatClientApp.Stop (Seconds (10.0));
  // gwServerApps.Start (Seconds (1.0));
  // gwServerApps.Stop (Seconds (10.0));
  remoteServerApps.Start (Seconds (1.0));
  remoteServerApps.Stop (Seconds (3.0));
  gwClientApps.Start (Seconds (1.0));
  gwClientApps.Stop (Seconds (2.0));

  // nr_p2ph.EnablePcapAll("pcaps/nr_p2ph_pcap_rev", true);
  // lte_p2ph.EnablePcapAll("pcaps/lte_p2ph_pcap_rev", true);
  // wifi_p2ph.EnablePcapAll("pcaps/wifi_p2ph_pcap_rev", true);
  // wifiPhy.EnablePcapAll("pcaps/wifi_phy_pcap_rev", true);

  AnimationInterface anim_interface("anim.xml");

  std::cout << "\nStart Simulation" << std::endl;
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  Simulator::Destroy ();
  std::cout << "End Simulation" << std::endl;



  return 0;
}

int
main (int argc, char **argv)
{
  multi_proto ();
  return 0;
}
