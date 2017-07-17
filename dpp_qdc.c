/******************************************************************************
*
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
******************************************************************************/
#include "dpp_qdc.h"

/* Globals */

extern int	gHandle; /* CAEN library handle */

/* Variable declarations */
unsigned int gActiveChannel;
unsigned int gEquippedChannels;
unsigned int gEquippedGroups;

long         gCurrTime;
long         gPrevTime;
long         gPrevWPlotTime;
long         gPrevHPlotTime;
long         gRunStartTime;
long         gRunElapsedTime;

uint32_t     gEvCnt[MAX_CHANNELS];
uint32_t     gEvCntOld[MAX_CHANNELS];
uint32_t**   gHisto;

uint64_t     gExtendedTimeTag[MAX_CHANNELS];
uint64_t     gETT[MAX_CHANNELS];
uint64_t     gPrevTimeTag[MAX_CHANNELS];

BoardParameters   gParams;
Stats        gAcqStats  ;

FILE *       gPlotDataFile;
FILE *       gListFiles[MAX_CHANNELS];

extern CAEN_DGTZ_BoardInfo_t             gBoardInfo;
_CAEN_DGTZ_DPP_QDC_Event_t*     gEvent[MAX_CHANNELS];
_CAEN_DGTZ_DPP_QDC_Waveforms_t* gWaveforms;

/* Global variables definition */
int          gSWTrigger       = 0;
int          grp4stats   = 0; /* Select group for statistics update on screen */
int          gLoops       = 0;
int          gToggleTrace = 0;
int          gAnalogTrace = 0;
int          gRestart     = 0;
int          gAcquisitionBufferAllocated = 0;

char *       gAcqBuffer      = NULL;
char *       gEventPtr      = NULL;
FILE *       gHistPlotFile       = NULL;
FILE *       gWavePlotFile       = NULL;

_CAEN_DGTZ_DPP_QDC_Event_t *gEventsGrp[8] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

#ifdef LINUX
    #include <sys/time.h> /* struct timeval, select() */
    #include <termios.h> /* tcgetattr(), tcsetattr() */
    #include <stdlib.h> /* atexit(), exit() */
    #include <unistd.h> /* read() */
    #include <stdio.h> /* printf() */
    #include <string.h> /* memcpy() */

    #define CLEARSCR "clear"

    static struct termios g_old_kbd_mode;

    /*****************************************************************************/
    static void cooked(void)
    {
    	tcsetattr(0, TCSANOW, &g_old_kbd_mode);
    }

    static void raw(void)
    {
    	static char init;
    /**/
    	struct termios new_kbd_mode;

    	if(init)
    		return;
    /* put keyboard (stdin, actually) in raw, unbuffered mode */
    	tcgetattr(0, &g_old_kbd_mode);
    	memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
    	new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
    	new_kbd_mode.c_cc[VTIME] = 0;
    	new_kbd_mode.c_cc[VMIN] = 1;
    	tcsetattr(0, TCSANOW, &new_kbd_mode);
    /* when we exit, go back to normal, "cooked" mode */
    	atexit(cooked);

    	init = 1;
    }

    /*****************************************************************************/
    /*  SLEEP  */
    /*****************************************************************************/
    void Sleep(int t) {
        usleep( t*1000 );
    }

    /*****************************************************************************/
    /*  GETCH  */
    /*****************************************************************************/
    // int getch(void)
    // {
    // 	unsigned char temp;
    //
    // 	raw();
    //     /* stdin = fd 0 */
    // 	if(read(0, &temp, 1) != 1)
    // 		return 0;
    // 	return temp;
    //
    // }


    /*****************************************************************************/
    /*  KBHIT  */
    /*****************************************************************************/
    // int kbhit()
    // {
    //
    // 	struct timeval timeout;
    // 	fd_set read_handles;
    // 	int status;
    //
    // 	raw();
    //     /* check stdin (fd 0) for activity */
    // 	FD_ZERO(&read_handles);
    // 	FD_SET(0, &read_handles);
    // 	timeout.tv_sec = timeout.tv_usec = 0;
    // 	status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
    // 	if(status < 0)
    // 	{
    // 		printf("select() failed in kbhit()\n");
    // 		exit(1);
    // 	}
    //     return (status);
    // }


#else  /* Windows */

    #include <conio.h>
    #define CLEARSCR "cls"

#endif

/* Get time in milliseconds from the computer internal clock */
// long get_time()
// {
//     long time_ms;
// #ifdef WIN32
//     struct _timeb timebuffer;
//     _ftime( &timebuffer );
//     time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
// #else
//     struct timeval t1;
//     struct timezone tz;
//     gettimeofday(&t1, &tz);
//     time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
// #endif
//     return time_ms;
// }

