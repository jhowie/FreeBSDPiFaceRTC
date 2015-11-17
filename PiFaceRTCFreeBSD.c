/*
**  PiFaceRTCFreeBSD.c
**
**  Created by John Howie on 10/27/15.
**
**      j.howie@napier.ac.uk
**      john@thehowies.com
**
**  This source file contains the  PiFace Real Time Clock application.
**
** Modifications
**
** 20151027 jh  Original.
**
** Copyright (c) 2015, jhowie
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** * Redistributions of source code must retain the above copyright notice, this
**   list of conditions and the following disclaimer.
**
** * Redistributions in binary form must reproduce the above copyright notice,
**   this list of conditions and the following disclaimer in the documentation
**   and/or other materials provided with the distribution.
** 
** * Neither the name of FreeBSDPiFaceRTC nor the names of its
**   contributors may be used to endorse or promote products derived from
**   this software without specific prior written permission.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
*/

# include <stdbool.h>
# include <sys/cdefs.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <fcntl.h>
# include <sys/ioctl.h>
# include <errno.h>
# include <unistd.h>
# include <sys/utsname.h>
# include <sys/types.h>
# include <sys/time.h>
# include <time.h>
# include <ctype.h>

# include "PiFaceRTC.h"
# include "I2CRoutines.h"

/*
** Funtion prototypes
*/

void Usage (void);
int DisplayPowerFailTime (int busfd, int nBusDevId);
int DisplayPowerRestoreTime (int busfd, int nBusDevId);
int HWSetTimeOfDay (int busfd, int nBusDevId, char *szDatetime, bool bUseComputerClockToSetRTC);
int HWGetTimeOfDay (int busfd, int nBusDevId, bool bDisplayDateTimeAsDateInput, bool bSetComputerClockFromRTC);
int ReadNVRAM (int busfd, int nBusDevId);
int WriteNVRAM (int busfd, int nBusDevId, char *szNVRAMContents);
int ProcessHWClockOption (int busfd, int nBusDevId, char *szOptionsToProcess);

int HWOptionInitRTC (int busfd, int nBusDevId);
int HWOptionBatteryEnable (int busfd, int nBusDevId);
int HWOptionBatteryDisable (int busfd, int nBusDevId);
int HWOptionBatteryGetSetting (int busfd, int nBusDevId);
int HWOptionControlRegistersDisplay (int busfd, int nBusDevId);
int HWOptionCalibrateClock (int busfd, int nBusDevId);
int HWOptionClearNVRAM (int busfd, int nBusDevId);
int HWOptionOscillatorEnable (int busfd, int nBusDevId);
int HWOptionOscillatorDisable (int busfd, int nBusDevId);
int HWOptionOscillatorGetStatus (int busfd, int nBusDevId);
int HWOptionOscillatorGetSetting (int busfd, int nBusDevId);
int HWOptionPowerFailStatus (int busfd, int nBusDevId);
int HWOptionPowerFailClearFlag (int busfd, int nBusDevId);

int HWOptionOscillatorConfigure (int busfd, int nBusDevId, bool bEnable);
int HWOptionBatteryConfigure (int busfd, int nBusDevId, bool bEnable);

void TranslateRTCDateTimeToTm (struct mcp7940n_datetime *pdatetimeRTCClock, struct tm *ptmRTCDateTime);
void TranslateTmToRTCDateTime (struct tm *ptmRTCDateTime, struct mcp7940n_datetime *pdatetimeRTCClock);

struct rtc_option {
    char *m_szOption;
    int (*m_pOptionFunc)();
} RTCOptions [] = {
    { "init", &HWOptionInitRTC },
    { "bat", &HWOptionBatteryEnable },
    { "nobat", &HWOptionBatteryDisable },
    { "batset", &HWOptionBatteryGetSetting },
    { "cal", &HWOptionCalibrateClock },
    { "clrnvram", &HWOptionClearNVRAM },
    { "control", &HWOptionControlRegistersDisplay },
    { "osc", &HWOptionOscillatorEnable },
    { "noosc", &HWOptionOscillatorDisable },
    { "oscset", &HWOptionOscillatorGetSetting },
    { "oscstat", &HWOptionOscillatorGetStatus },
    { "pwrstat", &HWOptionPowerFailStatus },
    { "clrpwr", &HWOptionPowerFailClearFlag },
    { 0, 0 }
};

