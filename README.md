# FreeBSDPiFaceRTC
PiFace Real Time Clock utility for FreeBSD 11.0 on Raspberry Pi

Run 'make install' to compile and install rtcdate into /usr/local/bin. From
the command line type 'rtcdate -h' for details of command line switches and
options. A manual page will follow!

Features
--------

* Runs in usermode (no kernel drivers required)
* Query power down and power up times
* Write to and read from the 64 bytes of NVRAM on the PiFace Real Time Clock
* Easy initialization
* Query various parameters, registers, etc.

Quick Start
-----------

1. Download to an empty folder on your Raspberry Pi
2. Run 'make install'
3. Make sure /usr/local/bin is on your executable path, and use 'rehash' if
   using /bin/csh
4. Make sure your computer clock is correct
5. Run 'rtcdate -o init' to initialize the PiFace Real Time Clock (there
   should be no errors)
6. Run 'rtcdate' to check that the date set on the PiFace RTC
7. Add 'rtcdate -s' to /etc/rc.local (create it first if it does not exist -
   see rc.local(8) for details)
8. OPTIONAL IF YOU USE NTP - set up a cron task to run 'rtcdate -c' on a
   periodic basis to keep the clock accurate
   
---

Please report any errors or problems to john@thehowies.com.
