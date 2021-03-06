// compile with
// g++ -DLINUX acquisition.c X742CorrectionRoutines.c userparams.c dpp_qdc.c _CAENDigitizer_DPP-QDC.c ConfigFile.cc -o acquisition -lCAENDigitizer -lCAENComm -fpermissive

/*
* Synctest application is a simple piece of software to demonstrates
* multiboard synchronization with CAEN digitizers.
* It only depends on CAENDigitizer library.
*
* So far, this software has been tested with V1724, V1720 and V1721/31
* the V1740 is not yet supported.
*
* Synctest can be adapted to different synchronization setup by changing
* the parameter SyncMode. See UserParams.c for more details
*/

// TODO
// 0 - testing of ALL with boards
// 1-  write registers from config to specific modules - DONE
// 2-  implement TTT control for multi boards - up to now valid for 2 and only 2 boards (i.e. won't even work for 1 board alone...)
// 3-  active channels, groups etc
// 4 - what is Tt?
// 5 - SetSyncMode, various FIXME

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <CAENDigitizer.h>
#include <assert.h>

#include "acquisition.h"
#include "X742CorrectionRoutines.h"

int	gHandle; /* CAEN library handle */
CAEN_DGTZ_BoardInfo_t             gBoardInfo;

long get_time()
{
  long time_ms;
  #ifdef WIN32
  struct _timeb timebuffer;
  _ftime( &timebuffer );
  time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
  #else
  struct timeval t1;
  struct timezone tz;
  gettimeofday(&t1, &tz);
  time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
  #endif
  return time_ms;
}

#include "dpp_qdc.h"
// #include "ConfigFile.h"

#ifdef WIN32
#define GNUPLOTEXE  "pgnuplot"
#else
#define GNUPLOTEXE  "gnuplot"
#endif




int getch(void)
{
  unsigned char temp;
  raw();
  /* stdin = fd 0 */
  if(read(0, &temp, 1) != 1)
  return 0;
  return temp;
}

int kbhit()
{
  struct timeval timeout;
  fd_set read_handles;
  int status;
  raw();
  /* check stdin (fd 0) for activity */
  FD_ZERO(&read_handles);
  FD_SET(0, &read_handles);
  timeout.tv_sec = timeout.tv_usec = 0;
  status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
  if(status < 0)	{
    printf("select() failed in kbhit()\n");
    exit(1);
  }
  return (status);
}
/* ============================================================================== */
/* Get time in milliseconds from the computer internal clock */






