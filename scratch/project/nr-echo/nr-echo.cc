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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NR-echo");

int
main (int argc, char *argv[])
{
  // enable logging or not
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // set simulation time and mobility
  double simTime = 10; // seconds
  // double udpAppStartTime = 0.4; //seconds

  //other simulation parameters default values
  uint16_t numerology = 0;

  uint16_t gNbNum = 1;
  uint16_t ueNumPergNb = 1;

  double centralFrequency = 7e9;
  double bandwidth = 100e6;
  double txPower = 14;
  double lambda = 1000;
  uint32_t udpPacketSize = 1000;
  bool udpFullBuffer = true;
  uint8_t fixedMcs = 28;
  bool useFixedMcs = true;
  bool singleUeTopology = true;
  // Where we will store the output files.
  std::string simTag = "default";
  std::string outputDir = "./";

  CommandLine cmd;
  cmd.AddValue ("gNbNum", "The number of gNbs in multiple-ue topology", gNbNum);
  cmd.AddValue ("ueNumPergNb", "The number of UE per gNb in multiple-ue topology", ueNumPergNb);
  cmd.AddValue ("numerology", "The numerology to be used.", numerology);
  cmd.AddValue ("txPower", "Tx power to be configured to gNB", txPower);
  cmd.AddValue ("simTag",
                "tag to be appended to output filenames to distinguish simulation campaigns",
                simTag);
  cmd.AddValue ("outputDir", "directory where to store simulation results", outputDir);
  cmd.AddValue ("frequency", "The system frequency", centralFrequency);
  cmd.AddValue ("bandwidth", "The system bandwidth", bandwidth);
  cmd.AddValue ("udpPacketSize", "UDP packet size in bytes", udpPacketSize);
  cmd.AddValue ("lambda", "Number of UDP packets per second", lambda);
  cmd.AddValue ("udpFullBuffer",
                "Whether to set the full buffer traffic; if this parameter is set then the "
                "udpInterval parameter"
                "will be neglected",
                udpFullBuffer);
  cmd.AddValue (
      "fixedMcs",
      "The fixed MCS that will be used in this example if useFixedMcs is configured to true (1).",
      fixedMcs);
  cmd.AddValue ("useFixedMcs", "Whether to use fixed mcs, normally used for testing purposes",
                useFixedMcs);
  cmd.AddValue ("singleUeTopology",
                "If true, the example uses a predefined topology with one UE and one gNB; "
                "if false, the example creates a grid of gNBs with a number of UEs attached",
                singleUeTopology);

  cmd.Parse (argc, argv);

  NS_ASSERT (ueNumPergNb > 0);

  // setup the nr simulation
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

  /*
   * Spectrum division. We create one operation band with one component carrier
   * (CC) which occupies the whole operation band bandwidth. The CC contains a
   * single Bandwidth Part (BWP). This BWP occupies the whole CC band.
   * Both operational bands will use the StreetCanyon channel modeling.
   */
  CcBwpCreator ccBwpCreator;
  const uint8_t numCcPerBand = 1; // in this example, both bands have a single CC
  BandwidthPartInfo::Scenario scenario = BandwidthPartInfo::RMa_LoS;
  if (ueNumPergNb > 1)
    {
      scenario = BandwidthPartInfo::InH_OfficeOpen;
    }

  // Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
  // a single BWP per CC
  CcBwpCreator::SimpleOperationBandConf bandConf (centralFrequency, bandwidth, numCcPerBand,
                                                  scenario);

  // By using the configuration created, it is time to make the operation bands
  OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);

  /*
   * Initialize channel and pathloss, plus other things inside band1. If needed,
   * the band configuration can be done manually, but we leave it for more
   * sophisticated examples. For the moment, this method will take care
   * of all the spectrum initialization needs.
   */
  nrHelper->InitializeOperationBand (&band);

  BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps ({band});

  /*
   * Continue setting the parameters which are common to all the nodes, like the
   * gNB transmit power or numerology.
   */
  nrHelper->SetGnbPhyAttribute ("TxPower", DoubleValue (txPower));
  nrHelper->SetGnbPhyAttribute ("Numerology", UintegerValue (numerology));

  // Scheduler
  nrHelper->SetSchedulerTypeId (TypeId::LookupByName ("ns3::NrMacSchedulerTdmaRR"));
  nrHelper->SetSchedulerAttribute ("FixedMcsDl", BooleanValue (useFixedMcs));
  nrHelper->SetSchedulerAttribute ("FixedMcsUl", BooleanValue (useFixedMcs));

  if (useFixedMcs == true)
    {
      nrHelper->SetSchedulerAttribute ("StartingMcsDl", UintegerValue (fixedMcs));
      nrHelper->SetSchedulerAttribute ("StartingMcsUl", UintegerValue (fixedMcs));
    }

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (999999999));

  // Antennas for all the UEs
  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("AntennaElement",
                                   PointerValue (CreateObject<IsotropicAntennaModel> ()));

  // Antennas for all the gNbs
  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("AntennaElement",
                                    PointerValue (CreateObject<ThreeGppAntennaModel> ()));

  // Beamforming method
  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper> ();
  idealBeamformingHelper->SetAttribute ("BeamformingMethod",
                                        TypeIdValue (DirectPathBeamforming::GetTypeId ()));
  nrHelper->SetBeamformingHelper (idealBeamformingHelper);

  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue (MilliSeconds (0)));
  //  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

  // Error Model: UE and GNB with same spectrum error model.
  nrHelper->SetUlErrorModel ("ns3::NrEesmIrT1");
  nrHelper->SetDlErrorModel ("ns3::NrEesmIrT1");

  // Both DL and UL AMC will have the same model behind.
  nrHelper->SetGnbDlAmcAttribute (
      "AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
  nrHelper->SetGnbUlAmcAttribute (
      "AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel

  // Create EPC helper
  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  nrHelper->SetEpcHelper (epcHelper);
  // Core latency
  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));

  // gNb routing between Bearer and bandwidh part
  uint32_t bwpIdForBearer = 0;
  nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (bwpIdForBearer));

  // Initialize nrHelper
  nrHelper->Initialize ();

  /*
   *  Create the gNB and UE nodes according to the network topology
   */
  NodeContainer gNbNodes;
  NodeContainer ueNodes;

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> bsPositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> utPositionAlloc = CreateObject<ListPositionAllocator> ();

  const double gNbHeight = 10;
  const double ueHeight = 1.5;

  gNbNodes.Create (1);
  ueNodes.Create (1);
  gNbNum = 1;
  ueNumPergNb = 1;

  mobility.Install (gNbNodes);
  mobility.Install (ueNodes);
  bsPositionAlloc->Add (Vector (0.0, 0.0, gNbHeight));
  utPositionAlloc->Add (Vector (0.0, 30.0, ueHeight));

  mobility.SetPositionAllocator (bsPositionAlloc);
  mobility.Install (gNbNodes);

  mobility.SetPositionAllocator (utPositionAlloc);
  mobility.Install (ueNodes);

  // Install nr net devices
  NetDeviceContainer gNbNetDev = nrHelper->InstallGnbDevice (gNbNodes, allBwps);

  NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes, allBwps);

  int64_t randomStream = 1;
  randomStream += nrHelper->AssignStreams (gNbNetDev, randomStream);
  randomStream += nrHelper->AssignStreams (ueNetDev, randomStream);

  // When all the configuration is done, explicitly call UpdateConfig ()

  for (auto it = gNbNetDev.Begin (); it != gNbNetDev.End (); ++it)
    {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

  for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
    {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }

  // create the internet and install the IP stack on the UEs
  // get SGW/PGW and create a single RemoteHost
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // connect a remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  internet.Install (ueNodes);

  Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

  // Set the default gateway for the UEs
  for (uint32_t j = 0; j < ueNodes.GetN (); ++j)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (j)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (ueNetDev, gNbNetDev);

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

  std::cout << "UE\t\t" << ueIpIface.GetAddress (0, 0) << std::endl;
  Ptr<Ipv4> ipv4 = remoteHost->GetObject<Ipv4> ();
  std::cout << "remote host\t" << internetIpIfaces.GetAddress (1) << std::endl;

  std::cout << "pgw # of netdevices " << pgw->GetNDevices () << std::endl;
  std::cout << " netdev 0\t" << pgw->GetObject<Ipv4> ()->GetAddress (0, 0).GetLocal () << std::endl;
  std::cout << " netdev 1\t" << pgw->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () << std::endl;
  std::cout << " netdev 2\t" << pgw->GetObject<Ipv4> ()->GetAddress (2, 0).GetLocal () << std::endl;
  std::cout << " netdev 3\t" << pgw->GetObject<Ipv4> ()->GetAddress (3, 0).GetLocal () << std::endl;
  std::cout << "default gateway\t" << epcHelper->GetUeDefaultGatewayAddress () << std::endl;

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
