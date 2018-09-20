TOP=../..

include $(TOP)/configure/CONFIG

LIBRARY_IOC += retools

DBD += retools.dbd
INC += retools.h

# specify all source files to be compiled and added to the library
retools_SRCS += retools.c

retools_SYS_LIBS += $(subst -l,,$(shell pcre2-config --libs8))
retools_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
