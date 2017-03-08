#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include <vector>
#include "time.h"

#define numClients 9 
#define MAX_GRID 40


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ht-wifi-network");

int main (int argc, char *argv[])
{

  srand(time(NULL)); // seed random numbers
  bool udp = false;
  double simulationTime = 10; //seconds
  double distance = 20.0; //meters
  double frequency = 5.0; //whether 2.4 or 5.0 GHz
  // stuff to replace for loops
  int i = 0; //MSC
  int j = 40; // channel width MHz
  int k = 0; // Short Guard Interval
 


  // replacing parameters above with command line args 
  CommandLine cmd;
  cmd.AddValue ("frequency", "Whether working in the 2.4 or 5.0 GHz band (other values gets rejected)", frequency);
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("udp", "UDP if set to 1, TCP otherwise", udp);
  cmd.Parse (argc,argv);

  std::cout << "MCS value" << "\t\t" << "Channel width" << "\t\t" << "short GI" << "\t\t" << "Throughput" << '\n';
  uint32_t payloadSize; //1500 byte IP packet
  payloadSize = 1448; //bytes
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));



  // declare client nodes
  NodeContainer wifiStaNode;
  wifiStaNode.Create (numClients);
  
  // declare access point
  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  // Yans simulator declarations
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  // Set guard interval
  phy.Set ("ShortGuardEnabled", BooleanValue (k));

  
  WifiMacHelper mac;
  WifiHelper wifi;
  if (frequency == 5.0)
    {
       wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
    }
  else if (frequency == 2.4)
    {
      wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
      Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
    }
  else
    {
       std::cout<<"Wrong frequency value!"<<std::endl;
       return 0;
    }

  std::ostringstream oss;
  oss << "HtMcs" << i;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
	    "ControlMode", StringValue (oss.str ()));

  Ssid ssid = Ssid ("ns3-80211ac");

  mac.SetType ("ns3::StaWifiMac",
              "Ssid", SsidValue (ssid),
              "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (phy, mac, wifiStaNode);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

  // Set channel width
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (j));

  // mobility.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();


  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
             


  std::vector<double> PositionArr(2*numClients);
  for(int p=0; p<2*numClients; p++){PositionArr[p] = (double)rand()*MAX_GRID/(double)RAND_MAX-(MAX_GRID/2.0);}
  for(int z=0; z<2*numClients; z+=2)
    {
      positionAlloc->Add (Vector (PositionArr[z], PositionArr[z+1],0.0));
    } 

              

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNode);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNode);

  Ipv4AddressHelper address;

  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer staNodeInterface;
  Ipv4InterfaceContainer apNodeInterface;

  staNodeInterface = address.Assign (staDevice);
  apNodeInterface = address.Assign (apDevice);

  /* Setting applications */
  ApplicationContainer serverApp;
  std::vector<ApplicationContainer> sinkAppArr(numClients);

  //TCP flow
  uint16_t port = 50000;
  Address apLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", apLocalAddress);
  
  for(int i=0; i<numClients; i++)
    { 
      sinkAppArr[i] = packetSinkHelper.Install (wifiStaNode.Get (i));
      sinkAppArr[i].Start (Seconds (0.0));
      sinkAppArr[i].Stop (Seconds (simulationTime + 1));
    } 


  serverApp = packetSinkHelper.Install (wifiApNode.Get (0));
                  
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simulationTime + 1));



              

  std::vector<OnOffHelper> OnOffArr(numClients, OnOffHelper ("ns3::TcpSocketFactory",Ipv4Address::GetAny ()));
	    

  for(int i=0; i<numClients; i++){ 
    OnOffArr[i].SetAttribute ("OnTime",  StringValue ("ns3::ExponentialRandomVariable[Mean=1/10000]"));
    OnOffArr[i].SetAttribute ("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=1/10000]"));
    OnOffArr[i].SetAttribute ("PacketSize", UintegerValue (payloadSize));
    OnOffArr[i].SetAttribute ("DataRate", DataRateValue (1000000000)); //bit/s
    OnOffArr[i].SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory")); //bit/s
    }
 

  ApplicationContainer apps;
  std::vector<AddressValue> remoteAddressArr(numClients);

  for(int i=0; i<numClients; i++){ 
    remoteAddressArr[i] = AddressValue(InetSocketAddress (staNodeInterface.GetAddress (i), port)); 
    OnOffArr[i].SetAttribute ("Remote", remoteAddressArr[i]);
    apps.Add (OnOffArr[i].Install (wifiApNode.Get (0)));
    }
  

  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (simulationTime + 1));



  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (simulationTime + 1));



//// net anim/////

AnimationInterface anim("anim1.xml");

anim.SetConstantPosition(wifiApNode.Get(0),0.0,0.0);

  for(int z=0; z<2*numClients; z+=2)
    {
      anim.SetConstantPosition(wifiStaNode.Get(z/2),PositionArr[z],PositionArr[z+1]);
    }

/////////////////




  Simulator::Run ();
  Simulator::Destroy ();

  double throughput = 0;
  //TCP
  uint32_t totalPacketsThrough = 0;
  for(int i=0; i<numClients; i++){
    totalPacketsThrough += DynamicCast<PacketSink> (sinkAppArr[i].Get (0))->GetTotalRx ();
  }
  throughput = (totalPacketsThrough) * 8 / (simulationTime * 1000000.0); //Mbit/s
  std::cout << i << "\t\t\t" << j << " MHz\t\t\t" << k << "\t\t\t" << throughput << " Mbit/s" << std::endl;
  return 0;
}