/* ============================================================================== */
/* TIMING DIGITIZERS CONFIGURATION                                                */
/* ============================================================================== */
int ConfigureDigitizers(std::vector<int> handle, std::vector<CAEN_DGTZ_BoardInfo_t> Binfo, UserParams_t Params, ConfigFile& config)
{
  int ret = 0;
  uint32_t d32;
  printf("cHandle[0] = %d\ncHandle[1] = %d\n",handle[0],handle[1]);
  //--------------------------------------//
  // READ CONFIG FILE AND GENERIC WRITES
  //--------------------------------------//
  //read ConfigFile generic register writes
  std::vector<uint32_t> values,addresses;
  std::vector<int> write_handle;
  std::string values_s,addresses_s,write_handle_s;
  std::vector<std::string> values_f,addresses_f,write_handle_f;
  write_handle_s = config.read<std::string>("board","");
  addresses_s = config.read<std::string>("address","");
  values_s = config.read<std::string>("value","");
  config.split( values_f, values_s, "," );
  config.split( addresses_f, addresses_s, "," );
  config.split( write_handle_f, write_handle_s, "," );
  assert( (values_f.size() == addresses_f.size()) && (values_f.size() == write_handle_f.size()) );
  for(unsigned int i = 0 ; i < values_f.size() ; i++)
  {
    //trim strings
    config.trim(values_f[i]);
    config.trim(addresses_f[i]);
    config.trim(write_handle_f[i]);
    values.push_back((uint32_t) strtol(values_f[i].c_str(), NULL, 0) );
    addresses.push_back((uint32_t) strtol(addresses_f[i].c_str(), NULL, 0) );
    write_handle.push_back((uint32_t) strtol(write_handle_f[i].c_str(), NULL, 0) );
  }
  
  
  // write on registers 
  for(unsigned int i=0; i<Params.NumOfDigitizers; i++)
  {
    /* Reset all board registers */
    printf("ret %d \n",ret);
    ret = CAEN_DGTZ_Reset(handle[i]); //i get ret=-18 i.e. communication timeout??? only for the first digitizer among the two timing ones...
    printf("ret %d \n",ret);
    if (Params.TestPattern)
    ret |= CAEN_DGTZ_WriteRegister(handle[i], CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);
    printf("ret %d \n",ret);
    if (Params.DRS4Frequency) {
      ret |= CAEN_DGTZ_SetDRS4SamplingFrequency(handle[i], Params.DRS4Frequency);
    }
    printf("ret %d \n",ret);
    ret |= CAEN_DGTZ_SetRecordLength(handle[i], Params.RecordLength);
    ret |= CAEN_DGTZ_SetPostTriggerSize(handle[i], Params.PostTrigger[i]);
    ret |= CAEN_DGTZ_SetIOLevel(handle[i], Params.IOlevel);
    ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle[i], MAX_EVENTS_XFER);
    printf("ret %d \n",ret);
    ret |= CAEN_DGTZ_SetGroupEnableMask(handle[i], 0x000F);   // fixed to enable all groups
    int gr,ch;
    for(gr = 0 ; gr < 4 ; gr++)
    {
      ret |= CAEN_DGTZ_SetGroupFastTriggerThreshold(handle[i], gr, Params.FastTriggerThreshold[i]);
      ret |= CAEN_DGTZ_SetGroupFastTriggerDCOffset(handle[i], gr, Params.FastTriggerOffset[i]);
      ret |= CAEN_DGTZ_SetTriggerPolarity(handle[i],gr,Params.TriggerEdge);
    }
    ret |= CAEN_DGTZ_SetFastTriggerMode(handle[i],CAEN_DGTZ_TRGMODE_ACQ_ONLY);
    ret |= CAEN_DGTZ_SetFastTriggerDigitizing(handle[i],CAEN_DGTZ_ENABLE);
    for(ch = 0 ; ch < 32 ; ch++)
    {
      ret |= CAEN_DGTZ_SetChannelDCOffset(handle[i], ch, Params.DCoffset[i]);
    }
    printf("ret %d \n",ret);
    /* DO NOT CHANGE : Patch for 1024 or 1023 memory segments */
    /*ret |= CAEN_DGTZ_ReadRegister(handle[i], 0x8174, &rdata);
    *        if ((rdata == 0x400) || (rdata == 0x3FF))
    *                ret |= CAEN_DGTZ_WriteRegister(handle[i], 0x8174, 0x3FE);*/

    ///     Register Settings   ///
    for(unsigned int j = 0 ; j < values.size(); j++)
      ret |= CAEN_DGTZ_WriteRegister(write_handle[j],addresses[j],values[j]);
  }
  //--------------------------------------------//
  // end of READ CONFIG FILE AND GENERIC WRITES
  //--------------------------------------------//
  
  printf("ret %d \n",ret);
  
  //BEGIN of DEBUG OUTPUT  
  for(unsigned int i=0; i<Params.NumOfDigitizers; i++) {
    int gr,ch;
    uint32_t TValue;
    CAEN_DGTZ_TriggerPolarity_t TriggerPolarity;
    CAEN_DGTZ_TriggerMode_t mode;
    CAEN_DGTZ_EnaDis_t enable;
    printf("\n----------------------------------------------------------------\n");
    printf("                  TIME DIGITIZER %d          \n",i);
    printf("----------------------------------------------------------------\n\n");    
    ret |= CAEN_DGTZ_GetFastTriggerMode(handle[i],&mode);
    printf("CAEN_DGTZ_GetFastTriggerMode             = %d\n",mode);
    ret |= CAEN_DGTZ_GetFastTriggerDigitizing(handle[i],&enable);
    printf("CAEN_DGTZ_GetFastTriggerDigitizing       = %d\n",enable);
    for(gr = 0 ; gr < 4 ; gr++)
    {
      printf("Group %d \n",gr);
      ret |= CAEN_DGTZ_GetGroupFastTriggerThreshold(handle[i], gr, &TValue);
      printf("CAEN_DGTZ_GetGroupFastTriggerThreshold = %d\n",TValue);
      ret |= CAEN_DGTZ_GetGroupFastTriggerDCOffset(handle[i], gr, &TValue);
      printf("CAEN_DGTZ_GetGroupFastTriggerDCOffset  = %d\n",TValue);
      ret |= CAEN_DGTZ_GetTriggerPolarity(handle[i],gr,&TriggerPolarity);
      printf("CAEN_DGTZ_GetTriggerPolarity           = %d\n",TriggerPolarity);
    }
    for(ch = 0 ; ch < 32 ; ch++)
    {
      ret |= CAEN_DGTZ_GetChannelDCOffset(handle[i], ch,&TValue);
      printf("CAEN_DGTZ_GetChannelDCOffset, ch %d    = %d\n",ch,TValue);
    }
  }
  //END of DEBUG OUTPUT
  
  
  // CHECK REGISTER WRITES
  // check registers
  for(unsigned int j = 0; j < values.size() ; j++)
  {
    ret |= CAEN_DGTZ_ReadRegister(write_handle[j],addresses[j],&d32);
    printf("%x register in digitizer %d = %x\n",addresses[j],write_handle[j],d32);
  }

  return ret;
}



