TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = gentec
# gentec.dbd will be created and installed
DBD += gentec.dbd

# gentec.dbd will be made up from these files:
gentec_DBD += base.dbd calc.dbd asyn.dbd sscan.dbd stream.dbd AsynReg.dbd asSupport.dbd asub.dbd devIocStats.dbd iocAdmin.dbd opcua.dbd utilities.dbd PVAServerRegister.dbd qsrv.dbd
#gentec_DBD += utilities.dbd

# Add all the support libraries needed by this IOC
gentec_LIBS += calc sscan asyn stream autosave devIocStats qsrv pvData opcua utilities
pcre_DIR += /usr/local/lib
gentec_LIBS += pcre
gentec_LIBS += qsrv pvData
# gentec_registerRecordDeviceDriver.cpp derives from gentec.dbd
gentec_SRCS += gentec_registerRecordDeviceDriver.cpp
gentec_SRCS += monitor_connection.c
gentec_SRCS += calculations.c
gentec_SRCS += scale_mapper.c
# Build the main IOC entry point on workstation OSs.
gentec_SRCS_DEFAULT += gentecMain.cpp
gentec_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#gentec_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
gentec_LIBS += $(EPICS_BASE_IOC_LIBS)
gentec_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

