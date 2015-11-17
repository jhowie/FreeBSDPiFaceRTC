/*
**  I2CRoutines.c
**
**  Created by John Howie on 11/07/15.
**
**      j.howie@napier.ac.uk
**      john@thehowies.com
**
**  This header file contains the function prototypes for our read and write functions for the I2C bus
** device.
**
** Modifications
**
** 20151107 jh  Original.
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

# include <strings.h>
# include <stdlib.h>
# include <sys/types.h>
# include <dev/iicbus/iic.h>
# include <sys/sysctl.h>
# include <errno.h>
# include <stdio.h>
# include <fcntl.h>
# include <unistd.h>

# include "I2CRoutines.h"

/* int OpenI2CDevice (char *szDeviceName, int nBusDevID)
**
** Open the bus device specified as a parameter. If no device name is supplied we will simply guess based
** on what information we can get about the system.
*/

int OpenI2CDevice (char *szDeviceName, int nBusDevID)
{
    int nBusFD;
    char *szSelectedBusDeviceName;
    char szPErrorString [256 +1];
    int MIB [2], nNumCPUs;
    size_t nLen;
    uint8_t uiOffset [1] = { 0 }, lpBuffer [1];
    struct iic_msg iicMsg[2];
    struct iic_rdwr_data iicRdWr;

    
    if (szDeviceName != (char *) 0) {
        // Simply use the bus device specified by the user
        
        szSelectedBusDeviceName = szDeviceName;
    }
    else {
        // We have to try and guess what device to open. The Raspberry Pi A and B use /dev/iic0, whereas the B 2
        // uses /dev/iic1. We look to see how many CPUs are reported in the system to make a decision about whether
        // we are running an original Pi (one CPU) versus a model 2 (four CPUs). The sysctl equivalent is hw.ncpu
        
        
        MIB [0] = CTL_HW;
        MIB [1] = HW_NCPU;
        nLen = sizeof (nNumCPUs);
        if (sysctl (MIB, 2, &nNumCPUs, &nLen, NULL, 0) < 0) {
            // An error occurred and we could not determine the number of CPUs on the system
            
            (void) perror ("sysctl");
            return -1;
        }
        
        // Use the information we just collected to figure out what model of Raspberry Pi we are running on
            
        switch (nNumCPUs) {
        case 1: szSelectedBusDeviceName = "/dev/iic0"; break;   // This is an original Raspberry Pi
        case 4: szSelectedBusDeviceName = "/dev/iic1"; break;   // This is a model 2 Raspberry Pi
        default:
            // An error occurred
                
            (void) printf ("sysctl hw.ncpu= %d, unable to determine version of Raspberry Pi\n", nNumCPUs);
            return -1;
            break;
        }
    }

    // Open the device

    nBusFD = open (szSelectedBusDeviceName, O_RDWR);
    if (nBusFD < 0) {
        // An error occurred - display some information
        
        (void) sprintf (szPErrorString, "open %s", szSelectedBusDeviceName);
        (void) perror (szPErrorString);
        return -1;
    }
    
    // Check that the device is present. If it is not we the 'open' operation
    // is deemed to have failed. We check to see if the device is present by
    // attempting a dummy read of 0 byte from the device
       
    // Set up the offset in a message we write to the I2C bus
    
    iicMsg[0].slave = nBusDevID << 1;
    iicMsg[0].flags = IIC_M_WR;
    iicMsg[0].len = 1;
    iicMsg[0].buf = uiOffset;
    
    // Set up the buffer we read the data back into
    
    iicMsg[1].slave = nBusDevID << 1;
    iicMsg[1].flags = IIC_M_RD;
    iicMsg[1].len = 0;
    iicMsg[1].buf = lpBuffer;
    
    iicRdWr.nmsgs = 2;
    iicRdWr.msgs = iicMsg;
    
    // Request the data from the i@c device
    
    if (ioctl (nBusFD, I2CRDWR, &iicRdWr) < 0 ) {
		// An error occurred, just return -1 so the caller knows. They can
		// handle the error as they see fit
		
        (void) sprintf (szPErrorString, "ioctl I2CRDWR failed to read from device 0x%02x on bus %s", nBusDevID, szSelectedBusDeviceName);
        (void) perror (szPErrorString);
        (void) close (nBusFD);
        return -1;
    }
    
    // Simply return the nBusFD, whether or not it is an actual file descriptor
    
    return nBusFD;
}