void clear_screen()
{
	system(CLEARSCR);
}

/*
** Set default parameters for the acquisition
*/
void set_default_parameters(BoardParameters *params) {
    int i;

	params->RecordLength      = 200;		   /* Number of samples in the acquisition window (waveform mode only)    */
	params->PreTrigger        = 100;  		   /* PreTrigger is in number of samples                                  */
	params->ActiveChannel     = 0;			   /* Channel used for the data analysis (plot, histograms, etc...)       */
	params->BaselineMode      = 2;			   /* Baseline: 0=Fixed, 1=4samples, 2=16 samples, 3=64samples            */
	params->FixedBaseline     = 2100;		   /* fixed baseline (used when BaselineMode = 0)                         */
	params->PreGate           = 20;			   /* Position of the gate respect to the trigger (num of samples before) */
    params->TrgHoldOff        = 10;            /* */
    params->TrgMode           = 0;             /* */
    params->TrgSmoothing      = 0;             /* */
    params->SaveList          = 0;             /*  Set flag to save list events to file */
	params->DCoffset[0]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->DCoffset[1]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->DCoffset[2]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->DCoffset[3]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->DCoffset[4]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->DCoffset[5]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->DCoffset[6]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->DCoffset[7]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
	params->ChargeSensitivity = 2;             /* Charge sesnitivity (0=max, 7=min)                    */
	params->NevAggr           = 1;             /* Number of events per aggregate (buffer). 0=automatic */
    params->AcqMode           = ACQMODE_LIST;  /* Acquisition Mode (LIST or MIXED)                     */
	params->PulsePol          = 1;             /* Pulse Polarity (1=negative, 0=positive)              */
    params->EnChargePed       = 0;             /* Enable Fixed Charge Pedestal in firmware (0=off, 1=on (1024 pedestal)) */
    params->DisTrigHist       = 0;             /* 0 = Trigger Histeresys on; 1 = Trigger Histeresys off */
    params->DefaultTriggerThr = 10;            /* Default threshold for trigger                        */

    for(i = 0; i < 8; ++i)
	  params->GateWidth[i]         = 40;	   /* Gate Width in samples                                */

    for(i = 0; i < 8; ++i)
	  params->DCoffset[i] = 0x8000;            /* DC offset adjust in DAC counts (0x8000 = mid scale)  */

    for(i = 0; i < 64; ++i) {
      params->TriggerThreshold[i] = params->DefaultTriggerThr;
    }

}

