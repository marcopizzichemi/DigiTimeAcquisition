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

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <CAENDigitizer.h>

#include "acquisition.h"
#include "X742CorrectionRoutines.h"
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





/* ============================================================================== */
int ConfigureDigitizers(int handle[2], CAEN_DGTZ_BoardInfo_t Binfo[2], UserParams_t Params)
{
  int i, ret = 0;
  uint32_t d32;

  for(i=0; i<2; i++) {
    /* Reset all board registers */
    ret |= CAEN_DGTZ_Reset(handle[i]);

    if (Params.TestPattern)
    ret |= CAEN_DGTZ_WriteRegister(handle[i], CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);

    if (Params.DRS4Frequency) {
      ret |= CAEN_DGTZ_SetDRS4SamplingFrequency(handle[i], Params.DRS4Frequency);
    }

    ret |= CAEN_DGTZ_SetRecordLength(handle[i], Params.RecordLength);
    ret |= CAEN_DGTZ_SetPostTriggerSize(handle[i], Params.PostTrigger[i]);
    ret |= CAEN_DGTZ_SetIOLevel(handle[i], Params.IOlevel);
    ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle[i], MAX_EVENTS_XFER);

    //     gr=Params.RefChannel[i]/8;
    //     ret |= CAEN_DGTZ_SetGroupEnableMask(handle[i], 1<<gr);
    ret |= CAEN_DGTZ_SetGroupEnableMask(handle[i], 0x000F);
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
    /* DO NOT CHANGE : Patch for 1024 or 1023 memory segments */
    /*ret |= CAEN_DGTZ_ReadRegister(handle[i], 0x8174, &rdata);
    *        if ((rdata == 0x400) || (rdata == 0x3FF))
    *                ret |= CAEN_DGTZ_WriteRegister(handle[i], 0x8174, 0x3FE);*/

    ///     Register Setting common to master and slave   ///

    //     setting the TR DC OFFSET and TR TRESHOLD
    //TR DC OFFSET for nim signals
    ret |= CAEN_DGTZ_WriteRegister(handle[i],0x10DC,0x8000); //offset
    // TR THRESHOLD for nim signals
    ret |= CAEN_DGTZ_WriteRegister(handle[i],0x10D4,0x51C6); //threshold (old config = 8800)
    //ret |= CAEN_DGTZ_ReadRegister(handle[i],0x10D4,&d32);
    //printf("0x10D4 mod -  %x\n",d32);
    // read fast trigger register
    //      ret |= CAEN_DGTZ_GetFastTriggerMode (handle[i], &d32);
    //      printf("0x8000 -  %x\n",d32);
    ///     ___      ///
  }

  //BEGIN of DEBUG OUTPUT
  for(i=0; i<2; i++) {
    int gr,ch;
    uint32_t TValue;
    CAEN_DGTZ_TriggerPolarity_t TriggerPolarity;
    CAEN_DGTZ_TriggerMode_t mode;
    CAEN_DGTZ_EnaDis_t enable;
    for(gr = 0 ; gr < 4 ; gr++)
    {
      ret |= CAEN_DGTZ_GetGroupFastTriggerThreshold(handle[i], gr, &TValue);
      printf("CAEN_DGTZ_GetGroupFastTriggerThreshold - board %d group %d  = %d\n",i,gr,TValue);
      ret |= CAEN_DGTZ_GetGroupFastTriggerDCOffset(handle[i], gr, &TValue);
      printf("CAEN_DGTZ_GetGroupFastTriggerDCOffset - board %d group %d = %d\n",i,gr,TValue);
      ret |= CAEN_DGTZ_GetTriggerPolarity(handle[i],gr,&TriggerPolarity);
      printf("CAEN_DGTZ_GetTriggerPolarity - board %d group %d = %d\n",i,gr,TriggerPolarity);
    }
    ret |= CAEN_DGTZ_GetFastTriggerMode(handle[i],&mode);
    printf("CAEN_DGTZ_GetFastTriggerMode - board %d = %d\n",i,mode);
    ret |= CAEN_DGTZ_GetFastTriggerDigitizing(handle[i],&enable);
    printf("CAEN_DGTZ_GetFastTriggerDigitizing - board %d = %d\n",i,enable);

    for(ch = 0 ; ch < 32 ; ch++)
    {
      ret |= CAEN_DGTZ_GetChannelDCOffset(handle[i], ch,&TValue);
      printf("CAEN_DGTZ_GetChannelDCOffset - board %d channel %d = %d\n",i,ch,TValue);
    }
  }
  //END of DEBUG OUTPUT

  ///     Register Setting, separately    ///

  ret |= CAEN_DGTZ_WriteRegister(handle[0],0x811C,0x0904); //old value set
  ret |= CAEN_DGTZ_WriteRegister(handle[1],0x811C,0x000d0900); //
  ret |= CAEN_DGTZ_WriteRegister(handle[0],0x81A0,0x2); //
  ret |= CAEN_DGTZ_WriteRegister(handle[1],0x81A0,0x2); //
  ret |= CAEN_DGTZ_WriteRegister(handle[1],0x8110,0x80000100); //
  ret |= CAEN_DGTZ_WriteRegister(handle[0],0x8000,0x00029950); //
  ret |= CAEN_DGTZ_WriteRegister(handle[1],0x8000,0x00029950); //
  //
  //
  //     ret |= CAEN_DGTZ_ReadRegister(handle[0],0x811C,&d32);
  //      printf("0x811C mod master -  %x\n",d32);
  //     ret |= CAEN_DGTZ_ReadRegister(handle[0],0x81A0,&d32);
  //      printf("0x81A0 mod master -  %x\n",d32);
  ret |= CAEN_DGTZ_ReadRegister(handle[0],0x8120,&d32);
  printf("0x8000 mod master -  %x\n",d32);
  ret |= CAEN_DGTZ_ReadRegister(handle[1],0x8120,&d32);
  printf("0x8000 mod slave -  %x\n",d32);

  //
  //     ret |= CAEN_DGTZ_ReadRegister(handle[1],0x811C,&d32);
  //      printf("0x811C mod slave -  %x\n",d32);
  //     ret |= CAEN_DGTZ_ReadRegister(handle[1],0x81A0,&d32);
  //      printf("0x81A0 mod slave -  %x\n",d32);
  //     ret |= CAEN_DGTZ_ReadRegister(handle[1],0x8000,&d32);
  //      printf("0x8000 mod slave -  %x\n",d32);
  //     ret |= CAEN_DGTZ_ReadRegister(handle[1],0x8110,&d32);
  //      printf("0x8110 mod slave -  %x\n",d32);
  // ///     ___      ///



  return ret;
}

