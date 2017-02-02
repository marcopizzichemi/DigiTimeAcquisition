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


  std::vector<std::string> ConnectionType_f;
  std::vector<std::string> LinkNum_f;
  std::vector<std::string> ConetNode_f;
  std::vector<std::string> BaseAddress_f;

  //read the config file
  std::string ConnectionType_s = config.read<std::string>("ConnectionType","1,1");
  std::string LinkNum_s = config.read<std::string>("LinkNum","0,0");
  std::string ConetNode_s = config.read<std::string>("ConetNode","0,1");
  std::string BaseAddress_s = config.read<std::string>("BaseAddress","0x32100000,0x33100000");

  config.split( ConnectionType_f, ConnectionType_s, "," );
  config.split( LinkNum_f, LinkNum_s, "," );
  config.split( ConetNode_f, ConetNode_s, "," );
  config.split( BaseAddress_f, BaseAddress_s, "," );

  assert(
          (ConnectionType_f.size() == LinkNum_f.size() ) &&
          (ConnectionType_f.size() == ConetNode_f.size() ) &&
          (ConnectionType_f.size() == BaseAddress_f.size() )
        );

  for(unsigned int i = 0 ; i < ConnectionType_f.size() ; i++)
  {
    config.trim(ConnectionType_f[i]);
    config.trim(LinkNum_f[i]);
    config.trim(ConetNode_f[i]);
    config.trim(BaseAddress_f[i]);
    Params->ConnectionType.push_back((CAEN_DGTZ_ConnectionType) atoi(ConnectionType_f[i].c_str()));
    Params->LinkNum.push_back(atoi(LinkNum_f[i].c_str()));
    Params->ConetNode.push_back(atoi(ConetNode_f[i].c_str()));
    Params->BaseAddress.push_back((uint32_t)strtol(BaseAddress_f[i].c_str(), NULL, 0));
  }

  for(unsigned int i = 0 ; i < Params->ConnectionType.size() ; i++)
  {
    std::cout << Params->ConnectionType[i] << "\t" << Params->LinkNum[i] << "\t" << Params->ConetNode[i] << "\t0x" << std::hex << Params->BaseAddress[i] << std::endl;
    // Params->ConnectionType[i] = (CAEN_DGTZ_ConnectionType) ConnectionType[i];
    // Params->LinkNum[i] = LinkNum[i];
    // Params->ConetNode[i] = ConetNode[i];
    // Params->BaseAddress[i] = BaseAddress[i];
  }



  // Params->ConnectionType[0]	= CAEN_DGTZ_OpticalLink; //CAEN_DGTZ_USB;
  // Params->ConnectionType[1]	= CAEN_DGTZ_OpticalLink; //CAEN_DGTZ_USB;

	// Params->LinkNum[0]		= 0;
	// Params->ConetNode[0]		= 0;
	// Params->BaseAddress[0]		= 0x32100000; //Master board


	// Params->LinkNum[1]		= 0;
	// Params->ConetNode[1]		= 1;
	// Params->BaseAddress[1]		= 0x33100000; //Slave board

	// TR and CHANNEL SETTINGS
    // NIM signal on TR:	DC_OFFSET 32768 	TRIGGER_THRESHOLD 20934
    // NIM/2 signal on TR:	DC_OFFSET 32768 	TRIGGER_THRESHOLD 23574
	// AC signal on TR:		DC_OFFSET 32768		TRIGGER_THRESHOLD 26214
	// +2V signal on TR: 	DC_OFFSET 43520		TRIGGER_THRESHOLD 26214
	// Params->RefChannel[0]			=16;		// Channel(or group?) of the Master used for the acquisition
	Params->FastTriggerThreshold[0]	= 20934;	// TR Trigger threshold (for fast triggering)
	Params->FastTriggerOffset[0]	= 32768;   // TR Offset adjust (DAC value)
	Params->DCoffset[0]				= 0x7FFF;   // input DC offset adjust (DAC value)
	Params->PostTrigger[0]			= 0;		// Post trigger in percent of the acquisition window
	// Edge Setting
	Params->ChannelThreshold[0]		= 1500;		// Threshold for Time Pulse Calculation
	Params->ChannelPulseEdge[0]		= CAEN_DGTZ_TriggerOnFallingEdge;
	Params->TRThreshold[0]			= 1500;		// Threshold for Time Pulse Calculation

	// Params->RefChannel[1]			= 16;		// Group of the Slave used for the acquisition
	Params->FastTriggerThreshold[1]	= 20934;		// TR Trigger threshold (for fast triggering)
	Params->FastTriggerOffset[1]	= 32768;   // TR Offset adjust (DAC value)
	Params->DCoffset[1]				= 0x7FFF;   // input DC offset adjust (DAC value)
	Params->PostTrigger[1]			= 0;		// Post trigger in percent of the acquisition window
	// Edge Setting
	Params->ChannelThreshold[1]		= 1500;		// Threshold for Time Pulse Calculation
	Params->ChannelPulseEdge[1]		= CAEN_DGTZ_TriggerOnFallingEdge;
	Params->TRThreshold[1]			= 1500;		// Threshold for Time Pulse Calculation

	// Trigger edge (CAEN_DGTZ_TriggerOnRisingEdge, CAEN_DGTZ_TriggerOnFallingEdge)
	Params->TriggerEdge			= CAEN_DGTZ_TriggerOnFallingEdge;

	// Number of samples in the acquisition windows (Valid Options: 1024, 520, 256 and 136)
	Params->RecordLength		= 1024;

	// Max. distance between the trigger time tags in order to consider a valid coincidence
	Params->MatchingWindow		= 200;

	// Front Panel LEMO I/O level (NIM or TTL). Options: CAEN_DGTZ_IOLevel_NIM, CAEN_DGTZ_IOLevel_TTL
	Params->IOlevel				= CAEN_DGTZ_IOLevel_NIM;

	// Internal Test Pattern (triangular wave replacing ADC data): 0=disabled, 1=enabled
	Params->TestPattern			= 0;

	// DRS4SamplingFrequency. Options: CAEN_DGTZ_DRS4_5GHz, CAEN_DGTZ_DRS4_2_5GHz,CAEN_DGTZ_DRS4_1GHz
	Params->DRS4Frequency		= CAEN_DGTZ_DRS4_5GHz;

	// ***************************************************************************************************
	// Start Mode. Options: START_SW_CONTROLLED, START_HW_CONTROLLED
	// ***************************************************************************************************
	Params->StartMode = START_SW_CONTROLLED;

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
	Params->EnableLog			= 1;

	// ***************************************************************************************************
	// Parameters for the time distribution histogram
	// ***************************************************************************************************
	Params->HistoNbins			= 2000;		// Number of bins of the histogram
	Params->HistoBinSize		= 0.005;		// Bin size in ns
	Params->HistoOffset			= -4.0;	// Lower value of the histogram in ns

}
