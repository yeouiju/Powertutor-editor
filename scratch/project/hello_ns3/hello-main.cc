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
#include "ns3/nstime.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Hello Simulator");

static int g_count = 0;

void
HelloPrinter ()
{
  NS_LOG_UNCOND ("Hello World from scheduled event " << g_count++);
  Simulator::Schedule (Seconds (2), &HelloPrinter);
}

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("Hello NS-3 Simulator");
  CommandLine cmd;
  cmd.Parse (argc, argv);
  Simulator::Schedule (Seconds (0), &HelloPrinter);
  Simulator::Stop (Seconds (25));
  Simulator::Run ();
  Simulator::Destroy ();
}
