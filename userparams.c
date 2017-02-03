#include "acquisition.h"
#include "ConfigFile.h"
#include <string>
#include <sstream>
#include <vector>
#include <assert.h>


//we keep Params so we don't modify all the code, but the function now is dummy. It just copies the entries from a config file to the structure
void SetUserParams(UserParams_t *Params, ConfigFile& config)
{
	// CONNECTION PARAMETERS:
	// ConnectionType: can be CAEN_DGTZ_USB, CAEN_DGTZ_OpticalLink (A2818 or A3818)
	// LinkNum: USB or PCI/PCIe enumeration (typ=0)
	// ConetNode: position in the optical daisy chain
	// BaseAddress: only for VME access (otherwise 0)

  // string declarations
  std::vector<std::string> ConnectionType_f;
  std::vector<std::string> LinkNum_f;
  std::vector<std::string> ConetNode_f;
  std::vector<std::string> BaseAddress_f;
  std::vector<std::string> FastTriggerThreshold_f;
  std::vector<std::string> FastTriggerOffset_f;
  std::vector<std::string> DCoffset_f;
  std::vector<std::string> ChannelThreshold_f;
  std::vector<std::string> TRThreshold_f;
  std::vector<std::string> ChannelPulseEdge_f;
  std::vector<std::string> PostTrigger_f;
  // read the config file strings
  std::string ConnectionType_s       = config.read<std::string>("ConnectionType","1,1");
  std::string LinkNum_s              = config.read<std::string>("LinkNum","0,0");
  std::string ConetNode_s            = config.read<std::string>("ConetNode","0,1");
  std::string BaseAddress_s          = config.read<std::string>("BaseAddress","0x32100000,0x33100000");
  std::string FastTriggerThreshold_s = config.read<std::string>("FastTriggerThreshold","20934,20934");
  std::string FastTriggerOffset_s    = config.read<std::string>("FastTriggerOffset","32768,32768");
  std::string DCoffset_s             = config.read<std::string>("DCoffset","0x7FFF,0x7FFF");
  std::string ChannelThreshold_s     = config.read<std::string>("ChannelThreshold","1500,1500");
  std::string TRThreshold_s          = config.read<std::string>("TRThreshold","1500,1500");
  std::string ChannelPulseEdge_s     = config.read<std::string>("ChannelPulseEdge","1,1");
  std::string PostTrigger_s          = config.read<std::string>("PostTrigger","0,0");
  // string splitting
  config.split( ConnectionType_f, ConnectionType_s, "," );
  config.split( LinkNum_f, LinkNum_s, "," );
  config.split( ConetNode_f, ConetNode_s, "," );
  config.split( BaseAddress_f, BaseAddress_s, "," );
  config.split( FastTriggerThreshold_f,FastTriggerThreshold_s, "," );
  config.split( FastTriggerOffset_f,FastTriggerOffset_s, "," );
  config.split( DCoffset_f,DCoffset_s, "," );
  config.split( ChannelThreshold_f,ChannelThreshold_s, "," );
  config.split( TRThreshold_f,TRThreshold_s, "," );
  config.split( ChannelPulseEdge_f,ChannelPulseEdge_s, "," );
  config.split( PostTrigger_f,PostTrigger_s, "," );
  // check vectors length
  assert((ConnectionType_f.size() == LinkNum_f.size()     ) &&
         (ConnectionType_f.size() == ConetNode_f.size()   ) &&
         (ConnectionType_f.size() == BaseAddress_f.size() ) &&
         (ConnectionType_f.size() == FastTriggerThreshold_f.size() ) &&
         (ConnectionType_f.size() == FastTriggerOffset_f.size() ) &&
         (ConnectionType_f.size() == DCoffset_f.size() ) &&
         (ConnectionType_f.size() == ChannelThreshold_f.size() ) &&
         (ConnectionType_f.size() == TRThreshold_f.size() ) &&
         (ConnectionType_f.size() == ChannelPulseEdge_f.size() ) &&
         (ConnectionType_f.size() == PostTrigger_f.size() )
         );
  Params->NumOfDigitizers = ConnectionType_f.size();
  // fill the Params struct
  for(unsigned int i = 0 ; i < ConnectionType_f.size() ; i++)
  {
    //trim strings
    config.trim(ConnectionType_f[i]);
    config.trim(LinkNum_f[i]);
    config.trim(ConetNode_f[i]);
    config.trim(BaseAddress_f[i]);
    config.trim(FastTriggerThreshold_f[i]);
    config.trim(FastTriggerOffset_f[i]);
    config.trim(DCoffset_f[i]);
    config.trim(ChannelThreshold_f[i]);
    config.trim(TRThreshold_f[i]);
    config.trim(ChannelPulseEdge_f[i]);
    config.trim(PostTrigger_f[i]);
    //fill Params
    Params->ConnectionType.push_back((CAEN_DGTZ_ConnectionType) atoi(ConnectionType_f[i].c_str()));
    Params->LinkNum.push_back(atoi(LinkNum_f[i].c_str()));
    Params->ConetNode.push_back(atoi(ConetNode_f[i].c_str()));
    Params->BaseAddress.push_back((uint32_t)strtol(BaseAddress_f[i].c_str(), NULL, 0));
    Params->FastTriggerThreshold.push_back((uint16_t)strtol(FastTriggerThreshold_f[i].c_str(), NULL, 0));
    Params->FastTriggerOffset.push_back((uint16_t)strtol(FastTriggerOffset_f[i].c_str(), NULL, 0));
    Params->DCoffset.push_back((uint16_t)strtol(DCoffset_f[i].c_str(), NULL, 0));
    Params->ChannelThreshold.push_back((uint16_t)strtol(ChannelThreshold_f[i].c_str(), NULL, 0));
    Params->TRThreshold.push_back((uint16_t)strtol(TRThreshold_f[i].c_str(), NULL, 0));
    Params->ChannelPulseEdge.push_back((CAEN_DGTZ_TriggerPolarity_t) atoi(ChannelPulseEdge_f[i].c_str()));
    Params->PostTrigger.push_back((uint16_t)strtol(PostTrigger_f[i].c_str(), NULL, 0));
  }



  //SINGLE VALUES
  //read config file
  int TriggerEdge     = config.read<int>("TriggerEdge",1);       // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
  int RecordLength    = config.read<int>("RecordLength",1024);   // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
  int MatchingWindow  = config.read<int>("MatchingWindow",200);  // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
  int IOlevel         = config.read<int>("IOlevel",0);           // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
  int TestPattern     = config.read<int>("TestPattern",0);       // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
  int DRS4Frequency   = config.read<int>("DRS4Frequency",0);     // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
  int StartMode       = config.read<int>("StartMode",0);         // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
  int EnableLog       = config.read<int>("EnableLog",1);         // Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)

	// TR and CHANNEL SETTINGS
    // NIM signal on TR:	DC_OFFSET 32768 	TRIGGER_THRESHOLD 20934
    // NIM/2 signal on TR:	DC_OFFSET 32768 	TRIGGER_THRESHOLD 23574
	// AC signal on TR:		DC_OFFSET 32768		TRIGGER_THRESHOLD 26214
  // +2V signal on TR: 	DC_OFFSET 43520		TRIGGER_THRESHOLD 26214

	// Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)
	Params->TriggerEdge			= (CAEN_DGTZ_TriggerPolarity_t) TriggerEdge;

	// Number of samples in the acquisition windows (Valid Options: 1024, 520, 256 and 136)
	Params->RecordLength		= RecordLength;

	// Max. distance between the trigger time tags in order to consider a valid coincidence
	Params->MatchingWindow		= MatchingWindow;

	// Front Panel LEMO I/O level (NIM or TTL). Options: CAEN_DGTZ_IOLevel_NIM, CAEN_DGTZ_IOLevel_TTL
	Params->IOlevel				= (CAEN_DGTZ_IOLevel_t) IOlevel;

	// Internal Test Pattern (triangular wave replacing ADC data): 0=disabled, 1=enabled
	Params->TestPattern			= TestPattern;

	// DRS4SamplingFrequency. Options: CAEN_DGTZ_DRS4_5GHz, CAEN_DGTZ_DRS4_2_5GHz,CAEN_DGTZ_DRS4_1GHz
	Params->DRS4Frequency		= (CAEN_DGTZ_DRS4Frequency_t) DRS4Frequency;

	// ***************************************************************************************************
	// Start Mode. Options: START_SW_CONTROLLED, START_HW_CONTROLLED
	// ***************************************************************************************************
	Params->StartMode =  StartMode;

	// ***************************************************************************************************
	// Sync Mode: INDIVIDUAL_TRIGGER_SIN_TRGOUT ()
	/*
	N boards with TR triggers
	SETUP:
	daisy chain SIN-TRGOUT to propagate the start of run

	START OF RUN:
	1.	All boards: set start mode = start on SIN high level;
	2.	All boards: set Tr parameters and enable triggering and digitizing
	3.	All boards: set propagation of SIN to TRG-OUT
	4.	All boards armed to start
	5.	Send SW Start to 1st board
	6.	The RUN signal is propagate through the daisy chain SIN-TRGOUT and start all the boards.
		Delay can be compensated by means of RUN_DELAY register
	7.	Once started, each board is triggered by the TR signal

	NOTE1: Start and Stop can also be controlled by an external signal going into SIN of the 1st board
	NOTE2: the stop is also synchronous:  when the 1st board is stopped (by SW command), the stop is propagated to the other boards
		   through the daisy chain.
	*/

	// ***************************************************************************************************
	// Log Enabling
	// ***************************************************************************************************
	Params->EnableLog			= EnableLog;

	// ***************************************************************************************************
	// Parameters for the time distribution histogram
	// ***************************************************************************************************
	// Params->HistoNbins			= 2000;		// Number of bins of the histogram
	// Params->HistoBinSize		= 0.005;		// Bin size in ns
	// Params->HistoOffset			= -4.0;	// Lower value of the histogram in ns



  // Parameters check
  std::cout << std::endl;
  std::cout << "//////////////////////////////////" << std::endl;
  std::cout << "//      Config Parameters       //" << std::endl;
  std::cout << "//////////////////////////////////" << std::endl;
  std::cout << std::endl;
  for(unsigned int i = 0 ; i < Params->ConnectionType.size() ; i++)
  {
    std::cout << "********** Digitizer "            << i << "************" << std::endl;
    std::cout << "ConnectionType\t\t= "             << Params->ConnectionType[i]                  << "\t\t" << "# 0 = USB, 1 = OpticalLink"  << std::endl
              << "LinkNum\t\t\t= "                  << Params->LinkNum[i]                         << "\t\t" << "# Link number for this digitizer" << std::endl
              << "ConetNode\t\t= "                  << Params->ConetNode[i]                       << "\t\t" << "# Conet Node" << std::endl
              << "BaseAddress\t\t= "                << "0x" << std::hex << Params->BaseAddress[i] << std:: dec << "\t" << "# Base address for this digitizer" << std::endl
              << "FastTriggerThreshold\t= "         << Params->FastTriggerThreshold[i]            << "\t\t" << "# TR Trigger threshold (for fast triggering)" << std::endl
              << "FastTriggerOffset\t= "            << Params->FastTriggerOffset[i]               << "\t\t" << "# TR Offset adjust (DAC value)" << std::endl
              << "DCoffset\t\t= "                   << "0x"<< std::hex << Params->DCoffset[i]     << std:: dec << "\t" << "# input DC offset adjust (DAC value)" << std::endl
              << "ChannelThreshold\t= "             << Params->ChannelThreshold[i]                << "\t\t" << "# Threshold for Time Pulse Calculation" << std::endl
              << "TRThreshold\t\t= "                << Params->TRThreshold[i]                     << "\t\t" << "# Threshold for Time Pulse Calculation" << std::endl
              << "ChannelPulseEdge\t= "             << Params->ChannelPulseEdge[i]                << "\t\t" << "# 0 = Rising Edge, 1 = Falling Edge"  << std::endl
              << "PostTrigger\t\t= "                << Params->PostTrigger[i]                     << "\t\t" << "# Post trigger in percent of the acquisition window" << std::endl;


  }
  std::cout << "*****  Global parameters *****" << std::endl;
  std::cout << "TriggerEdge\t\t= "			            << Params->TriggerEdge		<< "\t\t" << "# Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge = 0, CAEN_DGTZ_TriggerOnFallingEdge = 1)" << std::endl
            << "RecordLength\t\t= "		              << Params->RecordLength		<< "\t\t" << "# Number of samples in the acquisition windows (Valid Options: 1024, 520, 256 and 136)" << std::endl
            << "MatchingWindow\t\t= "			          << Params->MatchingWindow	<< "\t\t" << "# Max. distance between the trigger time tags in order to consider a valid coincidence" << std::endl
            << "IOlevel\t\t\t= "		                << Params->IOlevel			  << "\t\t" << "# Front Panel LEMO I/O level (NIM or TTL). Options: CAEN_DGTZ_IOLevel_NIM = 0, CAEN_DGTZ_IOLevel_TTL = 1" << std::endl
            << "TestPattern\t\t= "		              << Params->TestPattern		<< "\t\t" << "# Internal Test Pattern (triangular wave replacing ADC data): 0 = disabled, 1 = enabled" << std::endl
            << "DRS4Frequency\t\t= "		            << Params->DRS4Frequency	<< "\t\t" << "# DRS4SamplingFrequency. Options: CAEN_DGTZ_DRS4_5GHz = 0, CAEN_DGTZ_DRS4_2_5GHz = 1 ,CAEN_DGTZ_DRS4_1GHz = 2" << std::endl
            << "StartMode\t\t= "		                << Params->StartMode      << "\t\t" << "# Start Mode. Options: START_SW_CONTROLLED = 0, START_HW_CONTROLLED = 1" << std::endl
            << "EnableLog\t\t= "		                << Params->EnableLog	    << "\t\t" << "# Log Enabling (1 = yes, 0 = no)" << std::endl ;
}

