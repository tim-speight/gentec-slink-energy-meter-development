# slink energymeter

slink-energy-meter repository is destined for Gentec-EO PC S-Link with a 1 port or two port head.


## Installation

### IOC
Before installation make sure that you have installed EPICS properly. Dependencies: ASYN, STREAM, AUTOSAVE, CALC, SSCAN, IOCSTATS, UTILITIES, OPCUA (see `ioc/configure/RELEASE`), plus PVA/QSRV from EPICS base and libpcre (`PCRE_LIB` in `ioc/configure/CONFIG_SITE`).

```sh
make
```

When it's done, IOC is ready to use from st.cmd. You will need to setup a procServ service runner and template from st.cmd
To start IOC:

```sh
cd ioc/iocBoot/iocgentec
./st.cmd
```

### st.cmd environment variables

| Variable | Default | Purpose |
|---|---|---|
| `STATUS_SCAN` | `5 second` | Background poll of `:Status`, suppressed while any channel streams. RBVs are also refreshed after any setter, when streaming stops and on asyn reconnect |
| `RANGE_HIGH` | `0.9` | MINOR alarm on `:RangeFraction` (reading / range full scale) |
| `RANGE_HIHI` | `1.0` | MAJOR alarm on `:RangeFraction` (over-metering / saturation) |

Commissioned graphing range per head (`GS_LOW`/`GS_HIGH`) is set in `ioc/iocBoot/iocgentec/head.substitutions`.

### Acquisition model
`StartAcquisition` / `StopAcquisition` drive the hardware (`*SS…1`, `*VNP`, `*CA` / `*CS`). In streaming mode the S-LINK pushes one ASCII line per laser pulse; `:Value` is `I/O Intr` and processes once per pulse. While any channel streams (`$(device):Streaming` = 1), solicited reads on the shared port (status, name, serial, range) are suppressed, and the setters that need `*CS` first (scale, scale up/down, external trigger, energy mode) are disabled on the streaming head.

### Readback aliases
`Wavelength_RBV`, `Scale_RBV` (scale string, e.g. `10nJ`), `TrigLev_RBV`, `Attenuator_RBV`, `ExtTrig_RBV`, `Anticipation_RBV`, `EnergyMode_RBV` alias the existing readback records; the original names are unchanged.