/*
** Read config file and assign new values to the parameters
** Returns 0 on success; -1 in case of any error detected.
*/
int load_configuration_from_file(char * fname, BoardParameters *params) {
    FILE *parameters_file;

    params->ConnectionType = CONNECTION_TYPE_AUTO;

	parameters_file = fopen(fname, "r");
	if (parameters_file != NULL) {
		while (!feof(parameters_file)) {
			char str[100];
			fscanf(parameters_file, "%s", str);
            if (str[0] == '#') {
                fgets(str, 100, parameters_file);
                continue;
            }

            if (strcmp(str, "AcquisitionMode") == 0) {
                char str1[100];
                fscanf(parameters_file, "%s", str1);
                if (strcmp(str1, "MIXED") == 0)
                    params->AcqMode = ACQMODE_MIXED;
                if (strcmp(str1, "LIST")  == 0)
                    params->AcqMode = ACQMODE_LIST;
            }

            if (strcmp(str, "ConnectionType") == 0) {
                char str1[100];
                fscanf(parameters_file, "%s", str1);
                if (strcmp(str1, "USB") == 0)
                 params->ConnectionType = CONNECTION_TYPE_USB;
                if (strcmp(str1, "OPT")  == 0)
                 params->ConnectionType = CONNECTION_TYPE_OPT;
            }


			if (strcmp(str, "ConnectionLinkNum") == 0)
             fscanf(parameters_file, "%d", &params->ConnectionLinkNum);
			if (strcmp(str, "ConnectionConetNode") == 0)
             fscanf(parameters_file, "%d", &params->ConnectionConetNode);
			if (strcmp(str, "ConnectionVmeBaseAddress") == 0)
				fscanf(parameters_file, "%llx", &params->ConnectionVMEBaseAddress);

			if (strcmp(str, "TriggerThreshold") == 0) {
				int ch;
				fscanf(parameters_file, "%d", &ch);
				fscanf(parameters_file, "%d", &params->TriggerThreshold[ch]);
			}

			if (strcmp(str, "RecordLength") == 0)
				fscanf(parameters_file, "%d", &params->RecordLength);
			if (strcmp(str, "PreTrigger") == 0)
				fscanf(parameters_file, "%d", &params->PreTrigger);
			if (strcmp(str, "ActiveChannel") == 0)
				fscanf(parameters_file, "%d", &params->ActiveChannel);
			if (strcmp(str, "BaselineMode") == 0)
				fscanf(parameters_file, "%d", &params->BaselineMode);
            if (strcmp(str, "TrgMode") == 0)
                fscanf(parameters_file, "%d", &params->TrgMode);
            if (strcmp(str, "TrgSmoothing") == 0)
                fscanf(parameters_file, "%d", &params->TrgSmoothing);
            if (strcmp(str, "TrgHoldOff") == 0)
                fscanf(parameters_file, "%d", &params->TrgHoldOff);
			if (strcmp(str, "FixedBaseline") == 0)
				fscanf(parameters_file, "%d", &params->FixedBaseline);
			if (strcmp(str, "PreGate") == 0)
				fscanf(parameters_file, "%d", &params->PreGate);
			if (strcmp(str, "GateWidth") == 0) {
                int gr;
				fscanf(parameters_file, "%d", &gr);
				fscanf(parameters_file, "%d", &params->GateWidth[gr]);
             }
			if (strcmp(str, "DCoffset") == 0) {
				int ch;
				fscanf(parameters_file, "%d", &ch);
				fscanf(parameters_file, "%d", &params->DCoffset[ch]);
			}
			if (strcmp(str, "ChargeSensitivity") == 0)
				fscanf(parameters_file, "%d", &params->ChargeSensitivity);
			if (strcmp(str, "NevAggr") == 0)
				fscanf(parameters_file, "%d", &params->NevAggr);
			if (strcmp(str, "SaveList") == 0)
				fscanf(parameters_file, "%d", &params->SaveList);
			if (strcmp(str, "ChannelTriggerMask") == 0)
				fscanf(parameters_file, "%llx", &params->ChannelTriggerMask);
			if (strcmp(str, "PulsePolarity") == 0)
				fscanf(parameters_file, "%d", &params->PulsePol);
			if (strcmp(str, "EnableChargePedestal") == 0)
				fscanf(parameters_file, "%d", &params->EnChargePed);
			if (strcmp(str, "DisableTriggerHysteresis") == 0)
				fscanf(parameters_file, "%d", &params->DisTrigHist);
			if (strcmp(str, "DisableSelfTrigger") == 0)
				fscanf(parameters_file, "%d", &params->DisSelfTrigger);
			if (strcmp(str, "EnableTestPulses") == 0)
				fscanf(parameters_file, "%d", &params->EnTestPulses);
			if (strcmp(str, "TestPulsesRate") == 0)
				fscanf(parameters_file, "%d", &params->TestPulsesRate);
			if (strcmp(str, "DefaultTriggerThr") == 0)
				fscanf(parameters_file, "%d", &params->DefaultTriggerThr);
			if (strcmp(str, "EnableExtendedTimeStamp") == 0)
				fscanf(parameters_file, "%d", &params->EnableExtendedTimeStamp);
		}
		fclose(parameters_file);


        return 0;
	}
    return -1;
}


/* Set parameters for the acquisition */
int setup_parameters(BoardParameters *params, char *fname) {
    int ret;
    set_default_parameters(params);

    ret = load_configuration_from_file(fname, params);
    return ret;
}

