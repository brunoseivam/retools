TOP=../..

include $(TOP)/configure/CONFIG

LIBRARY_IOC += retools

DBD += retools.dbd
INC += retools.h


retools_CXXFLAGS += -std=c++11
retools_SRCS += retools.cpp
retools_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