char *szDisplayWeekday [] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char *szDisplayMonth [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
 
/* int main (int argc, char **argv)
**
** This is the entry point for the application
*/

int main (int argc, char **argv)
{
    int ch, nBusDevId = 0x6f, busfd;     // The PiFace RTC bus device id is 0x69 in 7-bit addressing
    char *szBusName = (char *) 0, *szOptions = (char *) 0, *szNVRAMContents = (char *) 0;
    bool bUseComputerClockToSetRTC = false, bSetComputerClockFromRTC = false,
            bDisplayPowerFail = false, bDisplayPowerRestore = false,
            bProcessOptions = false, bDisplayDateTimeAsDateInput = false,
            bReadNVRAM = false, bWriteNVRAM = false;

    // Go through the command line arguments
    
    while ((ch = getopt (argc, argv, "b:cdhi:o:prsuw:")) != -1) {
        switch (ch) {
        case 'b':
            // The user wants to set the device id on the bus
            
            nBusDevId = strtol (optarg, (char **) 0, 0);
            if (nBusDevId >= 0x80) {
                // The bus devid needs to be a 7-bit address for our purposes
                
                Usage ();
                exit (1);
            }
            break;
            
        case 'c':
            // The user wants us to use the clock of the computer to set the RTC
            
            bUseComputerClockToSetRTC = true;
                
        case 'd':
            // The user wants us to display the date in a format suitable as input to the date command
                
            bDisplayDateTimeAsDateInput = true;
            break;
                
        case 'h':
            // User wants to display some help
                
            Usage ();
            exit (0);
            break;
                
        case 'i':
            // The user is specifying the bus. The single optarg character tells us which bus we are using
                
            switch (*optarg) {
                case '0': szBusName = "/dev/iic0"; break;
                case '1': szBusName = "/dev/iic1"; break;
                default: Usage (); exit (1); break;
            }
            break;
            
        case 'o':
            // The user wants to process an option
                
            bProcessOptions = true;
            szOptions = optarg;
            break;
                
        case 'p':
            // The user wants the powerfail time
                
            bDisplayPowerFail = true;
            break;
                
        case 'r':
            // The user wants to display the NVRAM contents
                
            bReadNVRAM = true;
            break;
            
        case 's':
            // The user wants to set the computer clock from the RTC
            
            bSetComputerClockFromRTC = true;
            break;    
                
        case 'u':
            // The user wants the time the power was restored at
                
            bDisplayPowerRestore = true;
            break;
                
        case 'w':
            // The user wants to write to the NVRAM
                
            szNVRAMContents = optarg;
            bWriteNVRAM = true;
            break;
                
        default:
            // We received an unknown command line switch
                
            Usage ();
            exit (1);
            break;
        }
    }
    
    // Set up argv and argv to process the remainder of the command line
    
    argc -= optind;
    argv += optind;
    
    // Open the bus device
    
    busfd = OpenI2CDevice (szBusName, nBusDevId);
    if (busfd < 0) {
        // An error occurred, and we were unable to open the I2C bus device
        
        exit (1);
    }
    
    // If the user wanted the powerfail or powerrestore date/time, get it
    
    if (bDisplayPowerFail) {
        if (DisplayPowerFailTime (busfd, nBusDevId) < 0) {
            // An error occurred
            
            exit (1);
        }
        
        exit (0);
    }

    if (bDisplayPowerRestore) {
        if (DisplayPowerRestoreTime (busfd, nBusDevId) < 0) {
            // An error occurred
            
            exit (1);
        }
        
        exit (0);
    }
    
    // Check whether the user wanted to read from or write to the NVRAM
    
    if (bReadNVRAM) {
        if (ReadNVRAM (busfd, nBusDevId) < 0) {
            // An error occurred
            
            exit (1);
        }
        
        exit (0);
    }
    
    if (bWriteNVRAM) {
        if (WriteNVRAM (busfd, nBusDevId, szNVRAMContents) < 0) {
            // An error occurred
            
            exit (1);
        }
        
        exit (0);
    }
    
    // If the user wanted to process an option, do it now
    
    if (bProcessOptions) {
        if (ProcessHWClockOption (busfd, nBusDevId, szOptions) < 0) {
            // An error occurred
            
            exit (1);
        }
        
        exit (0);
    }
    
    // Check to see if the user wants to get the time, or set the time
    
    if (argc == 1 || bUseComputerClockToSetRTC) {
        // The user wants to set the time
        
        if (HWSetTimeOfDay (busfd, nBusDevId, argv [0], bUseComputerClockToSetRTC) < 0) {
            // An error occurred
            
            exit (1);
        }
    }
    else {
        // The user simply wants the date/time
            
        if (HWGetTimeOfDay (busfd, nBusDevId, bDisplayDateTimeAsDateInput, bSetComputerClockFromRTC) < 0) {
            // An error occurred
            
            exit (1);
        }
    }
    
    // Simply close the device
    
    (void) CloseI2CDevice (busfd);
    
    // Return
    
    exit (0);
}

/* int ProcessHWClockOption (int busfd, int nBusDevId, char *OptionsToProcess)
**
** This option is used to process options for the PiFace Real Time Clock
*/

int ProcessHWClockOption (int busfd, int nBusDevId, char *szOptionsToProcess)
{
    char *szOption;
    struct rtc_option *pOption;
    
    // Process each of the options specified. Get the first option
    
    szOption = strtok (szOptionsToProcess, ",");
    while (szOption != (char *) 0) {
        // Go through the array of options and option processing functions looking for a match
        
        pOption = RTCOptions;
        while (pOption ->m_szOption != (char *) 0) {
            // Check to see if we have a match between the option specified and the option at the current location in the array
            
            if (! strcasecmp (szOption, pOption ->m_szOption)) {
                // We have a match - process it
                
                (*pOption ->m_pOptionFunc) (busfd, nBusDevId);
                break;
            }
            
            // Get the next option
            
            pOption ++;
        }
        
        // We should only get here because we 'broke' out of the loop after processing an option. We can sanity check, though,
        // and display a warning message if we got a bogus option
        
        if (pOption ->m_szOption == (char *) 0) {
            // We did get a bogus option
            
            (void) printf ("WARNING: Invalid option %s specified, ignoring and continuing processing!\n", szOption);
        }
        
        // Get the next option (if there are any more)
        
        szOption = strtok (0, ",");
    }
    
    // We never return anything other than 0
    
    return 0;
}

/* int HWOptionInitRTC (int busfd, int nBusDevId)
**
** This option initializes the Real Time Clock, setting the trim value, the date and time, and
** enabling the battery and external oscillator
*/

int HWOptionInitRTC (int busfd, int nBusDevId)
{
    // First, set the TRIM value 
    
    if (HWOptionCalibrateClock (busfd, nBusDevId) < 0) {
        // An error occurred
        
        (void) fprintf (stderr, "Initialization failed!\n");
        return -1;
    }
    
    // Now, enable the battery
    
    if (HWOptionBatteryEnable (busfd, nBusDevId) < 0) {
        // An error occurred
        
        (void) fprintf (stderr, "Initialization failed!\n");
        return -1;
    }
    
    // Now, enable the oscillator
    
    if (HWOptionOscillatorEnable (busfd, nBusDevId) < 0) {
        // An error occurred
        
        (void) fprintf (stderr, "Initialization failed!\n");
        return -1;
    } 
    
    // Set the date on the RTC using the computer clock
    
    if (HWSetTimeOfDay (busfd, nBusDevId, (char *) 0, true) < 0) {
        // An error occurred
        
         (void) fprintf (stderr, "Initialization failed!\n");
       return -1;
    }
    
    (void) printf ("Initialization successful.\n");
    return 0;
}

/* int HWOptionBatteryEnable (int busfd, int nBusDevId)
**
** Set the battery enable flag on the Real Time Clock
*/

int HWOptionBatteryEnable (int busfd, int nBusDevId)
{   
    return HWOptionBatteryConfigure (busfd, nBusDevId, true);
}

/* int HWOptionBatteryDisable (int busfd, int nBusDevId)
**
** Set the battery enable flag to false on the Real Time Clock
*/

int HWOptionBatteryDisable (int busfd, int nBusDevId)
{
    return HWOptionBatteryConfigure (busfd, nBusDevId, false);
}


/* int HWOptionBatteryGetSetting (int busfd, int nBusDevId)
**
** This option is used to get the status of the battery enable flag
*/

int HWOptionBatteryGetSetting (int busfd, int nBusDevId)
{
    struct mcp7940n_rtcwkday rtcwkdayBatteryStatus;
    char szErrorString [128 +1];
    
    // Clear out the buffer we will read data into
    
    bzero ((void *) &rtcwkdayBatteryStatus, sizeof (struct mcp7940n_rtcwkday));
       
    // Request the data from the RTC
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayBatteryStatus, sizeof (struct mcp7940n_rtcwkday)) < 0 ) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x getting Battery Enable (VBATEN) bit", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    // Display what was retrieved from memory
    
    (void) printf ("Battery Enable bit is %s.\n", (rtcwkdayBatteryStatus.vbaten ? "Enabled" : "Disabled"));
    return 0;
}

/* int HWOptionCalibrateClock (int busfd, int nBusDevId)
**
** This option is used to calibrate the clock. It is typically called only once.
*/

