#CFLAGS =-Wall -g -O0 -march=native
CFLAGS=  -Wall -g -O3 -march=native -DGSL_RANGE_CHECK_OFF -DHAVE_INLINE
main: main.o controller.o logistic.o
	$(CXX)  $(CFLAGS) main.o controller.o logistic.o  -lm -lgsl -lgslcblas -lboost_iostreams -lz  -o torus
static: main.o controller.o logistic.o
	$(CXX)  $(CFLAGS) main.o controller.o logistic.o  -lm -lgsl -lgslcblas -lboost_iostreams -lz -static -o torus.static
main.o: main.cc
	$(CXX) $(CFLAGS) -c  main.cc
controller.o: controller.cc classdef.h
	$(CXX)  $(CFLAGS) -c  controller.cc
logistic.o: logistic.cc logistic.h
	$(CXX) $(CFLAGS) -c logistic.cc

clean:
	rm *.o torus