int configure_digitizer(int handle, int EquippedGroups, BoardParameters *params) {

    int ret = 0;
    int i;
    uint32_t DppCtrl1;
    uint32_t GroupMask = 0;

    /* Reset Digitizer */
	ret |= CAEN_DGTZ_Reset(handle);

    for(i=0; i<EquippedGroups; i++) {
        uint8_t mask = (params->ChannelTriggerMask>>(i*8)) & 0xFF;
        ret |= _CAEN_DGTZ_SetChannelGroupMask(handle, i, mask);
        if (mask)
            GroupMask |= (1<<i);
    }


    ret |= CAEN_DGTZ_SetGroupEnableMask(handle, GroupMask);


    /*
	** Set selfTrigger threshold
    ** Check if the module has 64 (VME) or 32 channels available (Desktop NIM)
    */
    if ((gBoardInfo.FormFactor == 0) || (gBoardInfo.FormFactor == 1))
	  for(i=0; i<64; i++)
		ret |= _CAEN_DGTZ_SetChannelTriggerThreshold(handle,i,params->TriggerThreshold[i]);
    else
	  for(i=0; i<32; i++)
		  ret |= _CAEN_DGTZ_SetChannelTriggerThreshold(handle,i,params->TriggerThreshold[i]);

	/* Disable Group self trigger for the acquisition (mask = 0) */
	ret |= CAEN_DGTZ_SetGroupSelfTrigger(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY,0x00);
	/* Set the behaviour when a SW tirgger arrives */
	ret |= CAEN_DGTZ_SetSWTriggerMode(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY);
	/* Set the max number of events/aggregates to transfer in a sigle readout */
	ret |= CAEN_DGTZ_SetMaxNumAggregatesBLT(handle, MAX_AGGR_NUM_PER_BLOCK_TRANSFER);
	/* Set the start/stop acquisition control */
    ret |= CAEN_DGTZ_SetAcquisitionMode(handle,CAEN_DGTZ_SW_CONTROLLED);

    /* Trigger Hold Off */
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8074, params->TrgHoldOff);

    /* DPP Control 1 register */
    DppCtrl1 = (((params->DisTrigHist & 0x1)      << 30)   |
                ((params->DisSelfTrigger & 1)     << 24)   |
                ((params->BaselineMode & 0x7)     << 20)   |
                ((params->TrgMode & 3)            << 18)   |
                ((params->ChargeSensitivity & 0x7)<<  0)   |
                ((params->PulsePol & 1)           << 16)   |
                ((params->EnChargePed & 1)        <<  8)   |
                ((params->TestPulsesRate & 3)     <<  5)   |
                ((params->EnTestPulses & 1)       <<  4)   |
                ((params->TrgSmoothing & 7)       << 12));

	ret |= CAEN_DGTZ_WriteRegister(handle, 0x8040, DppCtrl1);

    /* Set Pre Trigger (in samples) */
	ret |= CAEN_DGTZ_WriteRegister(handle, 0x803C, params->PreTrigger);

    /* Set Gate Offset (in samples) */
	ret |= CAEN_DGTZ_WriteRegister(handle, 0x8034, params->PreGate);

    /* Set Baseline (used in fixed baseline mode only) */
	ret |= CAEN_DGTZ_WriteRegister(handle, 0x8038, params->FixedBaseline);

    /* Set Gate Width (in samples) */
    for(i=0; i<EquippedGroups; i++) {
    	 ret |= CAEN_DGTZ_WriteRegister(handle, 0x1030 + 0x100*i, params->GateWidth[i]);
    }

    /* Set the waveform lenght (in samples) */
    ret |= _CAEN_DGTZ_SetRecordLength(handle,params->RecordLength);

    /* Set DC offset */
	for (i=0; i<4; i++)
		ret |= CAEN_DGTZ_SetGroupDCOffset(handle, i, params->DCoffset[i]);

    /* enable Charge mode */
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00080000);
    /* enable Timestamp */
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00040000);

    /* set Scope mode */
	if (params->AcqMode == ACQMODE_MIXED)
		ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00010000);
	else
        ret |= CAEN_DGTZ_WriteRegister(handle, 0x8008, 0x00010000);

     /* Set number of events per memory buffer */
     _CAEN_DGTZ_DPP_QDC_SetNumEvAggregate(handle, params->NevAggr);

	/* enable test pulses on TRGOUT/GPO */
	if (ENABLE_TEST_PULSE) {
		uint32_t d32;
		ret |= CAEN_DGTZ_ReadRegister(handle, 0x811C, &d32);
		ret |= CAEN_DGTZ_WriteRegister(handle, 0x811C, d32 | (1<<15));
		ret |= CAEN_DGTZ_WriteRegister(handle, 0x8168, 2);
	}

    /* Set Extended Time Stamp if enabled*/
    if (params->EnableExtendedTimeStamp)
       ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 1 << 17);

    /* Check errors */
	if(ret != CAEN_DGTZ_Success) {
        printf("Errors during Digitizer Configuration.\n");
       return -1;
    }

    return 0;
}