/* int ReadI2CDeviceMemory (int busfd, int busdevid, int nOffset, void *lpBuffer, int nReadLength)
**
** This function is used to read from the I2C device's memory. The device is identified by the second parameter,
** the offset in device memory by the third, the buffer to read into the fourth, and the amount of memory to read
** is the last.
*/

int ReadI2CDeviceMemory (int busfd, int busdevid, int nOffset, void *lpBuffer, int nReadLength)
{
    uint8_t uiOffset [1];
    struct iic_msg iicMsg[2];
    struct iic_rdwr_data iicRdWr;
        
    // Set the offset into the buffer we write out to the I2C bus
    
    uiOffset [0] = (uint8_t) nOffset;
       
    // Set up the offset in a message we write to the I2C bus
    
    iicMsg[0].slave = busdevid << 1;
    iicMsg[0].flags = IIC_M_WR;
    iicMsg[0].len = 1;
    iicMsg[0].buf = uiOffset;
    
    // Set up the buffer we read the data back into
    
    iicMsg[1].slave = busdevid << 1;
    iicMsg[1].flags = IIC_M_RD;
    iicMsg[1].len = nReadLength;
    iicMsg[1].buf = lpBuffer;
    
    iicRdWr.nmsgs = 2;
    iicRdWr.msgs = iicMsg;
    
    // Request the data from the i@c device
    
    if (ioctl (busfd, I2CRDWR, &iicRdWr) < 0 ) {
		// An error occurred, just return -1 so the caller knows. They can
		// handle the error as they see fit
		
        return -1;
    }
    
    return 0;
}

/* int WriteI2CDeviceMemory (int busfd, int busdevid, int nOffset, void *lpBuffer, int nWriteLength)
**
** This function is used to write to the memory of a device on the I2C bus. The device is identified by the second
** parameter to the function, the location of the memory on the device by the third, the buffer to write from the
** fourth, and the last parameter is the number of bytes to write
*/

int WriteI2CDeviceMemory (int busfd, int busdevid, int nOffset, void *lpBuffer, int nWriteLength)
{
	uint8_t *lpOffsetAndBuffer;
    struct iic_msg iicMsg[1];
    struct iic_rdwr_data iicRdWr;
    int nStatus;
	
	// We need to allocate memory for the the buffer we write from. We cannot use the caller's buffer as we need to
	// prepend the offset to the data
	
	lpOffsetAndBuffer = (uint8_t *) malloc (nWriteLength +1);
	if (lpOffsetAndBuffer == (uint8_t *) 0) {
		// An error occurred - simply return
		
		return -1;
	}
	
    // Write the offset (the location of the NVRAM) to the first location in the buffer
    
    lpOffsetAndBuffer [0] = (uint8_t) nOffset;
    
    // Now copy the contents we will write to the NVRAM to the rest of the buffer
    
    bcopy (lpBuffer, (void *) &(lpOffsetAndBuffer [1]), nWriteLength);
    
    // Set up the buffer we write out to the I2C bus
    
    iicMsg[0].slave = busdevid << 1;
    iicMsg[0].flags = IIC_M_WR;
    iicMsg[0].len = nWriteLength +1;
    iicMsg[0].buf = lpOffsetAndBuffer;
    
    iicRdWr.nmsgs = 1;
    iicRdWr.msgs = iicMsg;

    // Write the data to the I2C device
    
    nStatus = ioctl (busfd, I2CRDWR, &iicRdWr);

	// Free up our buffer, as we no longer need fit
	
	(void) free ((void *) lpOffsetAndBuffer);
	
    return (nStatus < 0 ? -1 : 0);
}

/* int CloseI2CDevice (int busfd)
**
** This is a simple wrapper for close(2)
*/

int CloseI2CDevice (int busfd)
{
    return close (busfd);
}