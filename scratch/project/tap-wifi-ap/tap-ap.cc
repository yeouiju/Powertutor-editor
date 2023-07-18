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

//  +----------+                           +----------+
//  |  Linux   |                           |  Linux   |
//  |  App     |                           |  App     |
//  +----------+                           +----------+
//       |                                      |
//  +------------+                       +-------------+
//  | "tap-left" |                       | "tap-right" |
//  +------------+                       +-------------+
//       |           n0            n1           |
//       |       +--------+    +--------+       |
//       +-------|  tap   |    |  tap   |-------+
//               | bridge |    | bridge |
//               +--------+    +--------+
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
//
#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/wifi-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/wifi-mac-header.h"

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

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  double rss = -80; // -dBm
  // uint32_t packetSize = 1000; // bytes
  // uint32_t numPackets = 1;
  Time interval = Seconds (1.0);
  // bool verbose = false;
  std::string phyMode ("DsssRate1Mbps");
  std::string animFile = "tap-ap-animation.xml";
  bool verbose = true;
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", ns3::LOG_LEVEL_ALL);
    }

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (3);

  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy;
  wifiPhy.Set ("RxGain", DoubleValue (0));
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel", "Rss", DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiMacHelper wifiMac;
  Ssid ssid = Ssid ("ns-3-ssid");
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode),
                                "ControlMode", StringValue (phyMode));

  // wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, wifiStaNodes.Get (0));
  NetDeviceContainer devices = apDevice;
  // setup STA
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, wifiStaNodes.Get (1));
  devices.Add (staDevice);
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevice2 = wifi.Install (wifiPhy, wifiMac, wifiStaNodes.Get (2));
  devices.Add (staDevice2);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (10.0), "MinY",
                                 DoubleValue (10.0), "DeltaX", DoubleValue (5.0), "DeltaY",
                                 DoubleValue (2.0), "GridWidth", UintegerValue (5), "LayoutType",
                                 StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds",
                             RectangleValue (Rectangle (-50, 50, -25, 50)));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  // mobility.Install (wifiApNode);
  // AnimationInterface::SetConstantPosition (wifiApNode.Get (0), 10, 30);

  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue ("ConfigureLocal"));
  tapBridge.SetAttribute ("DeviceName", StringValue ("thetap"));
  tapBridge.Install (wifiStaNodes.Get (1), devices.Get (0));

  InternetStackHelper internet;
  internet.Install (wifiStaNodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (2));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (120.0));

  // Not used, because we are using TapBridge
  UdpEchoClientHelper echoClient (i.GetAddress (2), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.000001)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1500));
  ApplicationContainer clientApps = echoClient.Install (wifiStaNodes.Get (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

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

  wifiPhy.EnablePcapAll ("tap-ap", false);

  // Packet::EnablePrinting ();
  // Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
  //                  MakeCallback (&Monitor));
  // Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacRx", MakeCallback (&DevRxTrace));
  std::cout << "AP\t" << i.GetAddress (0) << std::endl;
  std::cout << "Sta1\t" << i.GetAddress (1) << std::endl;
  std::cout << "Sta2\t" << i.GetAddress (2) << std::endl;

  // Simulator::Stop (Seconds (40.0));
  // Simulator::Schedule (Seconds (0), &VoidCallback);
  Simulator::Run ();
  Simulator::Destroy ();
}