/*
** Read data from board and decode events.
** The function has a number of side effects since it relies
** on global variables.
** Actions performed by this function are:
**  - send a software trigger (if enabled)
**  - read available data from board
**  - get events fromm acquisition buffer
**  - updates energy histogram
**  - plot histogram every second
**  - saves list files (if enabled)
**  - gets waveforms from events and plots them
**  - saves data to disk
**
** If software triggers are enabled, it generates a trigger prior to
** reading board data.
*/
int run_acquisition() {
//         printf("run\n");
        int ret;
        unsigned int i;
        unsigned int j;
        uint32_t     bin;
        uint32_t     bsize;
        uint32_t     NumEvents[MAX_CHANNELS];

        /* Send a SW Trigger if requested by user */
		if (gSWTrigger) {
            if (gSWTrigger == 1)    gSWTrigger = 0;
            CAEN_DGTZ_SendSWtrigger(gHandle);
        }

        memset(NumEvents, 0, MAX_CHANNELS*sizeof(NumEvents[0]));

	/* Read a block of data from the digitizer */
	ret = CAEN_DGTZ_ReadData(gHandle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, gAcqBuffer, &bsize); /* Read the buffer from the digitizer */
// 	printf("bsize %d\n",bsize);
	gLoops += 1;
	if (ret) {
	  printf("Readout Error\n");
	  return -1;
	}
	if (bsize == 0)
	  return 0;
//         printf("bsize %d\n",bsize);
        /* Decode and analyze Events */
        if ( (ret = _CAEN_DGTZ_GetDPPEvents(gHandle, gAcqBuffer, bsize, (void **)gEvent, NumEvents)) != CAEN_DGTZ_Success)
        {
          printf("Error during _CAEN_DGTZ_GetDPPEvents() call (ret = %d): exiting ....\n", ret);
          exit(-1);
        }

        /* Loop over all channels and events */
	for (i=0; i < gEquippedChannels; ++i) {
	  for(j=0; j<NumEvents[i]; ++j) {
	    uint32_t Charge;
	    
	    Charge = (gEvent[i][j].Charge & 0xFFFF);  /* rebin charge to 4Kchannels */
	    
	    
	    /* Update energy histogram */
	    if ((Charge < HISTO_NBIN) && (Charge >= CHARGE_LLD_CUT) && (Charge <= CHARGE_ULD_CUT) && (gEvent[i][j].Overrange == 0))
	      gHisto[i][Charge]++;
	    
	    /* Plot Histogram */
	    gCurrTime = get_time();
	    if ((gCurrTime-gPrevHPlotTime) > 1000) {
	      gPlotDataFile = fopen("PlotData.txt", "w");
	      for(bin=0; bin<HISTO_NBIN; bin++)
		fprintf(gPlotDataFile, "%d\n", gHisto[gActiveChannel][bin]);
	      fclose(gPlotDataFile);
	      fprintf(gHistPlotFile, "plot 'PlotData.txt' with step\n");
	      fflush(gHistPlotFile);
	      gPrevHPlotTime = gCurrTime;
	    }
	    
	    /* Check roll over of time tag */
	    if (gEvent[i][j].TimeTag < gPrevTimeTag[i])
	      gETT[i]++;
	    
	    gExtendedTimeTag[i] = (gETT[i] << 32) + (uint64_t)(gEvent[i][j].TimeTag);
	    
	    
	    /* Save event to output file */
	    if (gParams.SaveList) {
	      fprintf(gListFiles[i], "%16llu %8d\n", gExtendedTimeTag[i], gEvent[i][j].Charge);
	    }
	    
	    /* Plot Waveforms (if enabled) */
	    if ((i==gActiveChannel) && (gParams.AcqMode == ACQMODE_MIXED) && ((gCurrTime-gPrevWPlotTime) > 300) && (Charge >= CHARGE_LLD_CUT) && (Charge <= CHARGE_ULD_CUT)) {
	      _CAEN_DGTZ_DecodeDPPWaveforms(&gEvent[i][j], gWaveforms);
	      gPlotDataFile = fopen("PlotWave.txt", "w");
	      printf("gWaveforms = %d\n",gWaveforms->Ns);
	      for(j=0; j<gWaveforms->Ns; j++) {
		fprintf(gPlotDataFile, "%d ", gWaveforms->Trace1[j]);                 /* samples */
		fprintf(gPlotDataFile, "%d ", 2000 + 200 *  gWaveforms->DTrace1[j]);  /* gate    */
		fprintf(gPlotDataFile, "%d ", 1000 + 200 *  gWaveforms->DTrace2[j]);  /* trigger */
		fprintf(gPlotDataFile, "%d ", 500 + 200 *  gWaveforms->DTrace3[j]);   /* trg hold off */
		fprintf(gPlotDataFile, "%d\n", 100 + 200 *  gWaveforms->DTrace4[j]);  /* overthreshold */
	      }
	      fclose(gPlotDataFile);
	      
	      
	      switch (gAnalogTrace) {
		case 0:
		  fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Input' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
		  break;
		case 1:
		  fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Smooth' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
		  break;
		case 2:
		  fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Baseline' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
		  break;
		default:
		  fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Input' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
		  break;
	      }
	      
	      fflush(gWavePlotFile);
	      gPrevWPlotTime = gCurrTime;
	    }
	    
	    
	    gPrevTimeTag[i]   = gEvent[i][j].TimeTag;
	  }
	  
	  
	  gEvCnt[i]      += NumEvents[i];
	  if (gToggleTrace) {
	    
	    switch (gAnalogTrace) {
	      case 0:
		CAEN_DGTZ_WriteRegister(gHandle, 0x8008, 3<<12);
		break;
	      case 1:
		CAEN_DGTZ_WriteRegister(gHandle, 0x8004, 1<<12);
		CAEN_DGTZ_WriteRegister(gHandle, 0x8008, 1<<13);
		break;
	      case 2:
		CAEN_DGTZ_WriteRegister(gHandle, 0x8008, 1<<12);
		CAEN_DGTZ_WriteRegister(gHandle, 0x8004, 1<<13);
		break;
	      default:
		CAEN_DGTZ_WriteRegister(gHandle, 0x8004, 3<<12);
		break;
	    }
	    
	    
	    gToggleTrace = 0;
	  }
	  
	}
	
	gAcqStats.nb += bsize;
	return 0;

}