/* ============================================================================== */
int SetSyncMode(std::vector<int> handle, UserParams_t Params)
{
//   unsigned int i, ret=0;
//   uint32_t reg,d32;
// 
//   for(i=0; i<Params.NumOfDigitizers; i++) {
//     if (i > 0)  // Run starts with S-IN on the 2nd board (or more than 2?) //FIXME
//     ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_ACQUISITION_MODE, RUN_START_ON_SIN_LEVEL);
//     ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_GLOBAL_TRG_MASK, (1<<30/*|1<<Params.RefChannel[i])*/));  // accept only trg from selected channel --
//     // 	ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_GLOBAL_TRG_MASK, (1<<30|1<<5));  // accept only trg from selected channel
//     ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_TRG_OUT_MASK, 0);   // no tigger propagation to TRGOUT
//     ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_RUN_DELAY, 0);   // Run Delay decreases with the position (to compensate for run the propagation delay) //FIXME - what is this?
//     //      ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_RUN_DELAY, 2*(1-i));   // Run Delay decreases with the position (to compensate for run the propagation delay)
// 
//     // Set TRGOUT=RUN to propagate run through S-IN => TRGOUT daisy chain
//     ret |= CAEN_DGTZ_ReadRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, &reg);
// 
//     reg = reg & 0xFFF0FFFF | 0x00010000;
//     ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, reg);
// 
//   }
//   for(i=0; i<Params.NumOfDigitizers; i++) {
//     ret  |= CAEN_DGTZ_ReadRegister(handle[0],0x810C,&d32);
//     printf("SetSyncMode 0x810C[%d] = %x\n",i,d32);
//   }
//   return ret;
  
  //modified version, simplified for 3 digi (first charge, then 2 time)
  
  unsigned int i, ret=0;
  uint32_t reg,d32;
// 
  for(i=0; i<3; i++) 
  {
    if (i > 0)  // Run starts with S-IN on the 2nd board (or more than 2?) //FIXME
      ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_ACQUISITION_MODE, RUN_START_ON_SIN_LEVEL);
    
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_GLOBAL_TRG_MASK, (1<<30/*|1<<Params.RefChannel[i])*/));  // accept only trg from selected channel --
    // 	ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_GLOBAL_TRG_MASK, (1<<30|1<<5));  // accept only trg from selected channel
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_TRG_OUT_MASK, 0);   // no tigger propagation to TRGOUT
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_RUN_DELAY, 0);   // Run Delay decreases with the position (to compensate for run the propagation delay) //FIXME - what is this?
    //      ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_RUN_DELAY, 2*(1-i));   // Run Delay decreases with the position (to compensate for run the propagation delay)
// 
    // Set TRGOUT=RUN to propagate run through S-IN => TRGOUT daisy chain
    ret |= CAEN_DGTZ_ReadRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, &reg);
// 
    reg = reg & 0xFFF0FFFF | 0x00010000;
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, reg);
// 
  }
  for(i=0; i<3; i++) {
    ret  |= CAEN_DGTZ_ReadRegister(handle[i],0x810C,&d32);
    printf("SetSyncMode 0x810C[%d] = %x\n",i,d32);
  }
  return ret;
}


/* ============================================================================== */
int StartRun(std::vector<int> handle, int StartMode)
{
  if (StartMode == START_SW_CONTROLLED) {
    CAEN_DGTZ_WriteRegister(handle[0], ADDR_ACQUISITION_MODE, 0x4);
  } else {
    CAEN_DGTZ_WriteRegister(handle[0], ADDR_ACQUISITION_MODE, 0x5);
    printf("Run starts/stops on the S-IN high/low level\n");
  }

  return 0;
}



/* ============================================================================== */
int StopRun(std::vector<int> handle)
{
  CAEN_DGTZ_WriteRegister(handle[0], ADDR_ACQUISITION_MODE, 0x0);
  return 0;
}



/* ============================================================================== */
int ForceClockSync(int handle)
{
  int ret;
  SLEEP(500);
  /* Force clock phase alignment */
  ret = CAEN_DGTZ_WriteRegister(handle, ADDR_FORCE_SYNC, 1);
  /* Wait an appropriate time before proceeding */
  SLEEP(1000);
  return ret;
}