int HWOptionCalibrateClock (int busfd, int nBusDevId)
{
    struct mcp7940n_osctrim osctrimTrimValue;
    char szErrorString [128 +1];
    
    // Set the trim value. We got the value of 0x47 from the Linux driver and code for the PiFace RTC
    
    osctrimTrimValue.trimval = 0x47;
    
    // Write out the trim value
    
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_OSCTRIM_OFFSET, (void *) &osctrimTrimValue, sizeof (struct mcp7940n_osctrim)) < 0) {
        // An error occurred
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x setting trim value", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    return 0;
}
    
/* int HWOptionClearNVRAM (int busfd, int nBusDevId)
**
** Clear the NVRAM on the Real Time Clock
*/

int HWOptionClearNVRAM (int busfd, int nBusDevId)
{
    uint8_t NVRAMBuf [64];          // 64 bytes is the size of the NVRAM on the RTC
    char szErrorString [128 +1];
    
    // Clear out the buffer we will write data out from (clearing the NVRAM)
    
    bzero ((void *) NVRAMBuf, 64);
            
    // Write the data out to the RTC
    
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_NVRAM_OFFSET, NVRAMBuf, 64) < 0 ) {
        // An error occurred. All we can do is display the error
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    return 0;
}

/* int HWOptionOscillatorEnable (int busfd, int nBusDevId)
**
** This option is called to enable the external oscillator
*/

int HWOptionOscillatorEnable (int busfd, int nBusDevId)
{
    return HWOptionOscillatorConfigure (busfd, nBusDevId, true);
}

/* int HWOptionOscillatorDisable (int busfd, int nBusDevId)
**
** This option is called to disable the external oscillator
*/

int HWOptionOscillatorDisable (int busfd, int nBusDevId)
{
    return HWOptionOscillatorConfigure (busfd, nBusDevId, false);
}

/* int HWOptionOscillatorGetSetting (int busfd, int nBusDevId)
 **
 ** This option is used to get the status of the external oscillator
 */

int HWOptionOscillatorGetSetting (int busfd, int nBusDevId)
{
    struct mcp7940n_rtcsec rtcsecOscillatorSetting;
    char szErrorString [128 +1];
    
    // Clear out the buffer we will read data into
    
    bzero ((void *) &rtcsecOscillatorSetting, sizeof (struct mcp7940n_rtcsec));
        
    // Request the data from the RTC
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCSEC_OFFSET, (void *) &rtcsecOscillatorSetting, sizeof (struct mcp7940n_rtcsec)) < 0 ) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    // Display what was retrieved from memory
    
    (void) printf ("Oscillator is %s.\n", (rtcsecOscillatorSetting.st ? "Enabled" : "Disabled"));
    return 0;
}

/* int HWOptionOscillatorGetStatus (int busfd, int nBusDevId)
**
** This option is used to get the status of the external oscillator
*/

int HWOptionOscillatorGetStatus (int busfd, int nBusDevId)
{
    struct mcp7940n_rtcwkday rtcwkdayOscillatorStatus;
    char szErrorString [128 +1];
        
    // Clear out the buffer we will read data into
        
    bzero ((void *) &rtcwkdayOscillatorStatus, sizeof (struct mcp7940n_rtcwkday));
                
    // Request the data from the RTC
        
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayOscillatorStatus, sizeof (struct mcp7940n_rtcwkday)) < 0 ) {
        // An error occurred, display details and exit
            
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
        
    // Display what was retrieved from memory
        
    (void) printf ("%s\n", (rtcwkdayOscillatorStatus.oscrun ? "Oscillator is enabled and running." : "Oscillator has stopped or been disabled."));
    return 0;
}

/* int HWOptionPowerFailStatus (int busfd, int nBusDevId)
**
** Get the power fail status flag value
*/

int HWOptionPowerFailStatus (int busfd, int nBusDevId)
{
    struct mcp7940n_rtcwkday rtcwkdayPowerFailStatus;
    char szErrorString [128 +1];
    
    // Clear out the buffer we will read data into
    
    bzero ((void *) &rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday));
       
    // Request the data from the RTC
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday)) < 0 ) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x getting Power Fail Status (PWRFAIL) bit", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    // Display what was retrieved from memory
    
    (void) printf ("Power Fail Status bit is %s.\n", (rtcwkdayPowerFailStatus.pwrfail ? "Enabled" : "Disabled"));
    return 0;
}

/* int HWOptionPowerFailClearFlag (int busfd, int nBusDevId)
**
** This option is used to clear the power fail flag
*/

int HWOptionPowerFailClearFlag (int busfd, int nBusDevId)
{
    struct mcp7940n_rtcwkday rtcwkdayPowerFailStatus;
    char szErrorString [128 +1];
    
    // Clear out the buffer we will read data into
    
    bzero ((void *) &rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday));
       
    // Request the data from the RTC
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday)) < 0 ) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x getting Power Fail Status (PWRFAIL) bit", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    // Check that the Power Fail status bit is set
    
    if (rtcwkdayPowerFailStatus.pwrfail == 0) {
        // The bit is not set, so simply display a message and return
        
        (void) printf ("The Power Fail Status (PWRFAIL) bit is not set.\n");
        return 0;
    }
    
    // Clear the flag and write the byte back to the RTC
    
    rtcwkdayPowerFailStatus.pwrfail = 0;
    
    // This is a little risky as we should really turn off the oscillator input,
    // to make sure that the day of week does not rollover
    //
    // TODO: Fix this...

    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday)) < 0) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x writing cleared Power Fail Status (PWRFAIL) bit", nBusDevId);
        (void) perror (szErrorString);
        return -1;        
    }
    
    return 0;
}

/* int HWOptionControlRegistersDisplay (int busfd, int nBusDevId)
**
** This function is called to display the values of the control registers
*/

int HWOptionControlRegistersDisplay (int busfd, int nBusDevId)
{
    struct mcp7940n_control controlControlRegisters;
    char szErrorString [128 +1];
    
    // Zero out the control registers ahead of call to read them
    
    bzero (&controlControlRegisters, sizeof (struct mcp7940n_control));
    
    // Read the control registers
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_CONTROL_OFFSET, (void *) &controlControlRegisters, sizeof (struct mcp7940n_control)) < 0) {
         // An error occurred, display details and exit
            
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x reading control registers", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }

    // Display the control registers
    
    (void) printf ("Square Wave Clock Output Frequency Select: %d\n", controlControlRegisters.sqwfs);
    (void) printf ("Coarse Trim Enable:                        %d\n", controlControlRegisters.crstrim);
    (void) printf ("External Oscillator Input:                 %d\n", controlControlRegisters.extosc);
    (void) printf ("Alarm 0 Module Enable:                     %d\n", controlControlRegisters.alm0en);
    (void) printf ("Alarm 1 Module Enable:                     %d\n", controlControlRegisters.alm1en);
    (void) printf ("Square Wave Output Enable:                 %d\n", controlControlRegisters.sqwen);
    (void) printf ("Logic Level for General Purpose Output:    %d\n", controlControlRegisters.out);
    
    // Just return
    
    return 0;
}

