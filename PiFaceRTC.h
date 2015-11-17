/*
**  PiFaceRTC.h
**
**  Created by John Howie on 10/27/15.
**
**      j.howie@napier.ac.uk
**      john@thehowies.com
**
**  This header file contains the structures and definitions used in our PiFace Real Time Clock
**  application.
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

#ifndef PiFaceRTC_h
#define PiFaceRTC_h

/*
** The following are the variopus structures that make up the representation of date/time and control registers
** in the PiFace RTC
*/
 
struct mcp7940n_rtcsec {            // Offset 0x00
    unsigned char secone : 4;
    unsigned char secten : 3;
    unsigned char st : 1;
};

struct mcp7940n_rtcmin {            // Offset 0x01
    unsigned char minone : 4;
    unsigned char minten : 3;
    unsigned char unimplemented : 1;
};

struct mcp7940n_rtchour_12hour {    // Offset 0x02
    unsigned char hrone : 4;
    unsigned char hrten : 1;
    unsigned char ampm : 1;
    unsigned char twelvetwentyfour : 1;
    unsigned char unimplemented : 1;
};

struct mcp7940n_rtchour_24hour {    // Offset 0x02
    unsigned char hrone : 4;
    unsigned char hrten : 2;
    unsigned char twelvetwentyfour : 1;
    unsigned char unimplemented : 1;
};

struct mcp7940n_rtcwkday {          // Offset 0x03
    unsigned char wkday : 3;
    unsigned char vbaten : 1;
    unsigned char pwrfail : 1;
    unsigned char oscrun : 1;
    unsigned char unimplemented : 2;
};

struct mcp7940n_rtcdate {           // Offset 0x04
    unsigned char dateone : 4;
    unsigned char dateten : 2;
    unsigned char unimplemented : 2;
};

struct mcp7940n_rtcmnth {           // Offset 0x05
    unsigned char mthone : 4;
    unsigned char mthten : 1;
    unsigned char lpyr : 1;
    unsigned char unimplemented : 2;
};

struct mcp7940n_rtcyear {           // Offset 0x06
    unsigned char yrone : 4;
    unsigned char yrten : 4;
};

struct mcp7940n_control {           // Offset 0x07
    unsigned char sqwfs : 2;
    unsigned char crstrim : 1;
    unsigned char extosc : 1;
    unsigned char alm0en : 1;
    unsigned char alm1en : 1;
    unsigned char sqwen : 1;
    unsigned char out : 1;
    
};

struct mcp7940n_osctrim {           // Offset 0x08
    unsigned char trimval : 7;
    unsigned char sign : 1;
};

struct mcp7940n_datetime {
    struct mcp7940n_rtcsec  rtcseconds;
    struct mcp7940n_rtcmin  rtcminutes;
    union {
        struct mcp7940n_rtchour_12hour twelvehour;
        struct mcp7940n_rtchour_24hour twentyfourhour;
    } rtchour;
    struct mcp7940n_rtcwkday rtcweekday;
    struct mcp7940n_rtcdate rtcdate;
    struct mcp7940n_rtcmnth rtcmonth;
    struct mcp7940n_rtcyear rtcyear;
};

struct mcp7940n_pwrmin {
    unsigned char minone : 4;
    unsigned char minten : 3;
    unsigned char unimplemented : 1;
};

struct mcp7940n_pwr12hour {
    unsigned char hrone : 4;
    unsigned char hrten : 1;
    unsigned char ampm : 1;
    unsigned char twelvetwentyfour : 1;
    unsigned char unimplemented : 1;
};

struct mcp7940n_pwr24hour {
    unsigned char hrone : 4;
    unsigned char hrten : 2;
    unsigned char twelvetwentyfour : 1;
    unsigned char unimplemented : 1;

};

struct mcp7940n_pwrdate {
    unsigned char dateone : 4;
    unsigned char dateten : 2;
    unsigned char unimplemented : 2;
};

struct mcp7940n_pwrmth {
    unsigned char mthone : 4;
    unsigned char mthten : 1;
    unsigned char wkday : 3;
};

struct mcp7940n_pwrdn_timestamp {   // Offset 0x18
    struct mcp7940n_pwrmin  pwrdnminute;
    union {
        struct mcp7940n_pwr12hour twelvehour;
        struct mcp7940n_pwr24hour twentyfourhour;
    } pwrdnhour;
    struct mcp7940n_pwrdate pwrdndate;
    struct mcp7940n_pwrmth  pwrdnmth;
};

struct mcp7940n_pwrup_timestamp {   // Offset 0x1c
    struct mcp7940n_pwrmin  pwrupminute;
    union {
        struct mcp7940n_pwr12hour twelvehour;
        struct mcp7940n_pwr24hour twentyfourhour;
    } pwruphour;
    struct mcp7940n_pwrdate pwrupdate;
    struct mcp7940n_pwrmth  pwrupmth;
};

struct mcp7940n_pwrtimestamps {
    struct mcp7940n_pwrdn_timestamp pwrdn;
    struct mcp7940n_pwrup_timestamp pwrup;
};

/*
** The following are offsets in the memory of the RTC to various registers and fields
*/

# define MCP7940N_RTCSEC_OFFSET         0x00
# define MCP7940N_RTCMIN_OFFSET         0x01
# define MCP7940N_RTCHOUR_OFFSET        0x02
# define MCP7940N_RTCWKDAY_OFFSET       0x03
# define MCP7940N_RTCDATE_OFFSET        0x04
# define MCP7940N_RTCMTH_OFFSET         0x05
# define MCP7940N_RTCYEAR_OFFSET        0x06
# define MCP7940N_CONTROL_OFFSET        0x07
# define MCP7940N_OSCTRIM_OFFSET        0x08
# define MCP7940N_PWRDNMIN_OFFSET       0x18
# define MCP7940N_PWRDNHOUR_OFFSET      0x19
# define MCP7940N_PWRDNDATE_OFFSET      0x1a
# define MCP7940N_PWRDNMTH_OFFSET       0x1b
# define MCP7940N_PWRUPMIN_OFFSET       0x1c
# define MCP7940N_PWRUPHOUR_OFFSET      0x1d
# define MCP7940N_PWRUPDATE_OFFSET      0x1e
# define MCP7940N_PWRUPMTH_OFFSET       0x1f

# define MCP7940N_RTCDATETIME_OFFSET    0x00
# define MCP7940N_RTCPWRDNUP_OFFSET     0x18
# define MCP7940N_RTCPWRDN_OFFSET       0x18
# define MCP7940N_RTCPWRUP_OFFSET       0x1c
# define MCP7940N_NVRAM_OFFSET          0x20

#endif /* PiFaceRTC_h */
