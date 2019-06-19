TOP=..
include $(TOP)/configure/CONFIG

TESTPROD_HOST += testReTools
TESTS += testReTools
TARGETS += $(COMMON_DIR)/testReTools.dbd

testReTools_DBD += base.dbd retools.dbd

testReTools_SRCS = testReTools_registerRecordDeviceDriver.cpp test.c

testReTools_LIBS += retools
testReTools_LIBS += $(EPICS_BASE_IOC_LIBS)

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

include $(TOP)/configure/RULES