/* int DisplayPowerFailTime (int busfd, int nBusDevId)
**
** Retrieve the power fail time from the Real Time Clock, and display it
*/

int DisplayPowerFailTime (int busfd, int nBusDevId)
{
    struct mcp7940n_rtcwkday        rtcwkdayPowerFailStatus;
    struct mcp7940n_pwrdn_timestamp timestampPowerDown;
 
    // Before we attempt to retrieve the power down timestamp, check to see if the PWRFAIL flag is
    // set or cleared. If it has been cleared we assume that there is no power down time to read
    
    bzero (&rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday));
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday)) < 0) {
        // An error occurred, and we could not read the PWRFAIL flag
        
        (void) perror ("Unable to read PWRFAIL flag, assuming no power down data/time information available.\n");
        return -1;
    }
    
    // Check to make sure that the PWRFAIL flag is set
    
    if (rtcwkdayPowerFailStatus.pwrfail == 0) {
        // This is not an error, per se. The PWRFAIL flag is cleared, so we assume that there is no power fail
        // data available for us to read
        
        (void) printf ("No power down date/time information available (PWRFAIL bit is cleared).\n");
        return 0;
    }
    
    // Read the power down date and time from the RTC
    
    bzero (&timestampPowerDown, sizeof (struct mcp7940n_pwrdn_timestamp));
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCPWRDN_OFFSET, (void *) &timestampPowerDown, sizeof (struct mcp7940n_pwrdn_timestamp)) < 0) {
        // An error occurred
        
        (void) perror ("Unable to read power down date/time from real time clock.\n");
        return -1;
    }
    
    // Display the date and time we just read in
    
    (void) printf ("%s %s %d %02d:%02d UTC\n",
        szDisplayWeekday [(timestampPowerDown.pwrdnmth.wkday -1)],
        szDisplayMonth [((timestampPowerDown.pwrdnmth.mthten *10) + timestampPowerDown.pwrdnmth.mthone -1)],
        ((timestampPowerDown.pwrdndate.dateten *10) + timestampPowerDown.pwrdndate.dateone),
        ((timestampPowerDown.pwrdnhour.twentyfourhour.twelvetwentyfour == 0)
            ? ((timestampPowerDown.pwrdnhour.twentyfourhour.hrten * 10) + timestampPowerDown.pwrdnhour.twentyfourhour.hrone)
            : (((timestampPowerDown.pwrdnhour.twelvehour.hrten * 10) + timestampPowerDown.pwrdnhour.twelvehour.hrone) + (timestampPowerDown.pwrdnhour.twelvehour.ampm == 1 ? 12 : 0))),
        ((timestampPowerDown.pwrdnminute.minten * 10) + timestampPowerDown.pwrdnminute.minone));
        
    return 0;
}

/* int DisplayPowerRestoreTime (int busfd, int nBusDevId)
**
** Retrieve the power restore time from the Real Time Clock, and display it
*/

int DisplayPowerRestoreTime (int busfd, int nBusDevId)
{
    struct mcp7940n_rtcwkday        rtcwkdayPowerFailStatus;
    struct mcp7940n_pwrup_timestamp timestampPowerUp;
 
    // Before we attempt to retrieve the power up timestamp, check to see if the PWRFAIL flag is
    // set or cleared. If it has been cleared we assume that there is no power down time to read
    
    bzero (&rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday));
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayPowerFailStatus, sizeof (struct mcp7940n_rtcwkday)) < 0) {
        // An error occurred, and we could not read the PWRFAIL flag
        
        (void) perror ("Unable to read PWRFAIL flag, assuming no power up data/time information available.\n");
        return -1;
    }
    
    // Check to make sure that the PWRFAIL flag is set
    
    if (rtcwkdayPowerFailStatus.pwrfail == 0) {
        // This is not an error, per se. The PWRFAIL flag is cleared, so we assume that there is no power fail
        // data available for us to read
        
        (void) printf ("No power up date/time information available (PWRFAIL bit is cleared).\n");
        return 0;
    }
    
    // Read the power up date and time from the RTC
    
    bzero (&timestampPowerUp, sizeof (struct mcp7940n_pwrup_timestamp));
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCPWRUP_OFFSET, (void *) &timestampPowerUp, sizeof (struct mcp7940n_pwrup_timestamp)) < 0) {
        // An error occurred
        
        (void) perror ("Unable to read power down date/time from real time clock.\n");
        return -1;
    }
    
    // Display the date and time we just read in
    
    (void) printf ("%s %s %d %02d:%02d UTC\n",
        szDisplayWeekday [(timestampPowerUp.pwrupmth.wkday -1)],
        szDisplayMonth [((timestampPowerUp.pwrupmth.mthten *10) + timestampPowerUp.pwrupmth.mthone -1)],
        ((timestampPowerUp.pwrupdate.dateten *10) + timestampPowerUp.pwrupdate.dateone),
        ((timestampPowerUp.pwruphour.twentyfourhour.twelvetwentyfour == 0)
            ? ((timestampPowerUp.pwruphour.twentyfourhour.hrten * 10) + timestampPowerUp.pwruphour.twentyfourhour.hrone)
            : (((timestampPowerUp.pwruphour.twelvehour.hrten * 10) + timestampPowerUp.pwruphour.twelvehour.hrone) + (timestampPowerUp.pwruphour.twelvehour.ampm == 1 ? 12 : 0))),
        ((timestampPowerUp.pwrupminute.minten * 10) + timestampPowerUp.pwrupminute.minone));
        
    return 0;
}

/* int HWSetTimeOfDay (int busfd, int nBusDevId, char *szDateTime, bool bUseComputerClockToSetRTC)
**
** Set the Real Time Clock with the datetime value passed to us
*/