/* ============================================================================== */
int SetSyncMode(int handle[2], UserParams_t Params)
{
  int i, ret=0;
  uint32_t reg,d32;

  for(i=0; i<2; i++) {
    if (i > 0)  // Run starts with S-IN on the 2nd board
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_ACQUISITION_MODE, RUN_START_ON_SIN_LEVEL);
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_GLOBAL_TRG_MASK, (1<<30/*|1<<Params.RefChannel[i])*/));  // accept only trg from selected channel --
    // 	ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_GLOBAL_TRG_MASK, (1<<30|1<<5));  // accept only trg from selected channel

    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_TRG_OUT_MASK, 0);   // no tigger propagation to TRGOUT
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_RUN_DELAY, 0);   // Run Delay decreases with the position (to compensate for run the propagation delay)
    //      ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_RUN_DELAY, 2*(1-i));   // Run Delay decreases with the position (to compensate for run the propagation delay)

    // Set TRGOUT=RUN to propagate run through S-IN => TRGOUT daisy chain
    ret |= CAEN_DGTZ_ReadRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, &reg);
    reg = reg & 0xFFF0FFFF | 0x00010000;
    ret |= CAEN_DGTZ_WriteRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, reg);
  }

  for(i=0; i<2; i++) {
    ret  |= CAEN_DGTZ_ReadRegister(handle[0],0x810C,&d32);
    printf("SetSyncMode 0x810C[%d] = %x\n",i,d32);
  }

  //   ret  |= CAEN_DGTZ_ReadRegister(handle[0],0x10A8,&d32);
  //   printf("SetSyncMode 0x10A8 = %x\n",i,d32);
  //   ret  |= CAEN_DGTZ_ReadRegister(handle[0],0x11A8,&d32);
  //   printf("SetSyncMode 0x11A8 = %x\n",i,d32);
  //   ret  |= CAEN_DGTZ_ReadRegister(handle[0],0x12A8,&d32);
  //   printf("SetSyncMode 0x12A8 = %x\n",i,d32);
  //   ret  |= CAEN_DGTZ_ReadRegister(handle[0],0x13A8,&d32);
  //   printf("SetSyncMode 0x13A8 = %x\n",i,d32);


  return ret;
}

