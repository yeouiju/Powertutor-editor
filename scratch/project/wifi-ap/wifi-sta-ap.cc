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

//                   n0            n1
//               +--------+    +--------+
//               |        |    |        |
//               |  wifi  |    |  wifi  |
//               +--------+    +--------+
//                   |             |
//                 ((*))         ((*))
//
//                       Wifi LAN
//
//                        ((*))
//                          |
//                     +--------+
//                     |  wifi  |
//                     +--------+
//                     | access |
//                     |  point |
//                     +--------+

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/node-container.h"
#include "ns3/wifi-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapWifiAp");

void
Monitor (std::string context, Ptr<const Packet> pkt, uint16_t channel, WifiTxVector tx,
         MpduInfo mpdu, SignalNoiseDbm snr, uint16_t sta_id)
{
  std::cout << "Monitor: " << context << std::endl;
  std::cout << "\t Channel: " << channel << " Tx: " << tx.GetMode () << "\t Signal=" << snr.signal
            << "\t Noise=" << snr.noise << "\t Station=" << sta_id << std::endl;

  WifiMacHeader hdr;
  if (pkt->PeekHeader (hdr))
    {
      std::cout << "\t From: " << hdr.GetAddr2 () << " To: " << hdr.GetAddr1 () << std::endl;
    }
}

void
DevRxTrace (std::string context, Ptr<const Packet> p)
{

  std::cout << " RX p: " << *p << std::endl;
}

void
ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      std::cout << "Received one packet!" << std::endl;
    }
}

Ptr<PacketSink> sink; /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0; /* The value of the last total received bytes */

void
CalculateThroughput ()
{
  Time now = Simulator::Now (); /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 /
               1e5; /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();

  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  // bool pcapTracing = false;

  uint32_t payloadSize = 1472;
  std::string dataRate = "100Mbps"; /* Application layer datarate. */
  double simulationTime = 3;
  Time simulationEndTime = Seconds (3);
  Time interval = Seconds (1.0);
  std::string phyRate = "HtMcs7";
  std::string animFile = "tap-ap-animation.xml";
  bool verbose = false;
  if (verbose)
    {
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
      LogComponentEnable ("OnOffApplication", ns3::LOG_LEVEL_ALL);
    }

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

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
                                      StringValue (phyRate), "ControlMode", StringValue ("HtMcs0"));

  NodeContainer allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (2);

  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  allNodes.Add (wifiApNode);
  allNodes.Add (wifiStaNodes);

  // wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifiHelper.Install (wifiPhy, wifiMac, wifiApNode.Get (0));
  NetDeviceContainer devices = apDevice;
  // setup STA
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevice = wifiHelper.Install (wifiPhy, wifiMac, wifiStaNodes.Get (0));
  devices.Add (staDevice);
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevice2 = wifiHelper.Install (wifiPhy, wifiMac, wifiStaNodes.Get (1));
  devices.Add (staDevice2);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (10.0), "MinY",
                                 DoubleValue (10.0), "DeltaX", DoubleValue (5.0), "DeltaY",
                                 DoubleValue (2.0), "GridWidth", UintegerValue (5), "LayoutType",
                                 StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds",
                             RectangleValue (Rectangle (-50, 50, -25, 50)));
  mobility.Install (wifiApNode);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  // AnimationInterface::SetConstantPosition (wifiApNode.Get (0), 10, 30);

  InternetStackHelper internet;
  internet.Install (allNodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ipv4Address = ipv4.Assign (devices);
  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Install TCP Receiver on the access point */
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                               InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (wifiStaNodes.Get (0));
  sink = StaticCast<PacketSink> (sinkApp.Get (0));

  /* Install TCP/UDP Transmitter on the station */
  OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (ipv4Address.GetAddress (1), 9)));
  server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  ApplicationContainer serverApp = server.Install (wifiApNode.Get (0));

  /* Start Applications */
  sinkApp.Start (Seconds (0.0));
  serverApp.Start (Seconds (1.0));

  // Create the animation object and configure for specified output
  // AnimationInterface anim (animFile);
  // for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i)
  //   {
  //     anim.UpdateNodeDescription (wifiStaNodes.Get (i), "STA"); // Optional
  //     anim.UpdateNodeColor (wifiStaNodes.Get (i), 255, 0, 100); // Optional
  //   }

  // for (uint32_t i = 0; i < wifiApNode.GetN (); ++i)
  //   {
  //     anim.UpdateNodeDescription (wifiApNode.Get (i), "AP"); // Optional
  //     anim.UpdateNodeColor (wifiApNode.Get (i), 10, 8, 10); // Optional
  //   }

  // anim.EnablePacketMetadata (); // Optional
  // anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10));
  // anim.EnableWifiMacCounters (Seconds (0), Seconds (10));
  // anim.EnableWifiPhyCounters (Seconds (0), Seconds (10));

  // wifiPhy.EnablePcapAll ("tap-ap", false);

  // Packet::EnablePrinting ();
  // Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
  //                  MakeCallback (&Monitor));
  // Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacRx", MakeCallback (&DevRxTrace));

  std::cout << "AP\t" << ipv4Address.GetAddress (0) << std::endl;
  std::cout << "Sta1\t" << ipv4Address.GetAddress (1) << std::endl;
  std::cout << "Sta2\t" << ipv4Address.GetAddress (2) << std::endl;

  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin ();
       i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress
                << ")\n";
      std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
      std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
      std::cout << "  TxOffered:  "
                << i->second.txBytes * 8.0 / simulationEndTime.GetSeconds () / 1000 / 1000
                << " Mbps\n";
      std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      std::cout << "  Throughput: "
                << i->second.rxBytes * 8.0 / simulationEndTime.GetSeconds () / 1000 / 1000
                << " Mbps\n";
    }

  double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));
  Simulator::Destroy ();

  if (averageThroughput < 50)
    {
      NS_LOG_ERROR ("Obtained throughput is not in the expected boundaries!");
      exit (1);
    }
  std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;
}