/*
** Prints statistics to console.
*/
void print_statistics() {
  long elapsed;
  unsigned int i;
  uint64_t     PrevEvCnt   = 0;
  
  gCurrTime = get_time();
  elapsed = (gCurrTime-gPrevTime);
  gAcqStats.gRunElapsedTime = gCurrTime - gRunStartTime;
  if (elapsed>1000) {
    uint64_t diff;
    gAcqStats.TotEvCnt = 0;
    clear_screen();
    
    for(i=0; i < gEquippedChannels; ++i) {
      gAcqStats.TotEvCnt += gEvCnt[i];
    }
    
    printf("*********** Global statistics ***************\n");
    printf("=============================================\n\n");
    printf("Elapsed time         = %d ms\n", gAcqStats.gRunElapsedTime);
    printf("Readout Loops        = %d\n", gLoops );
    printf("Bytes read           = %d\n", gAcqStats.nb);
    printf("Events               = %lld\n", gAcqStats.TotEvCnt);
    printf("Readout Rate         = %.2f MB/s\n\n", (float)gAcqStats.nb / 1024 / (elapsed));
    
    
    printf("******** Group  %d statistics **************\n\n", grp4stats);
    printf("-- Channel -- Event Rate (KHz) -----\n\n");
    
    for(i=0; i < 8; ++i) {
      diff = gEvCnt[grp4stats*8+i]-gEvCntOld[grp4stats*8+i];
      printf("    %2d      %.2f\n", grp4stats*8+i, (float)(diff) / (elapsed));
      gEvCntOld[grp4stats*8+i] = gEvCnt[grp4stats*8+i];
    }
    
    gAcqStats.nb        = 0;
    gPrevTime  = gCurrTime;
    PrevEvCnt = gAcqStats.TotEvCnt;
    
  }
  
}


/*
** Cleanup heap memory
** Returns 0 on success; -1 in case of any error detected.
*/
int cleanup_on_exit() {

    int ret;
    int i;

    /* Free the buffers and close the digitizer */
    CAEN_DGTZ_SWStopAcquisition(gHandle);
    ret = _CAEN_DGTZ_FreeReadoutBuffer(&gAcqBuffer);

    if (gHistPlotFile != NULL)
      pclose(gHistPlotFile);
    if (gWavePlotFile != NULL)
      pclose(gWavePlotFile);

    if(gWaveforms) free(gWaveforms);

    for(i=0; i<MAX_CHANNELS; ++i) {
	  if (gListFiles[i] != NULL)
	  	fclose(gListFiles[i]);
    }

    for(i=0; i<MAX_CHANNELS; ++i) {
        if (gEvent[i] != NULL)
		  free(gEvent[i]);
      }

	/* deallocate memory for group events */
	for(i=0; i<8; i++) {
		if (gEventsGrp[i] != NULL)
			free(gEventsGrp[i]);
	}


    ret = CAEN_DGTZ_CloseDigitizer(gHandle);

	return 0;

}

/*
** Setup acquisition.
**   - setup board according to configuration file
**   - open a connection with the module
**   - allocates memory for acquisition buffers and structures
** Returns 0 on success; -1 in case of any error detected.
*/