int HWSetTimeOfDay (int busfd, int nBusDevId, char *szDateTime, bool bUseComputerClockToSetRTC)
{
    struct mcp7940n_datetime    datetimeRTCClock;
    struct timeval              tComputerDateTime;
    struct timezone             tzComputerTimezone;
    struct tm                   tmRTCDateTime, *ptmComputerDateTime;
    time_t                      timeComputerDateTime;
    struct mcp7940n_rtcwkday    rtcwkdayOscillatorStatus;
    int                         nRetryCount, nDateTimeLength;
    char                        *pszDateTimeDigit;          
    
    // Get the current date/time from the RTC. It might not be valid, but we need the various
    // flags and other settings mixed amongst the date registers
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCDATETIME_OFFSET, (void *) &datetimeRTCClock, sizeof (struct mcp7940n_datetime)) < 0) {
        // An error occurred, and we could not read the current date/time
        
        (void) perror ("Unable to read current date/time from real time clock");
        return -1;
    }
    
    // Check to see if we are to parse the command line, or if we are to use the computer clock
    
    if (bUseComputerClockToSetRTC) {
        // Get the current date/time from the computer
        
        if (gettimeofday (&tComputerDateTime, &tzComputerTimezone) < 0) {
            // An error occurred, and all we can do is display an error message and return
            
            (void) perror ("gettimeofday");
            return -1;
        }
        
        // Convert the date/time from seconds into a broken out structure we can use
        
        if ((ptmComputerDateTime = gmtime (&tComputerDateTime.tv_sec)) == (struct tm *) 0) {
            // An error occurred, so display an error message and return
            
            (void) perror ("gmtime");
            return -1;
        }        
    }
    else {
        // Copy over the date and time read in from the RTC to a tm structure. It will be easier for us to
        // manipulate it in that format
        
        bzero ((void *) &tmRTCDateTime, sizeof (struct tm));
        TranslateRTCDateTimeToTm (&datetimeRTCClock, &tmRTCDateTime);
        
        // The RTC stores date/time as UTC, but the user will enter the date and time as local time (we
        // assume). Convert the time to local time. 
        
        timeComputerDateTime = timegm (&tmRTCDateTime);
        if ((ptmComputerDateTime = localtime (&timeComputerDateTime)) == (struct tm *) 0) {
            // An error occurred, so display an error message and return
            
            (void) perror ("localtime");
            return -1;
        }
        bcopy (ptmComputerDateTime, &tmRTCDateTime, sizeof (struct tm));
        
        // Get the length of the date/time specified on the command line
        
        nDateTimeLength = strlen (szDateTime);
        if (nDateTimeLength %2 == 1) {
            // The string supplied on the command line is an odd length, so we assume it ends in
            // the characters ".SS", where SS is the seconds in the date/timer we have to write
            // to the RTC. We validate that assumption, though
            
            if ((szDateTime [(nDateTimeLength -3)] != '.') || (! isdigit (szDateTime [(nDateTimeLength -2)])) || (! isdigit (szDateTime [(nDateTimeLength -1)]))) {
                // The date/time specified on the command line is not in the format we expected
                
                goto dateformaterror;   // Ugly hack
            }
            
            // Strip off the seconds
            
            tmRTCDateTime.tm_sec = (digittoint (szDateTime [(nDateTimeLength -2)]) * 10) + digittoint (szDateTime [(nDateTimeLength -1)]);            
            if (tmRTCDateTime.tm_sec > 59)
                goto dateformaterror;
            nDateTimeLength -= 3;       // Strip off the seconds from the length, for our next calculation
        }
        
        // We can figure out what we got based on the length of the date and time (minus seconds if processed)
        
        pszDateTimeDigit = szDateTime;
        switch (nDateTimeLength) {
        case 12:        // Century, Months, Days, Hours and Minutes
            // Do nothing and fall through as the RTC does not understand century
            
            if (! isdigit (pszDateTimeDigit [0]) || (! isdigit (pszDateTimeDigit [1])))
                goto dateformaterror;
                
            pszDateTimeDigit += 2;          // Remove the century from the calculation and fall through
            
        case 10:        // Years, Months, Days, Hours and Minutes
            // Extract the years from the date specified
                       
            if (! isdigit (pszDateTimeDigit [0]) || (! isdigit (pszDateTimeDigit [1])))
                goto dateformaterror;
                
            tmRTCDateTime.tm_year = (digittoint (pszDateTimeDigit [0]) * 10) + digittoint (pszDateTimeDigit [1]) + 100; // We assume a base of 2000
            pszDateTimeDigit += 2;          // Remove the years from the calcultaion and fall through
            
        case 8:         // Months, Days, Hours and Minutes
             // Extract the months from the date specified
                       
            if (! isdigit (pszDateTimeDigit [0]) || (! isdigit (pszDateTimeDigit [1])))
                goto dateformaterror;
                
            tmRTCDateTime.tm_mon = ((digittoint (pszDateTimeDigit [0]) * 10) + digittoint (pszDateTimeDigit [1]) -1);
            if (tmRTCDateTime.tm_mon > 12)
                goto dateformaterror;
            pszDateTimeDigit += 2;          // Remove the months from the calculation and fall through
            
        case 6:         // Days, Hours and Minutes
            // Extract the days from the date specified
                       
            if (! isdigit (pszDateTimeDigit [0]) || (! isdigit (pszDateTimeDigit [1])))
                goto dateformaterror;
                
            tmRTCDateTime.tm_mday = (digittoint (pszDateTimeDigit [0]) * 10) + digittoint (pszDateTimeDigit [1]);
            if (tmRTCDateTime.tm_mday > 31)
                goto dateformaterror;
            pszDateTimeDigit += 2;          // Remove the months from the calculation and fall through
            
        case 4:         // Hours and Minutes
            // Extract the hours from the date specified
                       
            if (! isdigit (pszDateTimeDigit [0]) || (! isdigit (pszDateTimeDigit [1])))
                goto dateformaterror;
                
            tmRTCDateTime.tm_hour = (digittoint (pszDateTimeDigit [0]) * 10) + digittoint (pszDateTimeDigit [1]);
            if (tmRTCDateTime.tm_hour > 23)
                goto dateformaterror;
            pszDateTimeDigit += 2;          // Remove the hours from the calculation and fall through
            
        case 2:         // Minutes only
             // Extract the minutes from the date specified
                       
            if (! isdigit (pszDateTimeDigit [0]) || (! isdigit (pszDateTimeDigit [1])))
                goto dateformaterror;
                
            tmRTCDateTime.tm_min = (digittoint (pszDateTimeDigit [0]) * 10) + digittoint (pszDateTimeDigit [1]);
            if (tmRTCDateTime.tm_min > 59)
                goto dateformaterror;
            break;
            
        default:
            // We got something else
 
 dateformaterror:   // Ugly hack...
            
            (void) fprintf (stderr, "Illegal date format, must be [[[[[cc]yy]mm]dd]HH]MM[.ss]\n");
            return -1;
            break;
        }
        
        // Convert the tm to time_t, and then back to a tm. This populates the tm_wday value
        
        if ((timeComputerDateTime = mktime (&tmRTCDateTime)) == 0)                      // Catchall for bad date
            goto dateformaterror;
        if ((ptmComputerDateTime = gmtime (&timeComputerDateTime)) == (struct tm *) 0)  // Assume bad date
            goto dateformaterror;
    }
    
    // Copy data into the RTC clock structure. We do not zero it out first, as we do not want to
    // overwrite the flags, etc. we read in earlier

    TranslateTmToRTCDateTime (ptmComputerDateTime, &datetimeRTCClock);      
  
    // Clear the ST bit on the RTC ahead of the write to the device
    
    datetimeRTCClock.rtcseconds.st = 0;
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCSEC_OFFSET, (void *) &datetimeRTCClock.rtcseconds, sizeof (struct mcp7940n_rtcsec)) < 0) {
        // An error occurred, and we could not clear the ST bit
        
        (void) perror ("Unable to tell RTC that we were about to write out date and time");
        return -1;
    }
    
    // Sleep for 1000 microseconds, to make sure that the RTC is settled and ready for us to write. We check
    // the oscillator status bit to make sure that it is ready for us to write to. We do this up to five times
    // just "in case"
    
    for (nRetryCount = 0, rtcwkdayOscillatorStatus.oscrun = 1; ((nRetryCount < 5) && (rtcwkdayOscillatorStatus.oscrun == 1)); nRetryCount ++) {
        // Sleep for 1000 microseconds. We do this because the OSCRUN bit on the RTC should be cleared
        // after at most 32 cycles. As the RTC oscillator runs at 32,768Hz that equates to just shy of 1000
        // microseconds
        
        (void) usleep (1000);
        
        // Get the OSRUN status bit from the oscillator
        
        if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, &rtcwkdayOscillatorStatus, sizeof (struct mcp7940n_rtcwkday)) < 0) {
            // An error occurred getting the OSRUN status bit
            
            (void) perror ("Unable to read OSCRUN status bit from RTC");
            return -1;
        }
    }
    
    // Check to see if the OSCRUN bit is still set. If it is we are in an error conditions
    
    if (rtcwkdayOscillatorStatus.oscrun == 1) {
        // The OSCRUN bit is still set - display an error message and return
        
        (void) fprintf (stderr, "OSCRUN status bit is still set on the RTC, so date/time cannot be updated.\n");
        return -1;
    }
    
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCDATETIME_OFFSET, &datetimeRTCClock, sizeof (struct mcp7940n_datetime))) {
        // An error occurred writing out the date/time to the RTC
        
        (void) perror ("Unable to write out date and time to RTC");
        return -1; 
    }
    
    // Now that we have written out the date and time to the RTC we need to turn the ST bit back on
    
    datetimeRTCClock.rtcseconds.st = 1;
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCSEC_OFFSET, (void *) &datetimeRTCClock.rtcseconds, sizeof (struct mcp7940n_rtcsec)) < 0) {
        // An error occurred, and we could not clear the ST bit
        
        (void) perror ("Unable to tell RTC to re-enable the oscillator");
        return -1;
    }
 
    // Sleep for 1000 microseconds, to make sure that the RTC has detected the oscillator input. We do this up
    // to five times just "in case"
    
    for (nRetryCount = 0, rtcwkdayOscillatorStatus.oscrun = 0; ((nRetryCount < 5) && (rtcwkdayOscillatorStatus.oscrun == 0)); nRetryCount ++) {
        // Sleep for 1000 microseconds. We do this because the OSCRUN bit on the RTC should be set
        // after at most 32 cycles. As the RTC oscillator runs at 32,768Hz that equates to just shy of 1000
        // microseconds
        
        (void) usleep (1000);
        
        // Get the OSRUN status bit from the oscillator
        
        if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, &rtcwkdayOscillatorStatus, sizeof (struct mcp7940n_rtcwkday)) < 0) {
            // An error occurred getting the OSRUN status bit
            
            (void) perror ("Unable to read OSCRUN status bit from RTC");
            return -1;
        }
    }
    
    // Check to see if the OSCRUN bit is still clear. If it is we are in an error conditions
    
    if (rtcwkdayOscillatorStatus.oscrun == 0) {
        // The OSCRUN bit is still set - display an error message and return
        
        (void) fprintf (stderr, "OSCRUN status bit is still clear on the RTC, even after enabling oscillator input.\n");
        return -1;
    }
    
    // We are done! Just return
  
    return 0;
}

