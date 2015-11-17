/*
**  I2CRoutines.h
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

#ifndef I2CRoutines_h
#define I2CRoutines_h

int OpenI2CDevice (char *szDeviceName, int busdevid);
int ReadI2CDeviceMemory (int busfd, int busdevid, int nOffset, void *lpBuffer, int nReadLength);
int WriteI2CDeviceMemory (int busfd, int busdevid, int nOffset, void *lpBuffer, int nWriteLength);
int CloseI2CDevice (int busfd);

#endif // I2CRoutines_h