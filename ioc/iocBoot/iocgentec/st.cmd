#!../../bin/linux-x86_64/gentec

## You may have to change gentec to something else
## everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/gentec.dbd"
gentec_registerRecordDeviceDriver pdbbase

epicsEnvSet("STREAM_PROTOCOL_PATH", "$(TOP)/protocols")
epicsEnvSet("PORT","EM")
epicsEnvSet("DEVICE_NAME","GENTEC-EM")
epicsEnvSet("DEVICE_SERIAL","")
epicsEnvSet("DEVICE_IP","")
epicsEnvSet("DEVICE_PORT","10001")
epicsEnvSet("STS", "-STS")
epicsEnvSet("TFL", "-TFL")

epicsEnvSet("NUM_CHANNEL", "2")
epicsEnvSet("HEAD1", "Head1")   # use Head1 to load and NOHead1 to not load
epicsEnvSet("HEAD2", "Head2")   # use Head2 to load and NOHead2 to not load
epicsEnvSet("HEAD1_NAME", "Head1")
epicsEnvSet("HEAD2_NAME", "Head2")

# Data path: :Value is I/O Intr, one sample per laser pulse from the *CA stream.
# Telemetry rate (EPICS scan menu string); suppressed while any channel streams.
epicsEnvSet("STATUS_SCAN", "5 second")    # :Status background poll; RBVs also refresh on set/stream stop/reconnect
# Over-metering alarm thresholds on :RangeFraction (reading / range full scale)
epicsEnvSet("RANGE_HIGH", "0.9")          # MINOR
epicsEnvSet("RANGE_HIHI", "1.0")          # MAJOR

stringiftest("_USB_NEEDED", "$(DEVICE_SERIAL=)", 2)
stringiftest("_TCP_NEEDED", "$(DEVICE_IP=)", 2)
$(IF_USB_NEEDED) drvAsynSerialPortConfigure("$(PORT)", "$(DEVICE_SERIAL)", 0, 0, 0)
$(IF_TCP_NEEDED) drvAsynIPPortConfigure("$(PORT)", "$(DEVICE_IP):$(DEVICE_PORT)")

# Expose asyn record for connection information
dbLoadRecords("db/asynRecord.db","P=$(DEVICE_NAME):,R=asyn,PORT=$(PORT),ADDR=0,OMAX=256,IMAX=256")

# Commissioned graphing range, one row per head (edit values in head.substitutions)
dbLoadTemplate("$(TOP)/iocBoot/iocgentec/head.substitutions", "device=$(DEVICE_NAME),HEAD1=$(HEAD1),HEAD2=$(HEAD2)")

# Conditionally Load the heads for SLINK-2
stringiftest("_HEAD1_NEEDED", "$(HEAD1)", 5, "Head1")
$(IF_HEAD1_NEEDED) dbLoadRecords("db/channel.db", "device=$(DEVICE_NAME),head=$(HEAD1),channel=1,port=$(PORT), head_name=$(HEAD1_NAME),STATUS_SCAN='$(STATUS_SCAN)'")
$(IF_HEAD1_NEEDED) dbLoadRecords("db/acquisition.db", "device=$(DEVICE_NAME),head=$(HEAD1),channel=1,port=$(PORT),RANGE_HIGH=$(RANGE_HIGH),RANGE_HIHI=$(RANGE_HIHI)")
$(IF_HEAD1_NEEDED) dbLoadRecords("db/rms.db", "device=$(DEVICE_NAME),head=$(HEAD1)")

stringiftest("_HEAD2_NEEDED", "$(HEAD2)", 5, "Head2")
$(IF_HEAD2_NEEDED) dbLoadRecords("db/channel.db", "device=$(DEVICE_NAME),head=$(HEAD2),channel=2,port=$(PORT), head_name=$(HEAD2_NAME),STATUS_SCAN='$(STATUS_SCAN)'")
$(IF_HEAD2_NEEDED) dbLoadRecords("db/acquisition.db", "device=$(DEVICE_NAME),head=$(HEAD2),channel=2,port=$(PORT),RANGE_HIGH=$(RANGE_HIGH),RANGE_HIHI=$(RANGE_HIHI)")
$(IF_HEAD2_NEEDED) dbLoadRecords("db/rms.db", "device=$(DEVICE_NAME),head=$(HEAD2)")

dbLoadRecords("db/slink.db", "device=$(DEVICE_NAME),port=$(PORT)")

#Connection state and monitoring
dbLoadRecords("db/iocAdminSoft.db", "IOC=$(DEVICE_NAME)$(STS)")
dbLoadRecords("db/trafficLight.db", "device=$(DEVICE_NAME), TFL-PREFIX=$(DEVICE_NAME)$(TFL), TFL=$(TFL)")
dbLoadRecords("db/heartbeat.db", "device=$(DEVICE_NAME)")

#autosave
set_savefile_path("$(TOP)/autoSaveRestore")
set_requestfile_path("$(TOP)/autoSaveRestore")
set_pass0_restoreFile("energy_meter_two_heads.sav")
set_pass1_restoreFile("energy_meter_two_heads.sav")

# deploy machine protection if needed 
epicsEnvSet("MPS_DEPLOYMENT_SCRIPT", "NoMPS")
< ${TOP}/iocBoot/${IOC}/${MPS_DEPLOYMENT_SCRIPT}

cd ${TOP}/iocBoot/${IOC}

iocInit

## Start any sequence programs
#seq sncxxx,"user=swdev"

#Live comms - Debugging
#asynSetTraceMask("$(PORT)",-1,0x9)
#asynSetTraceIOMask("$(PORT)",-1,0x2)

# Stop acquisition at the hardware (*CS) to avoid undefined state on IOC reboot
#epicsThreadSleep(1)
dbpf $(DEVICE_NAME):$(HEAD1):StopAcquisition.PROC 1
dbpf $(DEVICE_NAME):$(HEAD2):StopAcquisition.PROC 1
#epicsThreadSleep(1)

# autosave monitor set, 5 sec period
create_monitor_set("energy_meter_two_heads.req", 5, "HEAD1=$(DEVICE_NAME):$(HEAD1), HEAD2=$(DEVICE_NAME):$(HEAD2)")

epicsThreadSleep(1)
