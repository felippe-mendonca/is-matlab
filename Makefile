MATLABDIR = /usr/local/MATLAB/R2016b
MEX       = $(MATLABDIR)/bin/mex
MATLAB    = $(MATLABDIR)/bin/matlab -nodisplay -noFigureWindows -nosplash

OPENCV_FLAGS = $(shell pkg-config --libs opencv)
OTHER_FLAGS  = $(shell pkg-config --libs --cflags librabbitmq libSimpleAmqpClient)
CXXFLAGS = "-ansi -fexceptions -fPIC -fno-omit-frame-pointer -pthread -std=c++14 -DMX_COMPAT_32 -D_GNU_SOURCE -DMATLAB_MEX_FILE"

all: MxArray is

MxArray: MxArray.cpp
	$(MEX) $< -c $(OPENCV_FLAGS) -v CXXFLAGS=$(CXXFLAGS)

is: is.cpp MxArray.o
	$(MEX) $? $(OTHER_FLAGS) $(OPENCV_FLAGS) -v CXXFLAGS=$(CXXFLAGS)

clean:
	rm -f MxArray.o is.mexa64