int setup_acquisition(char *fname) {

    int ret;
    unsigned int i;
    uint32_t     size = 0;
    uint32_t AllocatedSize = 0;
    uint32_t rom_version;
    uint32_t rom_board_id;

    clear_screen();

    /* ---------------------------------------------------------------------------------------
    ** Set Parameters (default values or read from config file)
    ** ---------------------------------------------------------------------------------------
    */
    ret = setup_parameters(&gParams, fname);
    if (ret < 0) {
        printf("Error in setting parameters\n");
        return -1;
    }

    /* ---------------------------------------------------------------------------------------
    // Open the digitizer and read board information
    // ---------------------------------------------------------------------------------------
    */

    CAEN_DGTZ_CloseDigitizer(gHandle);

    /* Open a conection with module and gets library handle */
    if (gParams.ConnectionType == CONNECTION_TYPE_AUTO) {
       /* Try USB connection first */
       ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
       if(ret != CAEN_DGTZ_Success) {
          /* .. if not successful try opticallink connection then ...*/
          ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_OpticalLink, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
          if(ret != CAEN_DGTZ_Success) {
            printf("Can't open digitizer\n");
            return -1;
          }
       }
    }
    else {
      if (gParams.ConnectionType == CONNECTION_TYPE_USB)
       ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
      if (gParams.ConnectionType == CONNECTION_TYPE_OPT)
       ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_OpticalLink, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
      if(ret != CAEN_DGTZ_Success) {
            printf("Can't open digitizer\n");
            return -1;
          }
    }

    /* Check board type */
    ret = CAEN_DGTZ_GetInfo(gHandle, &gBoardInfo);

    /* Patch to handle ModelName which is not decoded by CAENDigitizer library <= 2.6.7 */
    CAEN_DGTZ_ReadRegister(gHandle, 0xF030, &rom_version);
    if((rom_version & 0xFF) == 0x54)
    {
      CAEN_DGTZ_ReadRegister(gHandle, 0xF034, &rom_board_id);
      switch (rom_board_id)
      {
         case 0:
           sprintf(gBoardInfo.ModelName, "V1740D");
           break;
         case 1:
           sprintf(gBoardInfo.ModelName, "VX1740D");
           break;
         case 2:
           sprintf(gBoardInfo.ModelName,"DT5740D");
           break;
         case 3:
           sprintf(gBoardInfo.ModelName,"N6740D");
           break;
         default:
           break;
      }
    }
    else
    {
      printf("This software is not appropriate for module %s\n", gBoardInfo.ModelName);
      // printf("Press any key to exit!\n");
      // getch();
      exit(0);
    }


    printf("\nConnected to CAEN Digitizer Model %s\n", gBoardInfo.ModelName);
    printf("ROC FPGA Release is %s\n", gBoardInfo.ROC_FirmwareRel);
    printf("AMC FPGA Release is %s\n", gBoardInfo.AMC_FirmwareRel);    
    printf("VME Handle is %s\n", gBoardInfo.VMEHandle);
    printf("Serial Number is %d\n\n", gBoardInfo.SerialNumber);

    gEquippedChannels = gBoardInfo.Channels * 8;
    gEquippedGroups = gEquippedChannels/8;

    gActiveChannel = gParams.ActiveChannel;
    if (gActiveChannel >= gEquippedChannels)
		gActiveChannel = gEquippedChannels-1;

    ret = CAEN_DGTZ_SWStopAcquisition(gHandle);

    ret = configure_digitizer(gHandle, gEquippedGroups, &gParams);
    if (ret)  {
        printf("Error during digitizer configuration\n");
       return -1;
    }

    /* ---------------------------------------------------------------------------------------
    // Memory allocation and Initialization of counters, histograms, etc...
    */
    memset(gExtendedTimeTag, 0, MAX_CHANNELS*sizeof(uint64_t));
    memset(gETT, 0, MAX_CHANNELS*sizeof(uint64_t));
    memset(gPrevTimeTag, 0, MAX_CHANNELS*sizeof(uint64_t));
    memset(gPrevTimeTag, 0, MAX_CHANNELS*sizeof(uint64_t));
    memset(gEvCnt, 0, MAX_CHANNELS*sizeof(gEvCnt[0]));
    memset(gEvCntOld, 0, MAX_CHANNELS*sizeof(gEvCntOld[0]));

	/* Malloc Readout Buffer.
    NOTE: The mallocs must be done AFTER digitizer's configuration! */

    if (!gAcquisitionBufferAllocated) {
      gAcquisitionBufferAllocated = 1;
      ret = _CAEN_DGTZ_MallocReadoutBuffer(gHandle,&gAcqBuffer,&size);
      if (ret) {printf("Cannot allocate %d bytes for the acquisition buffer! Exiting...", size);exit(-1);}
      printf("Allocated %d bytes for readout buffer \n", size);

      /* NOTE : allocates a fixed size.
	  ** Needs CAENDigitizer library implementation.
	  */
      for (i= 0; i < gEquippedChannels; ++i) {
        int allocated_size = MAX_AGGR_NUM_PER_BLOCK_TRANSFER * MAX_EVENT_QUEUE_DEPTH * sizeof(_CAEN_DGTZ_DPP_QDC_Event_t);
	    gEvent[i] = (_CAEN_DGTZ_DPP_QDC_Event_t *)malloc(allocated_size);

        if (gEvent[i] == NULL)
           {printf("Cannot allocate memory for event list for channel %d\n. Exiting....", i); exit(-1);};
      }

      /* allocate memory buffer for the data readout */
      ret = _CAEN_DGTZ_MallocDPPWaveforms(gHandle, &gWaveforms, &AllocatedSize);

      /* Allocate memory for the histograms */
      gHisto = (uint32_t **)malloc(gEquippedChannels * sizeof(uint32_t *));
      if(gHisto == NULL) {printf("Cannot allocate %d bytes for histogram! Exiting...", gEquippedChannels * sizeof(uint32_t *));exit(-1);}
      for(i= 0 ; i < gEquippedChannels; ++i) {
	    gHisto[i] = (uint32_t *)malloc(HISTO_NBIN * sizeof(uint32_t));
        if(gHisto[i] == NULL) {printf("Cannot allocate %d bytes for histogram! Exiting...", HISTO_NBIN * sizeof(uint32_t));exit(-1);}
      }

	  /* open gnuplot in a pipe and the data file*/
//       gHistPlotFile = popen("pgnuplot.exe", "w");
         gHistPlotFile = popen("gnuplot", "w");
      if (gHistPlotFile==NULL) {
          printf("Can't open gnuplot\n");
          return -1;
      }
      if (gParams.AcqMode == ACQMODE_MIXED) {
//           gWavePlotFile = popen("pgnuplot.exe", "w");
          gWavePlotFile = popen("gnuplot", "w");
          fprintf(gWavePlotFile, "set yrange [0:4096]\n");
      }

    }



    for(i= 0 ; i < gEquippedChannels; ++i) {
     memset(gHisto[i], 0, HISTO_NBIN * sizeof(uint32_t));
     gEvCnt[i] = 0;
    }

    /* Crete output list files (one per channel) */
    for(i=0; i < gEquippedChannels; ++i) {
          char filename[200];
          sprintf(filename, "list_ch%d.txt", i);
	      if (gListFiles[i] == NULL) gListFiles[i] = fopen(filename, "w");
       }

	//printf("\nPress a key to start the acquisition\n");
	// getch();
    // ret = CAEN_DGTZ_SWStartAcquisition(gHandle);
	// printf("Acquisition started\n");

    return 0;
}