/* ########################################################################### */
/* MAIN                                                    SetSyncMode         */
/* ########################################################################### */
int main(int argc, char *argv[])
{

  /* *************************************************************************************** */
  /* SET USER PARAMS                                                                         */
  /* *************************************************************************************** */
  UserParams_t  Params;
  std::string ConfigFileName = "";
  if(argc > 1)
  {
    ConfigFileName = string(argv[1]);
  }
  else
  {
    std::cout << "ERROR!!!! you need to provide a config file (example config.cfg)" << std::endl;
    return 1;
  }
  ConfigFile config(ConfigFileName);

  //get the two config file names
  std::string digiTimeConfigFile = config.read<std::string>("digiTimeConfigFile");
  std::string digiChargeConfigFile = config.read<std::string>("digiChargeConfigFile");
  int NumbOfDigiTime = config.read<int>("NumbOfDigiTime");
  int NumbOfDigiCharge = config.read<int>("NumbOfDigiCharge");

  //open digi time config (charge digi uses a different type of 
  ConfigFile configDigiTime(digiTimeConfigFile.c_str());
  SetUserParams(&Params,configDigiTime); //set Params. Params is used ONLY for the two timing digitizers
  /* *************************************************************************************** */



  /* *************************************************************************************** */
  /* VARIABLES                                                                               */
  /* *************************************************************************************** */
  int cycle = 0;
  int ret=0, Nbit, Nch, error=1, running=0;
  int QuitAcquisition=0, plot=0, PlotHist=0, ContinousHistPlot=0,SoftwareTrigger =0,ContinousWaveplot=0;
  uint32_t Nb=0, MatchingEvents=0;
  uint64_t CurrentTime, PrevRateTime, ElapsedTime, NsT=0, NsTTT=0;
  char c;
  double Ts, Tt;
  double MeanT=0.0, MeanTTT=0.0, SigmaT=0.0, SigmaTTT=0.0;
  FILE *log        = NULL;
  FILE *event_file = NULL;
  FILE *plotter;
  FILE *binOut = NULL;
  FILE *textOut = NULL;
  std::vector<int> EvNoData;
  std::vector<int> GetNextEvent;
  std::vector<int> write;
  std::vector<int> ContinousWrite;
  std::vector<int> handle;
  std::vector<int> cHandle;
  std::vector<CAEN_DGTZ_BoardInfo_t> cBoardInfo;
  std::vector<char*> buffer;
  std::vector<char*> EventPtr;
  std::vector<CAEN_DGTZ_BoardInfo_t> BoardInfo;
  std::vector<CAEN_DGTZ_EventInfo_t> EventInfo;
  std::vector<CAEN_DGTZ_X742_EVENT_t*> Event742;
  std::vector<float*> Wave;
  std::vector<float*> Trigger;
  std::vector<FILE*> board;
  std::vector<uint32_t> Ns, EIndx, NumEvents, BufferSize, TrgCnt, missingEdge;
  std::vector<uint64_t> Nroll,Head_Nroll;
  std::vector<double> TTT, PrevTTT, DeltaT, DeltaTTT, PulseEdgeTime, TrEdgeTime;
  std::vector<DataCorrection_t> Table;

  for(unsigned int i = 0 ; i < Params.NumOfDigitizers; i++) //initialize variables, ony for timing digi
  {
    EvNoData.push_back(0);
    GetNextEvent.push_back(1);
    write.push_back(0);
    ContinousWrite.push_back(0);
/*    handle.push_back(0);*/
    buffer.push_back(NULL);
    EventPtr.push_back(NULL);
    Wave.push_back(NULL);
    Trigger.push_back(NULL);
    board.push_back(NULL);
    Ns.push_back(0);
    EIndx.push_back(0);
    NumEvents.push_back(0);
    BufferSize.push_back(0);
    TrgCnt.push_back(0);
    missingEdge.push_back(0);
    Nroll.push_back(0);
    Head_Nroll.push_back(0);
    TTT.push_back(0);
    PrevTTT.push_back(0);
    DeltaT.push_back(0);
    DeltaTTT.push_back(0);
    PulseEdgeTime.push_back(0);
    TrEdgeTime.push_back(0);
    Event742.push_back(NULL);
//     CAEN_DGTZ_BoardInfo_t BoardInfo_temp;
    CAEN_DGTZ_EventInfo_t EventInfo_temp;
    DataCorrection_t Table_temp;
//     BoardInfo.push_back(BoardInfo_temp);
    EventInfo.push_back(EventInfo_temp);
    Table.push_back(Table_temp);
  }



  printf("\n");
  printf("**************************************************************\n");
  printf(" Acquisition Software \n");
  printf("**************************************************************\n\n");



  /* *************************************************************************************** */
  /* *************************************************************************************** */
  /* OPEN DIGITIZERS                                                                         */
  /* *************************************************************************************** */
  /* *************************************************************************************** */
  
  
  
  /* *************************************************************************************** */
  /* CHARGE DIGITIZERS                                                                       */
  /* *************************************************************************************** */
  // open charge digitizer(s)
  ret = setup_acquisition(digiChargeConfigFile.c_str());
  if (ret) {
    printf("Error during acquisition setup (ret = %d) .... Exiting\n", ret);
  running = 0;
  }
  
  
  BoardInfo.push_back(gBoardInfo);
  CAEN_DGTZ_BoardInfo_t BoardInfo_temp;
  BoardInfo.push_back(BoardInfo_temp);
  BoardInfo.push_back(BoardInfo_temp);
  handle.push_back(gHandle);
  handle.push_back(0);
  handle.push_back(0);  
/*  printf("handle[0] = %d\n", handle[0]);*/
  

  /* *************************************************************************************** */
  /* TIME DIGITIZERS                                                                         */
  /* *************************************************************************************** */
  for (unsigned int i = 0; i < Params.NumOfDigitizers; i++) {  //open time digitizers
    ret = CAEN_DGTZ_OpenDigitizer(Params.ConnectionType[i], Params.LinkNum[i], Params.ConetNode[i], Params.BaseAddress[i], &handle[i+1]);
    if (ret) {
      printf("Can't open digitizer n. %d\n", i);
      goto QuitProgram;
    }
  }
  for (unsigned int i = 0; i < NumbOfDigiTime+NumbOfDigiCharge ; i++) {  //get info on time digitizers
    ret = CAEN_DGTZ_GetInfo(handle[i], &BoardInfo[i]);
    if (ret) {
      printf("Can't read board info for digitizer n. %d\n", i);
      goto QuitProgram;
    }
    /* Check BoardType */
    printf("Connected to CAEN Digitizer Model %s\n", BoardInfo[i].ModelName);
    printf("ROC FPGA Release is %s\n", BoardInfo[i].ROC_FirmwareRel);
    printf("AMC FPGA Release is %s\n", BoardInfo[i].AMC_FirmwareRel);
    printf("VME Handle is %d\n", BoardInfo[i].VMEHandle);
    printf("Serial Number is %d\n\n", BoardInfo[i].SerialNumber);
/*    printf("Correction Tables Loaded!\n\n");*/
  }
  
  /* GET BOARD INFO AND FW REVISION                                                          */
  /* Get num of channels, num of bit, num of group of the board from first board */
  Nbit = BoardInfo[1].ADC_NBits;  //N bit per event
  //printf("Nbit master %d\n\n", BoardInfo[0].ADC_NBits);
  Nch  = BoardInfo[1].Channels;   //N of groups!
  //printf("Nch master %d\n\n", BoardInfo[0].Channels);
  switch(Params.DRS4Frequency) {
    case CAEN_DGTZ_DRS4_5GHz: Ts = 0.2; break;
    case CAEN_DGTZ_DRS4_2_5GHz: Ts =  0.4; break;
    case CAEN_DGTZ_DRS4_1GHz: Ts =  1.0; break;
    default: Ts = 0.2; break;
  }
  
  Tt = 8.53; // step per il Local_TTT
  
  
  // configuration of board registers. needs to be done one by one or it's a real mess (for now...)
  
  // CHARGE DIGITIZER
  
/*  ret = ConfigureChargeDigitizer(handle)*/
  
  
  
  /* BOARD CONFIGURATION - TIMING DIGITIZERS                                                 */
  // Set registers for the acquisition (record length, channel mask, etc...)
  
  for(int i = 0 ; i < handle.size(); i++)
  {
    printf("handle[%d] = %d \n",i,handle[i]);
//     cHandle.push_back(handle[i]);
  }
  for(int i = 1 ; i < handle.size(); i++)
  {
    cHandle.push_back(handle[i]);
    cBoardInfo.push_back(BoardInfo[i]);
  }
  
  for(int i = 0 ; i < handle.size(); i++)
  {
    ret = CAEN_DGTZ_Reset(handle[i]);
    printf("ret after reset [%d] = %d \n",i,ret);
  }
  
  
  
  ret = ConfigureDigitizers(cHandle, cBoardInfo, Params, configDigiTime); // configure just for timing digitizers 
  
  
//   printf("ret %d \n",ret);
  if (ret) {
    printf("Errors occurred during time digitizers configuration\n");
    goto QuitProgram;
  }
  // Set registers for the synchronization (start mode, trigger masks, signals propagation, etc...)
  ret |= SetSyncMode(handle, Params);
  //printf("ret %d \n",ret);
  if (ret) {
    printf("Errors occurred during set sync mode \n");
    goto QuitProgram;
  }

  /* MEMORY ALLOCATION - TIMING DIGITIZERS                                                   */
//   for(unsigned int i = 0; i < Params.NumOfDigitizers; ++i) {
//     uint32_t AllocatedSize;
//     /* Memory allocation for event buffer and readout buffer */
//     ret = CAEN_DGTZ_AllocateEvent(handle[i],(void**)&Event742[i]);
//     /* NOTE : This malloc must be done after the digitizer programming */
//     ret |= CAEN_DGTZ_MallocReadoutBuffer(handle[i], &buffer[i], &AllocatedSize);
//     if (ret) {
//       printf("Can't allocate memory for the acquisition\n");
//       goto QuitProgram;
//     }
//   }

  
   


  // CHECK CONFIGURATIONS
  

  // Allocate memory for the histograms
  // histoT = malloc(Params.HistoNbins * sizeof(uint32_t));
  // memset(histoT, 0, Params.HistoNbins * sizeof(uint32_t));

  /* *************************************************************************************** */
  /* OPEN FILES                                                                              */
  /* *************************************************************************************** */
//   if (Params.EnableLog) log = fopen("events.txt", "w");  // Open Output Files
//   plotter = popen(GNUPLOTEXE, "w");    // Open plotter pipe (gnuplot)
// 
//   binOut = fopen("binary.dat","wb");
//   textOut = fopen("text.txt","w");

  /* *************************************************************************************** */
  /* CHECK CLOCK ALIGNMENT                                                                   */
  /* *************************************************************************************** */
  printf("Boards Configured. Press [s] to start run or [c] to check clock alignment\n\n");
  c = getch();
  if (c == 'c') {
    uint32_t rdata[3];
    // propagate CLK to trgout on both (all?) boards
    for(unsigned int i=0; i<3; i++) {
      CAEN_DGTZ_ReadRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, &rdata[i]);
      CAEN_DGTZ_WriteRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, 0x00050000);
    }
    printf("Trigger Clk is now output on TRGOUT.\n");
    printf("Press [r] to reload PLL config, [s] to start acquisition, any other key to quit\n");
    while( (c=getch()) == 'r') {
      CAEN_DGTZ_WriteRegister(handle[0], ADDR_RELOAD_PLL, 0);
      ForceClockSync(handle[0]);
      printf("PLL reloaded\n");
    }
    for(unsigned int i=0; i<3; i++)
    CAEN_DGTZ_WriteRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, rdata[i]);
  }
  if (c != 's') goto QuitProgram;
  
  /* *************************************************************************************** */
    /* START RUN                                                                               */
    /* *************************************************************************************** */
  ForceClockSync(handle[0]);            // Force clock sync in board 0
  StartRun(handle, 0);  // Start Run
  running = 1;
  printf("Run started\n");


  /* *************************************************************************************** */
  /* READOUT LOOP                                                                            */
  /* *************************************************************************************** */
  PrevRateTime = get_time();
  

  while(!QuitAcquisition) {

    // --------------------------------------------
    // check for keyboard commands
    // /Data/ --------------------------------------------
    if (kbhit()) {

      c = getch();

      if (c == 't')
        SoftwareTrigger = 1;

      if (c == 'q')
        QuitAcquisition = 1;

      if (c == 'p'){
        plot = 1;
        ContinousWaveplot = 0;
      }

      if (c == 'w'){
        printf("Writing single output\n");
        write[0] = 1;
        ContinousWrite[0] = 0;
        write[1] = 1;
        ContinousWrite[1] = 0;
      }

      if (c == 'W'){
        for(unsigned int i = 0; i < Params.NumOfDigitizers ; i++)
        {
          if(!ContinousWrite[i])
          {
            write[i] = 1;
            ContinousWrite[i] = 1;
            printf("Writing output to text file %d\n",i);
          }
          else
          {
            write[i] = 0;
            ContinousWrite[i] = 0;
            printf("Stopping output writing to file %d\n",i);
          }
        }
      }

      if (c == 'P'){
        plot = 1;
        ContinousWaveplot = 1;
      }
      if (c == 'h') {
        PlotHist = 1;
        ContinousHistPlot = 0;
      }
      if (c == 'H') {
        PlotHist = 1;
        ContinousHistPlot = 1;
      }
//       if (c == 's') {
//         if(!running) {
//           for(unsigned int i = 0; i < Params.NumOfDigitizers ; i++)
//           {
//             GetNextEvent[i] = 1;
//             NumEvents[i] = 0;
//           }
//           SetSyncMode(handle, Params);
// 	  //ret = CAEN_DGTZ_SWStartAcquisition(handle[0]);
//           StartRun(handle,0);
//           running=1;
//           printf("Acquisition started\n\n");
//         } else {
//           StopRun(handle);
//           running=0;
//           printf("Acquisition stopped. Press s to restart\n\n");
//         }
//       }
      if (c == 's') {
	      if (!running) {
// 		GetNextEvent[0]=1;
// 		GetNextEvent[1]=1;
// 		NumEvents[0]=0;
// 		NumEvents[1]=0;
		SetSyncMode(handle, Params);
		StartRun(handle, 0);
		running=1;
		printf("Acquisition started\n\n");
	      } else {
		StopRun(handle);
		running=0;
		printf("Acquisition stopped. Press s to restart\n\n");
	      }
	    }
      if (c == 'r') {
        // memset(histoT, 0, Params.HistoNbins * sizeof(uint32_t));
        // MeanT=0.0;
        // MeanTTT=0.0;
        // SigmaT=0.0;
        // SigmaTTT=0.0;
        // NsT=0;
        // NsTTT=0;
      }
    }

    
    ret = run_acquisition();
    if (ret) {
      printf("Error during acquisition loop (ret = %d) .... Exiting\n", ret);
      break;
    }
    print_statistics();
//     int ret;
//     unsigned int i;
//     unsigned int j;
//     uint32_t     bin;
//     uint32_t     bsize;
//     uint32_t     NumEvents[MAX_CHANNELS];
//     
//     /* Send a SW Trigger if requested by user */
//     if (gSWTrigger) {
//       if (gSWTrigger == 1)    gSWTrigger = 0;
//       CAEN_DGTZ_SendSWtrigger(gHandle);
//     }
//     
//     memset(NumEvents, 0, MAX_CHANNELS*sizeof(NumEvents[0]));
//     
//     /* Read a block of data from the digitizer */
//     ret = CAEN_DGTZ_ReadData(gHandle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, gAcqBuffer, &bsize); /* Read the buffer from the digitizer */
//     gLoops += 1;
//     if (ret) {
//       printf("Readout Error\n");
//       return -1;
//     }
//     if (bsize == 0)
//       return 0;
//     
//     /* Decode and analyze Events */
//     if ( (ret = _CAEN_DGTZ_GetDPPEvents(gHandle, gAcqBuffer, bsize, (void **)gEvent, NumEvents)) != CAEN_DGTZ_Success)
//     {
//       printf("Errro during _CAEN_DGTZ_GetDPPEvents() call (ret = %d): exiting ....\n", ret);
//       exit(-1);
//     }
    
    // ----------------------------------------------------------------
    // Calculate and print throughput and trigger rate (every second)
    // ----------------------------------------------------------------
//     CurrentTime = get_time();
//     ElapsedTime = CurrentTime - PrevRateTime;
//     if (ElapsedTime > 1000) {
//       printf("Readout Rate=%.2f MB\n", (float)Nb/((float)ElapsedTime*1048.576f));
//       for(unsigned int i=0; i<Params.NumOfDigitizers; i++) {
//         if (TrgCnt[i]>0) {
//           printf("Board %d:\tTrgRate=%.2f KHz. Matching Events=%.2f%%; ", i, (float)TrgCnt[i]/(float)ElapsedTime, 100*(float)MatchingEvents/(float)TrgCnt[i]/*, 100*(float)missingEdge[i]/(float)MatchingEvents*/);
//           //BEGIN DEBUG//
//           //printf("buffer size: %d\n",BufferSize[0]);
//           printf("number of event in the current buffer: %d\n",NumEvents[i]);
//           //printf("trigger time tag: %d\n", Event742[i]->DataGroup[Params.RefChannel[i]/8].TriggerTimeTag);
//           //END DEBUG//
//           if (MatchingEvents>0)
//           printf("Missing Edges=%.2f%%\n", 100*(float)missingEdge[i]/(float)MatchingEvents);
//           else
//           printf("No edge found\n");
//         } else {
//           printf("Board %d:\tNo Data\n", i);
//         }
//         TrgCnt[i]=0;
//         missingEdge[i]=0;
//       }
//       printf("DeltaT Edges:    mean=%.4f  sigma=%.4f on %lu data\n", MeanT/NsT, sqrt(SigmaT/NsT-(MeanT*MeanT)/(NsT*NsT)),NsT);
//       printf("DeltaT Channel Time Tag: mean=%.4f  sigma=%.4f on %lu data\n", MeanTTT/NsTTT, sqrt(SigmaTTT/NsTTT-(MeanTTT*MeanTTT)/(NsTTT*NsTTT)), NsTTT);
//       Nb = 0;
//       MatchingEvents=0;
//       if (PlotHist) {  //histograms plotting will be added here
//       //   FILE *hist_file;
//       //   if (!ContinousHistPlot)
//       //   PlotHist = 0;
//       //   hist_file = fopen("hist.txt", "w");
//       //   for (i = 0; i < Params.HistoNbins; ++i)
//       //   fprintf(hist_file, "%f\t%d\n", (float)(i*Params.HistoBinSize + Params.HistoOffset), histoT[i]);
//       //   fclose(hist_file);
//       //   if (plotter) {
//       //     fprintf(plotter, "set xlabel 'Time Difference [ns]' \n");
//       //     fprintf(plotter, "plot 'hist.txt' w boxes fs solid 0.7\n");
//       //     fflush(plotter);
//       //   }
//       }
//       PrevRateTime = CurrentTime;
//       printf("\n\n");
//     }

    //         if (SoftwareTrigger) {
    // 	  printf("SoftwareTrigger issued \n");
    // 	    CAEN_DGTZ_SendSWtrigger(handle[0]);
    //             CAEN_DGTZ_SendSWtrigger(handle[1]);
    //                 }

    // ----------------------------------------------------------------
    // Read data
    // ----------------------------------------------------------------
    //             int cycle = 0;


    // ----------------------------------------------------------------
    // Read data
    // ----------------------------------------------------------------
//     for(unsigned int i=0; i<Params.NumOfDigitizers; i++) {
//       if (GetNextEvent[i]) {
// 
//         /* read a new data block from the board if there are no more events to use in the readout buffer */
//         if (EIndx[i] >= NumEvents[i]) {
//           EIndx[i] = 0;
//           /* Read a data block from the board */
//           ret = CAEN_DGTZ_ReadData(handle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer[i], &BufferSize[i]);
//           // 	      		    printf("buffersize %d\n",BufferSize[i]);
//           Nb += BufferSize[i];
//           if (BufferSize[i]>0)
//           ret=ret;
//           ret |= CAEN_DGTZ_GetNumEvents(handle[i], buffer[i], BufferSize[i], &NumEvents[i]);
//           // 	      printf("NumEvents[%d] = %d\n",i,NumEvents[i]);
//           if (ret) {
//             printf("Readout Error\n");
//             goto QuitProgram;
//           }
//         }
//         /* Get one event from the readout buffer */
//         if (NumEvents[i]) {
//           // 	      printf("NumEvents[%d] = %d\n",i,NumEvents[i]);
//           // 	      printf("EIndx[%d] = %d\n",i,EIndx[i]);
//           ret = CAEN_DGTZ_GetEventInfo(handle[i], buffer[i], BufferSize[i], EIndx[i], &EventInfo[i], &EventPtr[i]);
//           ret = CAEN_DGTZ_DecodeEvent(handle[i], EventPtr[i], (void**)&Event742[i]);
//           TrgCnt[i]++;
//           if (ret) {
//             printf("Event build error\n");
//             goto QuitProgram;
//           }
//           GetNextEvent[i]=0;
//         }
//       }
//     }

//     bool missingData = false;
//     for(unsigned int i=0; i<Params.NumOfDigitizers; i++)
//        missingData |= GetNextEvent[i];
//     if (missingData)  // missing data from one or both boards
//     continue;

    // ----------------------------------------------------------------
    // Analyze data
    // ----------------------------------------------------------------
    // calculate extended Trigger Time Tag (take roll over into account)
//     for(unsigned int i=0; i<Params.NumOfDigitizers; i++)
//     {
//       double internalTTT[4];
//       int gr=0;
//       double sum =0;
//       int counter =0;
//       for(gr=0; gr<4; gr++)
//       {
//         internalTTT[gr] = ((Nroll[i]<<30) + (Event742[i]->DataGroup[gr].TriggerTimeTag & 0x3FFFFFFF))*Tt;
//         if(internalTTT[gr]!=0)
//         {
//           counter++;
//           sum += internalTTT[gr];
//         }
//       }
//       TTT[i] = sum / ( (double) counter );
// 
//       // 	      if (Event742[i]->GrPresent[(Params.RefChannel[i]/8)])
//       // 	      {
//       // 		TTT[i] = ((Nroll[i]<<30) + (Event742[i]->DataGroup[(Params.RefChannel[i]/8)].TriggerTimeTag & 0x3FFFFFFF))*Tt;
//       if (TTT[i] < PrevTTT[i]) {
//         Nroll[i]++;
//         TTT[i] += (1<<30)*Tt;
//       }
//       PrevTTT[i] = TTT[i];
//       // 	      }
//       // 	      else{
//       // 		EvNoData[i]++;
//       // 	      }
//     }

    // rewrite of check matching window for being compatible with multi board (more than two)
    // all boards have to be aligned with Master, or we pass to next event in that board
    //TODO - for now is 0 and 1 like before!!!

    // std::sort(TTT.begin(),TTT.end());
//     if (TTT[0] < (TTT[1] - Params.MatchingWindow*Tt)) {
//       EIndx[0]++;
//       // 	    printf("board 0 is behind board 1\n");
//       GetNextEvent[0]=1;
//       continue;
//       // CASE2: board 1 is behind board 0; keep event from 0 and take next event from 1
//     } else if (TTT[1] < (TTT[0] - Params.MatchingWindow*Tt)) {
//       EIndx[1]++;
//       GetNextEvent[1]=1;
//       // 	    printf("board 1 is behind board 0\n");
//       continue;
//       // CASE3: trigger time tags match: calculate DeltaT between edges
//     }
//     else
//     {
//       MatchingEvents++;
//       for(unsigned int i=0; i<Params.NumOfDigitizers; i++)
//       {
// 
//         EIndx[i]++;
//         GetNextEvent[i]=1;
//         if (write[i]) {
// 
//           if (!ContinousWrite[i]) {
//             write[i] = 0;
//           }
//           int gr =0;
//           for(gr =0 ; gr < 4 ; gr++)
//           {
//             ApplyDataCorrection(gr, 7,Params.DRS4Frequency, &(Event742[i]->DataGroup[gr]), &Table[i]);
//           }
//           printf("TTT[%d] = %lf\n",i,TTT[i]);
//           char name[12];
//           sprintf(name,"board%d.txt",i);
//           if(!ContinousWrite[i])
//           board[i] = fopen(name,"w");
//           else
//           board[i] = fopen(name,"a");
//           int ch=0;
// 
//           int iSample = 0;
//           for( iSample = 0 ; iSample < Params.RecordLength ; iSample++)
//           {
//             for(gr =0 ; gr < 4 ; gr++)
//             {
//               // 		 ApplyDataCorrection(gr, 7,Params.DRS4Frequency, &(Event742[i]->DataGroup[gr]), &Table[i]);
//               for(ch =0 ; ch < 9 ; ch++)
//               {
//                 fprintf(board[i],"%f ",Event742[i]->DataGroup[gr].DataChannel[ch][iSample]);
//               }
//             }
//             fprintf(board[i],"\n");
//           }
//           fclose(board[i]);
// 
//           
//         }
//       }
// 
//     }

    
  }
//   error=0;

  QuitProgram:

//   if (error)
//   getch();

  /* *************************************************************************************** */
  /* FINAL CLEANUP                                                                           */
  /* *************************************************************************************** */
  /* stop the acquisition */
//   CAEN_DGTZ_SWStopAcquisition(handle[0]); //stop should be propagated to the others 
  cleanup_on_exit(); //stop and cleanup for charge digitizer
//   for (unsigned int i = 1; i < 3; i++) { //clean buffers for timing digi...
//     /* close the device and free the buffers */
//     CAEN_DGTZ_FreeEvent(handle[i], (void**)&Event742);
//     CAEN_DGTZ_FreeReadoutBuffer(&buffer[i]);  
//   }
  
  /* close connection to timing boards */
  for (unsigned int i = 1; i < 3; i++) {
    CAEN_DGTZ_CloseDigitizer(handle[i]);
  }
  // if (histoT)    free(histoT);
  /* close open files */
//   if (log)       fclose(log);
//   if (plotter)   pclose(plotter);

//   fclose(binOut); //debug
//   fclose(textOut);
  //            fclose(board[0]);
  //            fclose(board[1]);


  return 0;
}
