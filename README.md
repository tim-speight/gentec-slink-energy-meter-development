# slink energymeter

slink-energy-meter repository is destined for Gentec-EO PC S-Link with a 1 port or two port head.


## Installation

### IOC
Before installation make sure that you have installed EPICS properly. Depandancies STREAM, AUTOSAVE, ASYNC, CALC 

```sh
make
```

When it's done, IOC is ready to use from st.cmd. You will need to setup a procServ service runner and template from st.cmd
To start IOC:

```sh
cd iocBoot/iocslink
./st.cmd
```