/* ============================================================================== */
int StartRun(int handle[2], int StartMode)
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
int StopRun(int handle[2])
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
/* MAIN                                                    SetSyncMode                    */
/* ########################################################################### */
int main(int argc, char *argv[])
{

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

  // if(argc > 1)
  // {
  SetUserParams(&Params,config);
  // }
  // else
  // {
  //   SetDefaultParams(&Params);
  // }

  // SetUserParams(&Params,config);

  // ConfigFile config;
  int cycle = 0;
  int i, ret=0, Nbit, Nch, error=1, running=0;
  int EvNoData[2]={0,0};
  int GetNextEvent[2]={1,1};
  int QuitAcquisition=0, plot=0, PlotHist=0, ContinousHistPlot=0,SoftwareTrigger =0,ContinousWaveplot=0;
  int write[2]={0,0};
  int ContinousWrite[2]={0,0};
  int handle[2];
  char *buffer[2]={NULL, NULL}, *EventPtr[2]={NULL, NULL};
  CAEN_DGTZ_BoardInfo_t  BoardInfo[2];
  CAEN_DGTZ_EventInfo_t  EventInfo[2];
  CAEN_DGTZ_X742_EVENT_t *Event742[2] = {NULL, NULL};
  float *Wave[2];
  float *Trigger[2];



  uint32_t Ns[2], EIndx[2]={0,0}, NumEvents[2]={0,0}, BufferSize[2]={0,0}, TrgCnt[2]={0,0}, missingEdge[2]={0,0};
  uint32_t Nb=0, MatchingEvents=0;
  // uint32_t *histoT = NULL;
  uint64_t CurrentTime, PrevRateTime, ElapsedTime, NsT=0, NsTTT=0, Nroll[2]={0,0},Head_Nroll[2]={0,0};
  char c;
  double Ts, Tt;
  double TTT[2], PrevTTT[2] = {0,0}, DeltaT, DeltaTTT, PulseEdgeTime[2], TrEdgeTime[2];
  double MeanT=0.0, MeanTTT=0.0, SigmaT=0.0, SigmaTTT=0.0;

  //   int iPrev,jPrev;
  //   for(iPrev = 0; iPrev < 2 ; iPrev++)
  //   {
  //     for(jPrev = 0; jPrev < 4 ; jPrev++)
  //     {
  //       PrevTTT[iPrev][jPrev] = 0;
  //     }
  //   }

  DataCorrection_t Table[2];

  FILE *log        = NULL;
  FILE *event_file = NULL;
  FILE *board[2] ={ NULL,NULL};
  //   FILE *board1 = NULL;
  FILE *plotter;

  FILE *binOut = NULL;
  FILE *textOut = NULL;

  printf("\n");
  printf("**************************************************************\n");
  printf(" CAEN V1742 Acquisition Software \n");
  printf("**************************************************************\n\n");




  /* *************************************************************************************** */
  /* OPEN DIGITIZERS                                                                         */
  /* *************************************************************************************** */
  for (i = 0; i < 2; i++) {
    ret = CAEN_DGTZ_OpenDigitizer(Params.ConnectionType[i], Params.LinkNum[i], Params.ConetNode[i], Params.BaseAddress[i], &handle[i]);
    if (ret) {
      printf("Can't open digitizer n. %d\n", i);
      goto QuitProgram;
    }
  }

  /* *************************************************************************************** */
  /* GET BOARD INFO AND FW REVISION                                                          */
  /* *************************************************************************************** */
  for (i = 0; i < 2; i++) {
    ret = CAEN_DGTZ_GetInfo(handle[i], &BoardInfo[i]);
    if (ret) {
      printf("Can't read board info for digitizer n. %d\n", i);
      goto QuitProgram;
    }
    /* Check BoardType */
    if (BoardInfo[i].FamilyCode!= 6) {
      printf("This digitizer is not a V1742; V1742TestSync doesn't support it\n");
      goto QuitProgram;
    }
    if (ret = LoadCorrectionTables(handle[i], &Table[i], 0, Params.DRS4Frequency))
    goto QuitProgram;

    printf("Connected to CAEN Digitizer Model %s\n", BoardInfo[i].ModelName);
    printf("ROC FPGA Release is %s\n", BoardInfo[i].ROC_FirmwareRel);
    printf("AMC FPGA Release is %s\n\n", BoardInfo[i].AMC_FirmwareRel);
    printf("Correction Tables Loaded!\n\n");
  }


  /* Get num of channels, num of bit, num of group of the board from first board */
  Nbit = BoardInfo[0].ADC_NBits;  //N bit per event
  printf("Nbit master %d\n\n", BoardInfo[0].ADC_NBits);
  Nch  = BoardInfo[0].Channels;   //N of groups!
  printf("Nch master %d\n\n", BoardInfo[0].Channels);
  switch(Params.DRS4Frequency) {
    case CAEN_DGTZ_DRS4_5GHz: Ts = 0.2; break;
    case CAEN_DGTZ_DRS4_2_5GHz: Ts =  0.4; break;
    case CAEN_DGTZ_DRS4_1GHz: Ts =  1.0; break;
    default: Ts = 0.2; break;
  }

  Tt = 8.53; // step per il Local_TTT

  /* *************************************************************************************** */
  /* BOARD CONFIGURATION                                                                     */
  /* *************************************************************************************** */
  // Set registers for the acquisition (record length, channel mask, etc...)
  ret = ConfigureDigitizers(handle, BoardInfo, Params);
  // Set registers for the synchronization (start mode, trigger masks, signals propagation, etc...)
  ret |= SetSyncMode(handle, Params);
  if (ret) {
    printf("Errors occurred during digitizer configuration\n");
    goto QuitProgram;
  }



  /* *************************************************************************************** */
  /* MEMORY ALLOCATION                                                                       */
  /* *************************************************************************************** */
  for(i = 0; i < 2; ++i) {
    uint32_t AllocatedSize;
    /* Memory allocation for event buffer and readout buffer */
    ret = CAEN_DGTZ_AllocateEvent(handle[i],(void**)&Event742[i]);
    /* NOTE : This malloc must be done after the digitizer programming */
    ret |= CAEN_DGTZ_MallocReadoutBuffer(handle[i], &buffer[i], &AllocatedSize);
    if (ret) {
      printf("Can't allocate memory for the acquisition\n");
      goto QuitProgram;
    }
  }

  // Allocate memory for the histograms
  // histoT = malloc(Params.HistoNbins * sizeof(uint32_t));
  // memset(histoT, 0, Params.HistoNbins * sizeof(uint32_t));

  /* *************************************************************************************** */
  /* OPEN FILES                                                                              */
  /* *************************************************************************************** */
  if (Params.EnableLog)
  log = fopen("events.txt", "w");  // Open Output Files
  //     board[0] = fopen("board0.txt", "w");
  //     board[1] = fopen("board1.txt", "w");
  plotter = popen(GNUPLOTEXE, "w");    // Open plotter pipe (gnuplot)

  binOut = fopen("binary.dat","wb");
  textOut = fopen("text.txt","w");

  /* *************************************************************************************** */
  /* CHECK CLOCK ALIGNMENT                                                                   */
  /* *************************************************************************************** */
  printf("Boards Configured. Press [s] to start run or [c] to check clock alignment\n\n");
  c = getch();
  if (c == 'c') {
    uint32_t rdata[2];
    // propagate CLK to trgout on both boards
    for(i=0; i<2; i++) {
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
    for(i=0; i<2; i++)
    CAEN_DGTZ_WriteRegister(handle[i], ADDR_FRONT_PANEL_IO_SET, rdata[i]);
  }
  if (c != 's')
  goto QuitProgram;

  /* *************************************************************************************** */
  /* START RUN                                                                               */
  /* *************************************************************************************** */
  ForceClockSync(handle[0]);            // Force clock sync in board 0
  StartRun(handle, Params.StartMode);  // Start Run
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
        int i =0;
        // 	  if(!ContinousWrite[0] && !ContinousWrite[1])
        //
        // 	  else

        for(i = 0; i < 2 ; i++)
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
      if (c == 's') {
        if(!running) {
          GetNextEvent[0]=1;
          GetNextEvent[1]=1;
          NumEvents[0]=0;
          NumEvents[1]=0;
          SetSyncMode(handle, Params);
          StartRun(handle, Params.StartMode);
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


    // ----------------------------------------------------------------
    // Calculate and print throughput and trigger rate (every second)
    // ----------------------------------------------------------------
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PrevRateTime;
    if (ElapsedTime > 1000) {
      printf("Readout Rate=%.2f MB\n", (float)Nb/((float)ElapsedTime*1048.576f));
      for(i=0; i<2; i++) {
        if (TrgCnt[i]>0) {
          printf("Board %d:\tTrgRate=%.2f KHz. Matching Events=%.2f%%; ", i, (float)TrgCnt[i]/(float)ElapsedTime, 100*(float)MatchingEvents/(float)TrgCnt[i]/*, 100*(float)missingEdge[i]/(float)MatchingEvents*/);
          //BEGIN DEBUG//
          //printf("buffer size: %d\n",BufferSize[0]);
          printf("number of event in the current buffer: %d\n",NumEvents[0]);
          //printf("trigger time tag: %d\n", Event742[i]->DataGroup[Params.RefChannel[i]/8].TriggerTimeTag);
          //END DEBUG//
          if (MatchingEvents>0)
          printf("Missing Edges=%.2f%%\n", 100*(float)missingEdge[i]/(float)MatchingEvents);
          else
          printf("No edge found\n");
        } else {
          printf("Board %d:\tNo Data\n", i);
        }
        TrgCnt[i]=0;
        missingEdge[i]=0;
      }
      printf("DeltaT Edges:    mean=%.4f  sigma=%.4f on %lu data\n", MeanT/NsT, sqrt(SigmaT/NsT-(MeanT*MeanT)/(NsT*NsT)),NsT);
      printf("DeltaT Channel Time Tag: mean=%.4f  sigma=%.4f on %lu data\n", MeanTTT/NsTTT, sqrt(SigmaTTT/NsTTT-(MeanTTT*MeanTTT)/(NsTTT*NsTTT)), NsTTT);
      Nb = 0;
      MatchingEvents=0;
      if (PlotHist) {  //histograms plotting will be added here
      //   FILE *hist_file;
      //   if (!ContinousHistPlot)
      //   PlotHist = 0;
      //   hist_file = fopen("hist.txt", "w");
      //   for (i = 0; i < Params.HistoNbins; ++i)
      //   fprintf(hist_file, "%f\t%d\n", (float)(i*Params.HistoBinSize + Params.HistoOffset), histoT[i]);
      //   fclose(hist_file);
      //   if (plotter) {
      //     fprintf(plotter, "set xlabel 'Time Difference [ns]' \n");
      //     fprintf(plotter, "plot 'hist.txt' w boxes fs solid 0.7\n");
      //     fflush(plotter);
      //   }
      }
      PrevRateTime = CurrentTime;
      printf("\n\n");
    }

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
    for(i=0; i<2; i++) {
      if (GetNextEvent[i]) {

        /* read a new data block from the board if there are no more events to use in the readout buffer */
        if (EIndx[i] >= NumEvents[i]) {
          EIndx[i] = 0;
          /* Read a data block from the board */
          ret = CAEN_DGTZ_ReadData(handle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer[i], &BufferSize[i]);
          // 	      		    printf("buffersize %d\n",BufferSize[i]);
          Nb += BufferSize[i];
          if (BufferSize[i]>0)
          ret=ret;
          ret |= CAEN_DGTZ_GetNumEvents(handle[i], buffer[i], BufferSize[i], &NumEvents[i]);
          // 	      printf("NumEvents[%d] = %d\n",i,NumEvents[i]);
          if (ret) {
            printf("Readout Error\n");
            goto QuitProgram;
          }
        }
        /* Get one event from the readout buffer */
        if (NumEvents[i]) {
          // 	      printf("NumEvents[%d] = %d\n",i,NumEvents[i]);
          // 	      printf("EIndx[%d] = %d\n",i,EIndx[i]);
          ret = CAEN_DGTZ_GetEventInfo(handle[i], buffer[i], BufferSize[i], EIndx[i], &EventInfo[i], &EventPtr[i]);
          ret = CAEN_DGTZ_DecodeEvent(handle[i], EventPtr[i], (void**)&Event742[i]);
          TrgCnt[i]++;
          if (ret) {
            printf("Event build error\n");
            goto QuitProgram;
          }
          GetNextEvent[i]=0;
        }
      }
    }
    if (GetNextEvent[0] || GetNextEvent[1])  // missing data from one or both boards
    continue;

    // ----------------------------------------------------------------
    // Analyze data
    // ----------------------------------------------------------------
    // calculate extended Trigger Time Tag (take roll over into account)
    for(i=0; i<2; i++)
    {
      double internalTTT[4];
      int gr=0;
      double sum =0;
      int counter =0;
      for(gr=0; gr<4; gr++)
      {
        internalTTT[gr] = ((Nroll[i]<<30) + (Event742[i]->DataGroup[gr].TriggerTimeTag & 0x3FFFFFFF))*Tt;
        if(internalTTT[gr]!=0)
        {
          counter++;
          sum += internalTTT[gr];
        }
      }
      TTT[i] = sum / ( (double) counter );

      // 	      if (Event742[i]->GrPresent[(Params.RefChannel[i]/8)])
      // 	      {
      // 		TTT[i] = ((Nroll[i]<<30) + (Event742[i]->DataGroup[(Params.RefChannel[i]/8)].TriggerTimeTag & 0x3FFFFFFF))*Tt;
      if (TTT[i] < PrevTTT[i]) {
        Nroll[i]++;
        TTT[i] += (1<<30)*Tt;
      }
      PrevTTT[i] = TTT[i];
      // 	      }
      // 	      else{
      // 		EvNoData[i]++;
      // 	      }
    }

    if (TTT[0] < (TTT[1] - Params.MatchingWindow*Tt)) {
      EIndx[0]++;
      // 	    printf("board 0 is behind board 1\n");
      GetNextEvent[0]=1;
      continue;
      // CASE2: board 1 is behind board 0; keep event from 0 and take next event from 1
    } else if (TTT[1] < (TTT[0] - Params.MatchingWindow*Tt)) {
      EIndx[1]++;
      GetNextEvent[1]=1;
      // 	    printf("board 1 is behind board 0\n");
      continue;
      // CASE3: trigger time tags match: calculate DeltaT between edges
    } else
    {
      MatchingEvents++;
      for(i=0; i<2; i++)
      {

        EIndx[i]++;
        GetNextEvent[i]=1;
        if (write[i]) {

          if (!ContinousWrite[i]) {
            write[i] = 0;
          }
          int gr =0;
          for(gr =0 ; gr < 4 ; gr++)
          {
            ApplyDataCorrection(gr, 7,Params.DRS4Frequency, &(Event742[i]->DataGroup[gr]), &Table[i]);
          }
          printf("TTT[%d] = %lf\n",i,TTT[i]);
          char name[12];
          sprintf(name,"board%d.txt",i);
          if(!ContinousWrite[i])
          board[i] = fopen(name,"w");
          else
          board[i] = fopen(name,"a");
          int ch=0;

          int iSample = 0;
          for( iSample = 0 ; iSample < 1024 ; iSample++)
          {
            for(gr =0 ; gr < 4 ; gr++)
            {
              // 		 ApplyDataCorrection(gr, 7,Params.DRS4Frequency, &(Event742[i]->DataGroup[gr]), &Table[i]);
              for(ch =0 ; ch < 9 ; ch++)
              {
                fprintf(board[i],"%f ",Event742[i]->DataGroup[gr].DataChannel[ch][iSample]);
              }
            }
            fprintf(board[i],"\n");
          }
          fclose(board[i]);

          // 	    if(plot[i]){
          // 	    if (!ContinousPlot[i]) {
          // 	      plot[i] = 0;
          // 	    }
          //
          // 	    }
          //

          // 	    fprintf(plotter, "set xlabel 'Samples' \n");
          // 	    fprintf(plotter, "plot '%s' using ($0):($1) title 'Ch%d_Board0','event.txt' using ($0):($2) title 'TR%d_Board0','event.txt' using ($0):($3)  title 'Ch%d_Board1', 'event.txt' using ($0):($4) title 'TR%d_Board1'\n",name'
          // 		    Params.RefChannel[0],(Params.RefChannel[0]/16),Params.RefChannel[1], (Params.RefChannel[1]/16));
          // 	    fflush(plotter);
          // 	    SLEEP(1);
        }
      }

    }

    ////////////////////////////

    //         for(i=0; i<2; i++) {
    // 	  ret = CAEN_DGTZ_ReadData(handle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer[i], &BufferSize[i]);
    // 	  ret |= CAEN_DGTZ_GetNumEvents(handle[i], buffer[i], BufferSize[i], &NumEvents[i]);
    // 	  if (ret)
    // 	  {
    // 	    printf("Readout Error\n");
    // 	    goto QuitProgram;
    // 	  }
    // 	  int NEvents = NumEvents[i];
    // 	  int iEvent = 0;
    //
    // 	  for(iEvent = 0 ; iEvent <NEvents ; iEvent++)
    // 	  {
    // 	     char name[12];
    //
    // 	     ret = CAEN_DGTZ_GetEventInfo(handle[i], buffer[i], BufferSize[i], iEvent, &EventInfo[i], &EventPtr[i]);
    // 	     ret = CAEN_DGTZ_DecodeEvent(handle[i], EventPtr[i], (void**)&Event742[i]);
    // 	     if (ret)
    // 	     {
    // 		printf("Event build error\n");
    // 		goto QuitProgram;
    // 	     }
    // 	     int gr = 0;
    // 	     for(gr =0 ; gr < 4 ; gr++)
    // 	      {
    // 		if (Event742[i]->GrPresent[gr])
    // 		{
    // 		  TTT[i][gr] = ((Nroll[i]<<30) + (Event742[i]->DataGroup[gr].TriggerTimeTag & 0x3FFFFFFF))*Tt;
    // 		  if (TTT[i][gr] < PrevTTT[i][gr]) {
    // 		    Nroll[i]++;
    // 		    TTT[i][gr] += (1<<30)*Tt;
    // 		  }
    // 		  PrevTTT[i][gr] = TTT[i][gr];
    // 	        }
    // 	        else
    // 		{
    // 		  EvNoData[i]++;
    // 	        }
    // 	      }
    // // 	     OutputData_t outputData;
    // // // 	for(i=0; i<2; i++)
    // // // 	{ // get the wave regardless of any time stamp consideration
    // // // 	  int gr = 0;
    // 	     for(gr =0 ; gr < 4 ; gr++)
    // 	     {
    // 	       ApplyDataCorrection(gr, 7,Params.DRS4Frequency, &(Event742[i]->DataGroup[gr]), &Table[i]); //correct data every time. this was moved here for debugging
    // // 	       outputData.TTT[i][gr] = TTT[i][gr];
    // // 	       outputData.Trigger[i][gr] = Event742[i]->DataGroup[gr].DataChannel[8];
    // 	     }
    //
    // // 	     for(gr =0 ; gr < 4 ; gr++)
    // // 	     {
    // // // 	       fprintf(board[i],"%f\n",TTT[i][gr]);
    // // 	       int iSample = 0;
    // // 	       for( iSample = 0 ; iSample < 1024 ; iSample++)
    // // 	       {
    // // 	         fprintf(board[i],"%f ",Event742[i]->DataGroup[gr].DataChannel[8][iSample]);
    // // 	       }
    // // 	       fprintf(board[i],"\n");
    // // 	     }
    // // 	     int ch=0;
    // // 	     for(ch =0 ; ch < 32 ; ch++)
    // // 	     {
    // // 	       int iSample = 0;
    // // 	       for( iSample = 0 ; iSample < 1024 ; iSample++)
    // // 	       {
    // // 		 fprintf(board[i],"%f ",Event742[i]->DataGroup[ch/8].DataChannel[ch%8][iSample]);
    // // 	       }
    // // 	       fprintf(board[i],"\n");
    // // 	     }
    // // 	     fprintf(board[i],"\n");
    // //             printf("write[%d]=%d",i,write[i]);
    //             if (write[i]) {
    // 	    if (!ContinousWrite[i]) {
    // 	      write[i] = 0;
    // 	    }
    // 	    for(gr =0 ; gr < 4 ; gr++)
    // 	    {
    // 	      printf("TTT[%d][%d] = %zu\n",i,gr,Event742[i]->DataGroup[gr].TriggerTimeTag);
    // 	    }
    // 	    sprintf(name,"board%d.txt",i);
    // 	    if(!ContinousWrite[i])
    // 	      board[i] = fopen(name,"w");
    // 	    else
    // 	      board[i] = fopen(name,"a");
    // 	    int ch=0;
    // 	     int iSample = 0;
    // 	     for( iSample = 0 ; iSample < 1024 ; iSample++)
    // 	     {
    // 	       for(gr =0 ; gr < 4 ; gr++)
    // 	       {
    // 		 for(ch =0 ; ch < 9 ; ch++)
    // 		 {
    // 		   fprintf(board[i],"%f ",Event742[i]->DataGroup[gr].DataChannel[ch][iSample]);
    // 		 }
    // 	       }
    // 	       fprintf(board[i],"\n");
    // 	     }
    // 	     fclose(board[i]);
    //
    // // 	    if(plot[i]){
    // // 	    if (!ContinousPlot[i]) {
    // // 	      plot[i] = 0;
    // // 	    }
    // //
    // // 	    }
    // //
    //
    // // 	    fprintf(plotter, "set xlabel 'Samples' \n");
    // // 	    fprintf(plotter, "plot '%s' using ($0):($1) title 'Ch%d_Board0','event.txt' using ($0):($2) title 'TR%d_Board0','event.txt' using ($0):($3)  title 'Ch%d_Board1', 'event.txt' using ($0):($4) title 'TR%d_Board1'\n",name'
    // // 		    Params.RefChannel[0],(Params.RefChannel[0]/16),Params.RefChannel[1], (Params.RefChannel[1]/16));
    // // 	    fflush(plotter);
    // // 	    SLEEP(1);
    // 	  }
    //
    //
    //
    //
    // 	  }
    // 	}

    //////////////////////////


    //         printf("cycle N %d\n",cycle);
    //         for(i=0; i<2; i++) {
    // 	  if (GetNextEvent[i]) {
    //
    // 	    /* read a new data block from the board if there are no more events to use in the readout buffer */
    // 	    if (EIndx[i] >= NumEvents[i]) {
    // 	      EIndx[i] = 0;
    // 	      /* Read a data block from the board */
    // 	      ret = CAEN_DGTZ_ReadData(handle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer[i], &BufferSize[i]);
    // 	      printf("buffersize %d\n",BufferSize[i]);
    // 	      Nb += BufferSize[i];
    // 	      if (BufferSize[i]>0)
    // 		ret=ret;
    // 	      ret |= CAEN_DGTZ_GetNumEvents(handle[i], buffer[i], BufferSize[i], &NumEvents[i]);
    // 	      printf("NumEvents[%d] = %d\n",i,NumEvents[i]);
    // 	      if (ret) {
    // 		printf("Readout Error\n");
    // 		goto QuitProgram;
    // 	      }
    // 	    }
    // 	    /* Get one event from the readout buffer */
    // 	    if (NumEvents[i]) {
    // 	      printf("NumEvents[%d] = %d\n",i,NumEvents[i]);
    // 	      printf("EIndx[%d] = %d\n",i,EIndx[i]);
    // 	      ret = CAEN_DGTZ_GetEventInfo(handle[i], buffer[i], BufferSize[i], EIndx[i], &EventInfo[i], &EventPtr[i]);
    // 	      ret = CAEN_DGTZ_DecodeEvent(handle[i], EventPtr[i], (void**)&Event742[i]);
    // 	      TrgCnt[i]++;
    // 	      if (ret) {
    // 		printf("Event build error\n");
    // 		goto QuitProgram;
    // 	      }
    // 	      GetNextEvent[i]=0;
    // 	    }
    // 	  }
    // 	}
    //         if (GetNextEvent[0] || GetNextEvent[1])  // missing data from one or both boards
    //             continue;
    //
    //
    // 	// ----------------------------------------------------------------
    // //         // Analyze data
    // //         // ----------------------------------------------------------------
    // //         // calculate extended Trigger Time Tag (take roll over into account)
    // // 	    printf("Analyze data\n");
    // 	    for(i=0; i<2; i++) {
    // 	      int gr = 0;
    // 	      for(gr =0 ; gr < 4 ; gr++)
    // 	      {
    // 		if (Event742[i]->GrPresent[gr])
    // 		{
    // 		  TTT[i][gr] = ((Nroll[i]<<30) + (Event742[i]->DataGroup[gr].TriggerTimeTag & 0x3FFFFFFF))*Tt;
    // 		  if (TTT[i][gr] < PrevTTT[i][gr]) {
    // 		    Nroll[i]++;
    // 		    TTT[i][gr] += (1<<30)*Tt;
    // 		  }
    // 		  PrevTTT[i][gr] = TTT[i][gr];
    // 	        }
    // 	        else
    // 		{
    // 		  EvNoData[i]++;
    // 	        }
    // 	      }
    //
    // 	    }
    //
    //
    // 	OutputData_t outputData;
    // 	for(i=0; i<2; i++)
    // 	{ // get the wave regardless of any time stamp consideration
    // 	  int gr = 0;
    // 	  for(gr =0 ; gr < 4 ; gr++)
    // 	  {
    // 	    ApplyDataCorrection(gr, 7,Params.DRS4Frequency, &(Event742[i]->DataGroup[gr]), &Table[i]); //correct data every time. this was moved here for debugging
    // 	    outputData.TTT[i][gr] = TTT[i][gr];
    // 	    outputData.Trigger[i][gr] = Event742[i]->DataGroup[gr].DataChannel[8];
    // 	  }
    // 	  int ch = 0;
    // 	  for(ch =0 ; ch < 32 ; ch++)
    // 	  {
    // 	    outputData.Wave[i][ch] = Event742[i]->DataGroup[ch/8].DataChannel[ch];
    // 	  }
    //
    // 	  // 	  outputData.PulseEdgeTime[i] = -1; //set to -1 by default
    // 	  // 	  outputData.TrEdgeTime[i] = -1;    //set to -1 by default
    //
    // 	}
    //
    // 	GetNextEvent[0] =1;
    // 	GetNextEvent[1] =1;
    // 	EIndx[0]++; EIndx[1]++;
    //

    // 	fwrite(&outputData,sizeof(outputData),1,binOut);
  }
  error=0;

  QuitProgram:

  if (error)
  getch();

  /* *************************************************************************************** */
  /* FINAL CLEANUP                                                                           */
  /* *************************************************************************************** */
  for (i = 0; i < 2; i++) {
    /* stop the acquisition */
    CAEN_DGTZ_SWStopAcquisition(handle[i]);
    /* close the device and free the buffers */
    CAEN_DGTZ_FreeEvent(handle[i], (void**)&Event742);
    CAEN_DGTZ_FreeReadoutBuffer(&buffer[i]);
    /* close connection to boards */
    CAEN_DGTZ_CloseDigitizer(handle[i]);
  }
  // if (histoT)    free(histoT);
  /* close open files */
  if (log)       fclose(log);
  if (plotter)   pclose(plotter);

  fclose(binOut); //debug
  fclose(textOut);
  //            fclose(board[0]);
  //            fclose(board[1]);


  return 0;
}
