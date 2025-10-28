# Build Instructions for nRF9151 Minimal Vercel Client

## Forutsetninger

### 1. Installer nRF Connect SDK

```bash
# Følg offisiell guide: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation.html

# Eller bruk nRF Connect for Desktop:
# https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop
```

### 2. Installer toolchain

```bash
# For Ubuntu/Debian
sudo apt update
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

# For macOS
brew install cmake ninja gperf python3 ccache qemu dtc wget libmagic
```

### 3. Installer west

```bash
pip3 install west
```

### 4. Initialiser workspace (hvis ikke gjort)

```bash
west init -m https://github.com/nrfconnect/sdk-nrf --mr main ncs
cd ncs
west update
```

## Bygge prosjektet

### Standard bygg

```bash
cd sdk-nrf
west build -b nrf9151dk_nrf9151_ns samples/cellular/minimal_vercel_client
```

### Pristine bygg (ren bygg fra scratch)

```bash
west build -b nrf9151dk_nrf9151_ns samples/cellular/minimal_vercel_client -p
```

### Bygg med ekstra logging

```bash
west build -b nrf9151dk_nrf9151_ns samples/cellular/minimal_vercel_client -- \
  -DCONFIG_LOG_DEFAULT_LEVEL=4
```

## Flashe til enhet

### Via USB

```bash
west flash
```

### Via J-Link programmer

```bash
west flash --runner jlink
```

### Manuell flashing med nrfjprog

```bash
nrfjprog --program build/zephyr/merged.hex --chiperase --verify --reset
```

## Debugging

### Start debug sesjon

```bash
west debug
```

### GDB remote debugging

```bash
# Terminal 1: Start debug server
west debugserver

# Terminal 2: Connect GDB
arm-none-eabi-gdb build/zephyr/zephyr.elf
(gdb) target remote :2331
(gdb) monitor reset
(gdb) continue
```

## Se logg output

### Via minicom

```bash
sudo minicom -D /dev/ttyACM0 -b 115200
```

### Via screen

```bash
screen /dev/ttyACM0 115200
```

### Via nRF Terminal (anbefalt for Windows)

Bruk nRF Connect for Desktop -> Terminal

## Vanlige problemer

### Problem: "west: command not found"

**Løsning:**
```bash
pip3 install --user west
export PATH=$PATH:~/.local/bin
```

### Problem: "Board not found"

**Løsning:**
```bash
# Sjekk at nRF9151 DK er tilkoblet
nrfjprog --ids

# Hvis ingen enheter:
# - Sjekk USB kabel
# - Installer nRF Command Line Tools
```

### Problem: Build feil med "mbedtls"

**Løsning:**
```bash
# Sjekk at sdk-nrfxlib er oppdatert
cd ncs
west update
```

### Problem: LTE kobler ikke

**Løsning:**
1. Sjekk at SIM kort er satt inn
2. Sjekk at antenne er koblet til
3. Sjekk nettverksdekning
4. Sjekk AT kommandoer:
```
AT+CEREG?
AT+CGDCONT?
AT+COPS?
```

## Build artefakter

Etter vellykket bygg finner du:

```
build/
├── zephyr/
│   ├── zephyr.hex      # Firmware for flashing
│   ├── zephyr.bin      # Binary format
│   ├── zephyr.elf      # ELF format med debug info
│   ├── app_update.bin  # For FOTA update
│   └── merged.hex      # Inkluderer MCUboot
├── mcuboot/           # MCUboot bootloader
└── ...
```

## FOTA (Firmware Over The Air)

### 1. Bygg med FOTA støtte (allerede aktivert)

```bash
west build -b nrf9151dk_nrf9151_ns samples/cellular/minimal_vercel_client
```

### 2. Generer signed update image

```bash
# Update image er i: build/zephyr/app_update.bin
# Last opp denne til din Vercel server
```

### 3. Server side (Vercel)

Lag `/api/fota/check` endpoint som returnerer:
```json
{
  "update_available": true,
  "version": 2,
  "url": "https://your-app.vercel.app/firmware/app_update.bin"
}
```

### 4. Over-the-air update

Enheten vil:
1. Sjekke for updates hver 5. minutt
2. Laste ned ny firmware
3. Verifisere signatur
4. Installere og reboote

## Performance tuning

### Reduser strømforbruk

Legg til i `prj.conf`:
```
CONFIG_LTE_PSM_REQ=y
CONFIG_LTE_EDRX_REQ=y
```

### Øk heap for audio streaming

Når du legger til audio:
```
CONFIG_HEAP_MEM_POOL_SIZE=65536
```

## Testing

### Unit tests (når implementert)

```bash
west build -b native_posix tests/
west build -t run
```

### Intergration tests

Se `sample.yaml` for Twister test konfigurasjon:
```bash
west twister -T samples/cellular/minimal_vercel_client
```

## Neste steg

Se `README.md` for arkitektur og hvordan legge til:
- Bluetooth support
- SD kort storage
- Audio streaming
- Spotify integration