/* int HWGetTimeOfDay (int busfd, int nBusDevId, bool bDisplayDateTimeAsDateInput, bool bSetComputerClockFromRTC)
**
** Get the date and tine from the Real Time Clock, and display it in the format the user wants
*/

int HWGetTimeOfDay (int busfd, int nBusDevId, bool bDisplayDateTimeAsDateInput, bool bSetComputerClockFromRTC)
{
    struct mcp7940n_datetime    datetimeRTCClock;
    struct tm                   tmRTCDateTime;
    struct timeval              tvComputerDateTime;
    struct timezone             tzComputerTimezone;
    time_t                      timeRTCDateTime;
    char                        *pszRTCDateTime;
    
    // Get the current date/time from the RTC. It might not be valid, but we need the various
    // flags and other settings mixed amongst the date registers
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCDATETIME_OFFSET, (void *) &datetimeRTCClock, sizeof (struct mcp7940n_datetime)) < 0) {
        // An error occurred, and we could not read the current date/time
        
        (void) perror ("Unable to read current date/time from real time clock");
        return -1;
    }

    // Convert the RTC date and time into a tm structure
    
    bzero ((void *) &tmRTCDateTime, sizeof (struct tm));
    TranslateRTCDateTimeToTm (&datetimeRTCClock, &tmRTCDateTime);
    
    // Check to see of the user wants us to set the computer clock
    
    if (bSetComputerClockFromRTC) {
        // The user wants us to set the clock. Start by getting the current time, as we need the timezone
        
        if (gettimeofday (&tvComputerDateTime, &tzComputerTimezone)) {
            // An error occurred, display an error
            
            perror ("Call to gettimeofday failed, so unable to set computer clock");
            return -1;
        }
        
        // We convert the RTC date time read into seconds. The RTC uses UTC to record the date and time
        
        if ((tvComputerDateTime.tv_sec = timegm (&tmRTCDateTime)) == (time_t) 0) {
            // An error occurred converting the date and time from the RTC into seconds (time_t) so we are
            // unable to set the RTC
            
            perror ("Converting RTC date and time to seconds failed, unable to set clock");
            return -1;  
        }
        
        // Set the computer clock
        
        if (settimeofday (&tvComputerDateTime, &tzComputerTimezone) < 0) {
            // An error occurred, and we are unable to set the computer clock
            
            perror ("Call to settimeofday failed, unable to set RTC");
            return -1;
        }
    }
    
    // Check to see if the user wants us to display the RTC date and time in the format used by the date(1) command
    
    if (bDisplayDateTimeAsDateInput) {
        // Display as date command line input
        
        (void) printf ("%d%02d%02d%02d%02d.%02d\n",
            (tmRTCDateTime.tm_year + 1900), (tmRTCDateTime.tm_mon +1), tmRTCDateTime.tm_mday,
            tmRTCDateTime.tm_hour, tmRTCDateTime.tm_min, tmRTCDateTime.tm_sec);
    }
    else {
        // Display regular date/time string
        
        if ((timeRTCDateTime = timegm (&tmRTCDateTime)) == (time_t) 0) {
            // An error occurred
            
            perror ("Call to timegm failed");
            return -1;
        }
                
        if ((pszRTCDateTime = ctime (&timeRTCDateTime)) == (char *) 0) {
            // An error occurred
            
            perror ("Call to ctime failed");
            return -1;
        }
        
        printf ("%s", pszRTCDateTime);
    }
   
    return 0;
}

