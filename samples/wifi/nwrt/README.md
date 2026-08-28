# NCS Wi-Fi radio test (nWRT)

This repository contains a minimal Wi-Fi radio test application that builds
against the nRF Connect SDK (NCS) `nrf71` Wi-Fi driver in radio-test mode.

---

## 1. Getting the source

This project uses [west](https://docs.zephyrproject.org/latest/develop/west/index.html) as its workspace/manifest manager.
The first step is to initialize the workspace folder where the example-application and all nRF Connect SDK modules will be cloned.
Run the following command:

```bash
west init -m ssh://git@bitbucket.nordicsemi.no:7999/wssg/nwrt.git --mr main
west update
```

`west update` will pull in NCS, Zephyr, the `nrf` repository, and all other
modules that are referenced in `nWRT/west.yml`. Expect this to take a few
minutes the first time.

If you already have the workspace and only want to refresh the modules, run
`west update` again.

## 2. Building the application

Go to the `app/` folder inside this repository and build for the
`nrf7120dk/nrf7120/cpuapp` board:

```bash
cd nWRT/app
west build -p -b nrf7120dk/nrf7120/cpuapp
```

- `-p` does a pristine build (recommended whenever you change Kconfig or
  `CMakeLists.txt`).
- The build output is placed in `app/build/`.

## 3. Radio-test API reference

All radio-test APIs are declared in `/nrf/drivers/wifi/nrf71/osal/fw_if/umac_if/inc/radio_test/fmac_api.h`.
and implemented in `/nrf/drivers/wifi/nrf71/osal/fw_if/umac_if/src/radio_test/fmac_api.c`.
