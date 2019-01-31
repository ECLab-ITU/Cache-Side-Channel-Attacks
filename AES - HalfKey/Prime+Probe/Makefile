include config
export

LD_LIBRARY_PATH = $(OPENSSL)

all: clean spy 
clean:
	rm -f *.o spy
spy: spy.cpp cacheutils.h
	g++ -DOPENSSL=\"$(OPENSSL)\" -DNUMBER_OF_ENCRYPTIONS=$(NUMBER_OF_ENCRYPTIONS) -DTHRESHOLD=$(THRESHOLD) -DTABLE0=$(TABLE0) -DTABLE1=$(TABLE1) -DTABLE2=$(TABLE2) -DTABLE3=$(TABLE3) -std=gnu++11 -O0 -o $@ $< -I$(OPENSSL)/include -L$(OPENSSL) -lcrypto -Wl,-rpath=$(OPENSSL)
	