/* int ReadNVRAM (int busfd, int nBusDevId)
**
** Get the contents of the Real Time Clock's NVRAM, and dosplay it
*/

int ReadNVRAM (int busfd, int nBusDevId)
{
    uint8_t NVRAMBuf [64 +1]; // 64 bytes (size of NVRAM) plus room for a NULL
    char szErrorString [128 +1];
    
    // Clear out the buffer we will read data into
    
    bzero ((void *) NVRAMBuf, sizeof (NVRAMBuf));
       
    // Request the data from the RTC
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_NVRAM_OFFSET, (void *) NVRAMBuf, 64) < 0 ) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    // Display what was retrieved from memory
    
    (void) printf ("%s\n", (char *) NVRAMBuf);
    return 0;
}

/* int WriteNVRAM (int busfd, int nBusDevId, char *szNVRAMContents)
**
** Write to the NVRAM on the Real Time Clock
*/

int WriteNVRAM (int busfd, int nBusDevId, char *szNVRAMContents)
{
    uint8_t NVRAMBuf [64 +1]; //  64 bytes (size of NVRAM) plus a safety byte for a NULL
    char szErrorString [128 +1];
    
    // Clear out the buffer we will write data out from

    bzero ((void *) NVRAMBuf, sizeof (NVRAMBuf));
        
    //  copy the contents we will write to the NVRAM to the rest of the buffer
    
    (void) strncpy ((char *) NVRAMBuf, szNVRAMContents, 64);
    
    // Write out to the RTC's NVRAM
    
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_NVRAM_OFFSET, szNVRAMContents, 64) < 0 ) {
        // An error occurred. All we can do is display the error
        
        (void) sprintf (szErrorString, "malloc failure or ioctl I2CRDWR for nBusDevId 0x%02x", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }

    return 0;
}

/* int HWOptionBatteryConfigure (int busfd, int nBusDevId, bool bEnable)
**
** This function is used to configure the battery enable flag. We check to see if the battery enable
** bit is already set to the state the user wants, and if it is we do not write back out
*/

int HWOptionBatteryConfigure (int busfd, int nBusDevId, bool bEnable)
{
    struct mcp7940n_rtcwkday rtcwkdayBatteryEnable;
    char szErrorString [128 +1];
    
    // Clear out the buffer we will read data into
    
    bzero ((void *) &rtcwkdayBatteryEnable, sizeof (struct mcp7940n_rtcwkday));
        
    // Request the data from the RTC
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayBatteryEnable, sizeof (struct mcp7940n_rtcwkday)) < 0 ) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x getting Battery Enable (VBATEN) bit value", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    // We might not need to actually write back out to the RTC, if the bit is already set to
    // the state the user wants. Check to see what the bit is set to
    
    if (rtcwkdayBatteryEnable.vbaten == (bEnable ? 1 : 0)) {
        // The oscillator start bit is already set to the state the user wanted. Display a
        // message to the user and then simply return
        
        (void) printf ("The Battery Enable (VBATEN) bit was already set to %s.\n", (bEnable ? "Enabled" : "Disabled"));
        return 0;
    }
    
    // Update the VBATEN bit (the Battery Enable bit) write it back out to the RTC
    
    rtcwkdayBatteryEnable.vbaten = (bEnable ? 1 : 0);
    
    // This is a little risky as we should really turn off the oscillator input,
    // to make sure that the day of week does not rollover
    //
    // TODO: Fix this...
    
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCWKDAY_OFFSET, (void *) &rtcwkdayBatteryEnable, sizeof (struct mcp7940n_rtcwkday)) < 0 ) {
        // An error occurred. All we can do is display the error
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x writing Battery Enable (VBATEN) bit", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    return 0;
}

/* int HWOptionOscillatorConfigure (int busfd, int nBusDevId, bool bEnable)
**
** This function is called to actually set the oscillator configuration bit. It reads the byte that the bit is
** in, sets the bit to 1 or 0 based on bEnable, and then writes it back out to Real Time Clock
*/

