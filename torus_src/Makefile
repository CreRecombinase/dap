#CFLAGS =-Wall -g -O0 -march=native
CFLAGS=  -Wall -g -O3 -march=native -DGSL_RANGE_CHECK_OFF -DHAVE_INLINE
main: main.o controller.o logistic.o zstd_h5plugin.o
	$(CXX)  $(CFLAGS) main.o controller.o logistic.o  zstd_h5plugin.o -lhdf5 -lhdf5_hl -lm -lgsl -lgslcblas -lboost_iostreams -lz  -lzstd  -o torus
static: main.o controller.o logistic.o
	$(CXX)  $(CFLAGS) main.o controller.o logistic.o  -lm -lgsl -lgslcblas -lboost_iostreams -lz -static -o torus.static
main.o: main.cc
	$(CXX) $(CFLAGS) -c  main.cc
controller.o: controller.cc classdef.h
	$(CXX)  $(CFLAGS) -c  controller.cc
logistic.o: logistic.cc logistic.h
	$(CXX) $(CFLAGS) -c logistic.cc
zstd_h5plugin.o: zstd_h5plugin.c zstd_h5plugin.h
	$(CC) $(CFLAGS) -c zstd_h5plugin.c -lzstd -lhdf5 -lhdf5_hl

clean:
	rm *.o torus
