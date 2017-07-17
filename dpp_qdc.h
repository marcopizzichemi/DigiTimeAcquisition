/******************************************************************************
*
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
******************************************************************************/

#ifndef _DPP_QDC_H
#define _DPP_QDC_H

/* System library includes */
#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
// #include <process.h>
#include <errno.h>

#include "CAENDigitizer.h"
#include "_CAENDigitizer_DPP-QDC.h"

/* Macro definition */
// #define popen  _popen    /* redefine POSIX 'deprecated' popen as _popen */
// #define pclose _pclose   /* redefine POSIX 'deprecated' pclose as _pclose */

#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED

#define ACQMODE_LIST    0
#define ACQMODE_MIXED   1

#define CONNECTION_TYPE_USB    0
#define CONNECTION_TYPE_OPT    1
#define CONNECTION_TYPE_AUTO   255

#define HISTO_NBIN    4096

#define MAX_AGGR_NUM_PER_BLOCK_TRANSFER   1023 /* MAX 1023 */


#define ENABLE_TEST_PULSE 1

// #define CONFIG_FILE_NAME "config.txt"

/* Charge cuts */
#define CHARGE_LLD_CUT 0
#define CHARGE_ULD_CUT (HISTO_NBIN-1)

/* Data structures */

typedef struct
{
    uint32_t ConnectionType;
    uint32_t ConnectionLinkNum;
    uint32_t ConnectionConetNode;
    uint32_t ConnectionVMEBaseAddress;
	uint32_t RecordLength;
	uint32_t PreTrigger;
	uint32_t ActiveChannel;
	uint32_t GateWidth[8];
	uint32_t PreGate;
	uint32_t ChargeSensitivity;
	uint32_t FixedBaseline;
	uint32_t BaselineMode;
    uint32_t TrgMode;
    uint32_t TrgSmoothing;
    uint32_t TrgHoldOff;
    uint32_t TriggerThreshold[64];
	uint32_t DCoffset[8];
	uint32_t AcqMode;
	uint32_t NevAggr;
    uint32_t PulsePol;
    uint32_t EnChargePed;
    uint32_t SaveList;
    uint32_t DisTrigHist;
    uint32_t DisSelfTrigger;
    uint32_t EnTestPulses;
    uint32_t TestPulsesRate;
    uint32_t DefaultTriggerThr;
    uint32_t EnableExtendedTimeStamp;
	uint64_t ChannelTriggerMask;
} BoardParameters;

/* Data structures */
typedef struct
{
  long gRunElapsedTime;                                        /* Acquisition time (elapsed since start) */
  uint64_t TotEvCnt;                                           /* Total number of events                 */
  unsigned int nb;                                             /* Number of bytes read                   */
} Stats;

/* Globals */
extern int	        gHandle;                                   /* CAEN library handle */

/* Variable declarations */
extern unsigned int gActiveChannel;                            /* Active channel for data analysis */
extern unsigned int gEquippedChannels;                         /* Number of equipped channels      */
extern unsigned int gEquippedGroups;                           /* Number of equipped groups        */

extern long         gCurrTime;                                 /* Current time                     */
extern long         gPrevTime;                                 /* Previous saved time              */
extern long         gPrevWPlotTime;                            /* Previous time of Waveform plot   */
extern long         gPrevHPlotTime;                            /* Previous time of Histogram plot  */
extern long         gRunStartTime;                             /* Time of run start                */
extern long         gRunElapsedTime;                           /* Elapsed time                     */

extern uint32_t     gEvCnt[MAX_CHANNELS];                      /* Event counters per channel       */
extern uint32_t     gEvCntOld[MAX_CHANNELS];                   /* Event counters per channel (old) */
extern uint32_t**   gHisto;                                    /* Histograms                       */

extern uint64_t     gExtendedTimeTag[MAX_CHANNELS];            /* Extended Time Tag                */
extern uint64_t     gETT[MAX_CHANNELS];                        /* Extended Time Tag                */
extern uint64_t     gPrevTimeTag[MAX_CHANNELS];                /* Previous Time Tag                */

extern BoardParameters   gParams;                              /* Board parameters structure       */
extern Stats        gAcqStats;                                 /* Acquisition statistics           */

extern FILE *       gPlotDataFile;                             /* Target file for plotting data    */
extern FILE *       gListFiles[MAX_CHANNELS];                  /* Output file for list data        */

extern CAEN_DGTZ_BoardInfo_t             gBoardInfo;           /* Board Informations               */
extern _CAEN_DGTZ_DPP_QDC_Event_t*     gEvent[MAX_CHANNELS]; /* Events                           */
extern _CAEN_DGTZ_DPP_QDC_Waveforms_t* gWaveforms;           /* Waveforms                        */

/* Variable definitions */
extern int          gSWTrigger;                                /* Signal for software trigger          */
extern int          grp4stats;                                 /* Selected group for statistics        */
extern int          gLoops;                                    /* Number of acquisition loops          */
extern int          gToggleTrace;                              /* Signal for trace toggle from user    */
extern int          gAnalogTrace;                              /* Signal for analog trace toggle       */
extern int          gRestart    ;                              /* Signal acquisition restart from user */
extern char *       gAcqBuffer;                                /* Acquisition buffer                   */
extern char *       gEventPtr;                                 /* Events buffer pointer                */
extern FILE *       gHistPlotFile;                             /* Source file for histogram plot       */
extern FILE *       gWavePlotFile;                             /* Source file for waveform plot        */


/* *************************************************************************************************
** Function prototypes
****************************************************************************************************/

/* readout_demo functions prototypes */
extern long get_time();
int  setup_acquisition(char* fname);
int  run_acquisition();
void print_statistics();
int  check_user_input();
int  cleanup_on_exit();

/* Board parameters utility functions */
void set_default_parameters(BoardParameters *params);
int  load_configuration_from_file(char * fname, BoardParameters *params);
int  setup_parameters(BoardParameters *params, char *fname);
int  configure_digitizer(int handle, int gEquippedGroups, BoardParameters *params);

/* Utility functions prototypes */
// long get_time();
void clear_screen( void );

#ifdef LINUX
    #include <sys/time.h> /* struct timeval, select() */
    #include <termios.h> /* tcgetattr(), tcsetattr() */
    #include <stdlib.h> /* atexit(), exit() */
    #include <unistd.h> /* read() */
    #include <stdio.h> /* printf() */
    #include <string.h> /* memcpy() */

	#define CLEARSCR "clear"

/*****************************************************************************/
/*  SLEEP  */
/*****************************************************************************/
void Sleep(int t);

/*****************************************************************************/
/*  GETCH  */
/*****************************************************************************/
// int getch(void);

/*****************************************************************************/
/*  KBHIT  */
/*****************************************************************************/
// int kbhit();


#else  /* Windows */

    #include <conio.h>
	#define getch _getch
	#define kbhit _kbhit

	#define CLEARSCR "cls"
#endif

#endif