int HWOptionOscillatorConfigure (int busfd, int nBusDevId, bool bEnable)
{
    struct mcp7940n_rtcsec rtcsecStartOscillator;
    char szErrorString [128 +1];
    
    // Clear out the buffer we will read data into
    
    bzero ((void *) &rtcsecStartOscillator, sizeof (struct mcp7940n_rtcsec));
        
    // Request the data from the RTC
    
    if (ReadI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCSEC_OFFSET, (void *) &rtcsecStartOscillator, sizeof (struct mcp7940n_rtcsec)) < 0 ) {
        // An error occurred, display details and exit
        
        (void) sprintf (szErrorString, "ioctl I2CRDWR for nBusDevId 0x%02x getting Oscillator Status (ST) bit", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    // We might not need to actually write back out to the RTC, if the bit is already set to
    // the state the user wants. Check to see what the bit is set to
    
    if (rtcsecStartOscillator.st == (bEnable ? 1 : 0)) {
        // The oscillator start bit is already set to the state the user wanted. Display a
        // message to the user and then simply return
        
        (void) printf ("The Oscillator Start (ST) bit was already set to %s.\n", (bEnable ? "Enabled" : "Disabled"));
        return 0;
    }
    
    // Update the ST bit (the oscillator start bit) and write it out to the RTC
    
    rtcsecStartOscillator.st = (bEnable ? 1 : 0);    
    if (WriteI2CDeviceMemory (busfd, nBusDevId, MCP7940N_RTCSEC_OFFSET, (void *) &rtcsecStartOscillator, sizeof (struct mcp7940n_rtcsec)) < 0 ) {
        // An error occurred. All we can do is display the error
        
        (void) sprintf (szErrorString, "malloc or ioctl I2CRDWR for nBusDevId 0x%02x writing Oscillator Status (ST) bit", nBusDevId);
        (void) perror (szErrorString);
        return -1;
    }
    
    return 0;
}

/* void TranslateRTCDateTimeToTm (struct mcp7940n_datetime *pdatetimeRTCClock, struct tm *ptmRTCDateTime)
**
** This function is used to convert from the date/time in RTC format to date/time in struct tm format.
*/

void TranslateRTCDateTimeToTm (struct mcp7940n_datetime *pdatetimeRTCClock, struct tm *ptmRTCDateTime)
{
    ptmRTCDateTime ->tm_sec = (pdatetimeRTCClock ->rtcseconds.secten * 10) + pdatetimeRTCClock ->rtcseconds.secone;
    ptmRTCDateTime ->tm_min = (pdatetimeRTCClock ->rtcminutes.minten * 10) + pdatetimeRTCClock ->rtcminutes.minone;
    ptmRTCDateTime ->tm_hour =
                        ((pdatetimeRTCClock ->rtchour.twentyfourhour.twelvetwentyfour == 0)
                            ? ((pdatetimeRTCClock ->rtchour.twentyfourhour.hrten * 10) + pdatetimeRTCClock ->rtchour.twentyfourhour.hrone)
                            : ((pdatetimeRTCClock ->rtchour.twelvehour.hrten * 10) + pdatetimeRTCClock ->rtchour.twelvehour.hrone + ((pdatetimeRTCClock ->rtchour.twelvehour.ampm == 0) ? 0 : 12))
                        );
    ptmRTCDateTime ->tm_wday = ((pdatetimeRTCClock ->rtcweekday.wkday == 0) ? 0 : (pdatetimeRTCClock ->rtcweekday.wkday -1));
    ptmRTCDateTime ->tm_mday = (pdatetimeRTCClock ->rtcdate.dateten * 10) + pdatetimeRTCClock ->rtcdate.dateone;
    ptmRTCDateTime ->tm_mon = ((pdatetimeRTCClock ->rtcmonth.mthten * 10) + pdatetimeRTCClock ->rtcmonth.mthone -1);
    ptmRTCDateTime ->tm_year = ((pdatetimeRTCClock ->rtcyear.yrten * 10) + pdatetimeRTCClock ->rtcyear.yrone + 100);    // The RTC does not know about century, and we assue we are in the 21st Century    
}

/* void TranslateTmToRTCDateTime (struct tm *ptmRTCDateTime, struct mcp7940n_datetime *pdatetimeRTCClock)
**
** This function is used to convert from the date time in struct tm format to date/time in RTC format.
*/

void TranslateTmToRTCDateTime (struct tm *ptmRTCDateTime, struct mcp7940n_datetime *pdatetimeRTCClock)
{
    pdatetimeRTCClock ->rtcseconds.secone = ptmRTCDateTime ->tm_sec % 10;
    pdatetimeRTCClock ->rtcseconds.secten = ptmRTCDateTime ->tm_sec / 10;
    pdatetimeRTCClock ->rtcminutes.minone = ptmRTCDateTime ->tm_min % 10;
    pdatetimeRTCClock ->rtcminutes.minten = ptmRTCDateTime ->tm_min / 10;
    pdatetimeRTCClock ->rtchour.twentyfourhour.twelvetwentyfour = 0;                        // 24 hour clock format
    pdatetimeRTCClock ->rtchour.twentyfourhour.hrone = ptmRTCDateTime ->tm_hour % 10;
    pdatetimeRTCClock ->rtchour.twentyfourhour.hrten = ptmRTCDateTime ->tm_hour / 10;
    pdatetimeRTCClock ->rtcweekday.wkday = (ptmRTCDateTime ->tm_wday +1);                // 1 is a Sunday on the RTC
    pdatetimeRTCClock ->rtcdate.dateone = ptmRTCDateTime ->tm_mday % 10;
    pdatetimeRTCClock ->rtcdate.dateten = ptmRTCDateTime ->tm_mday / 10;
    pdatetimeRTCClock ->rtcmonth.mthone = (ptmRTCDateTime ->tm_mon +1) % 10;
    pdatetimeRTCClock ->rtcmonth.mthten = (ptmRTCDateTime ->tm_mon +1) / 10;
    pdatetimeRTCClock ->rtcyear.yrone = (ptmRTCDateTime ->tm_year) % 10;         
    pdatetimeRTCClock ->rtcyear.yrten = ((ptmRTCDateTime ->tm_year) % 100) / 10;       // The RTC does not know about century  
}

/* void Usage (void)
**
** Display usage information
*/

void Usage (void)
{
    (void) printf ("pifacertc [-d nBusDevId] [-i 0|1] [-o option]\n");
    (void) printf ("pifacertc [-d nBusDevId] [-i 0|1] [-p] [-u]\n");
    (void) printf ("pifacertc [-d nBusDevId] [-i 0|1] [-r] [-w \"...\"]\n");
    (void) printf ("pifacertc [-d nBusDevId] [-i 0|1] [-c] [[[[[cc]yy]mm]dd]HH]MM[.ss]\n");
    (void) printf ("pifacertc [-d nBusDevId] [-i 0|1] [-s]\n");
    (void) printf ("pifacertc [-d nBusDevId] [-i 0|1] [-d]\n\n");
    (void) printf ("Set or get the current date/time from the PiFace RTC, or get or set options.\n\n");
    (void) printf ("-b nn              Use the nBusDevId specified (7-bit address)\n");
    (void) printf ("-c                 Set the real time clock from the computer clock.\n");
    (void) printf ("-d                 Output the date/time as input to the date command.\n");
    (void) printf ("-h                 Prints this help.\n");
    (void) printf ("-i 0|1             Use /dev/iic0 or /dev/iic1 (will guess without).\n");
    (void) printf ("-o option          Set an option on the HW RTC.\n\nThe following options are supported\n\n");
    (void) printf ("  init      Initialize the RTC, and set the date to the current date/time\n");
    (void) printf ("  bat       Enable battery backup (*)\n");
    (void) printf ("  nobat     Disable battery backup (*)\n");
    (void) printf ("  batstat   Show the battery status\n");
    (void) printf ("  clrnvram  Clear the NVRAM\n");
    (void) printf ("  cal       Calibrate the device (write 0x47 to TRIM register)\n");
    (void) printf ("  osc       Enable the oscillator (*)\n");
    (void) printf ("  noosc     Disable the oscillator (*)\n");
    (void) printf ("  oscset    Get the oscillator setting\n");
    (void) printf ("  oscstat   Display the oscialltor status\n");
    (void) printf ("  pwrstat   Get the power fail status\n");
    (void) printf ("  clrpwr    Clear the powerfail status bit if set\n\n");
    (void) printf ("Options can be separated with a comma, e.g. \"pifacertc -o bat,osc\".\n");
    (void) printf ("(*) indicates options that can corrupt the RTC if used incorrectly.\n\n");
    (void) printf ("-p                 Print the time that the power was turned off at or failed\n");
    (void) printf ("-r                 Read the contents of the NVRAM from the Real Time Clock.\n");
    (void) printf ("-s                 Set the computer clock from the RTC.\n");
    (void) printf ("-u                 Print the time that the power was turned on or restored\n");
    (void) printf ("-w \"...\"           Write to the NVRAM on the Real Time Clock.\n");
}