/*
** Checks if a key has been pressed and sets global variables accordingly.
** Returns 0 on success; -1 in case of any error detected or when exit key is pressed.
*/
// int check_user_input() {
//   int c;
//   unsigned int i;
//   
//   /* check if any key has been pressed */
//   if (kbhit()) {
//     c = getch();
//     if (c=='q') return -1;
//     if (c=='t') gSWTrigger = 1;
//     if (c=='0') {gAnalogTrace = 0; gToggleTrace = 1;}
//     if (c=='1') {gAnalogTrace = 1; gToggleTrace = 1;}
//     if (c=='2') {gAnalogTrace = 2; gToggleTrace = 1;}
//     if (c=='l') gParams.SaveList = 1;
//     if (c=='T') gSWTrigger = gSWTrigger ^ 2;
//     if (c=='r') {
//       for(i= 0 ; i < gEquippedChannels; ++i) {
// 	memset(gHisto[i], 0, HISTO_NBIN * sizeof(uint32_t));
// 	gEvCnt[i] = 0;
//       }
//       gAcqStats.nb = 0;
//       gLoops = 0;
//     }
//     if (c=='R') {
//       gRestart = 1;
//     }
//     
//     if (c=='c') {
//       printf("Channel = ");
//       scanf("%d", &gActiveChannel);
//     }
//     if (c=='g') {
//       printf("Group = ");
//       scanf("%d", &grp4stats);
//     }
//   } /*  if (kbhit()) */
//   
//   return 0;
// }
