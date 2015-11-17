OBJECTS=I2CRoutines.o PiFaceRTCFreeBSD.o

rtcdate: $(OBJECTS)
	cc -o rtcdate $(OBJECTS)

I2CRoutines.o: I2CRoutines.h
PiFaceRTCFreeBSD.o: I2CRoutines.h PiFaceRTC.h

clean:
	rm $(OBJECTS) rtcdate
	
install:	rtcdate
	install -d /usr/local/bin -o root -g wheel -v
	install -o root -g wheel -c -v rtcdate /usr/local/bin/
	