void SetDefaultParams(UserParams_t *Params)
{
  //set parameters to default when no config file is passed
  Params->NumOfDigitizers = 2;
  for(unsigned int i = 0 ; i < Params->NumOfDigitizers ; i++)
  {
    Params->ConnectionType.push_back(CAEN_DGTZ_OpticalLink);
    Params->LinkNum.push_back(0);
    Params->FastTriggerThreshold.push_back(20934);
    Params->FastTriggerOffset.push_back(32768);
    Params->DCoffset.push_back(0x7FFF);
    Params->ChannelThreshold.push_back(1500);
    Params->TRThreshold.push_back(1500);
    Params->ChannelPulseEdge.push_back(CAEN_DGTZ_TriggerOnFallingEdge);
    Params->PostTrigger.push_back(0);
  }

  Params->ConetNode.push_back(0);
  Params->ConetNode.push_back(1);
  Params->BaseAddress.push_back(0x32100000);
  Params->BaseAddress.push_back(0x33100000);

  Params->TriggerEdge			= CAEN_DGTZ_TriggerOnFallingEdge;
  Params->RecordLength		= 1024;
  Params->MatchingWindow		= 200;
  Params->IOlevel				= CAEN_DGTZ_IOLevel_NIM;
  Params->TestPattern			= 0;
  Params->DRS4Frequency		= CAEN_DGTZ_DRS4_5GHz;
  Params->StartMode = START_SW_CONTROLLED;
  Params->EnableLog			= 1;
}
