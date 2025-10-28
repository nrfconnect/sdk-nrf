# Minimal nRF9151 Vercel Client

**REALISTISK** minimal firmware for nRF9151 med kjernefunksjonalitet.

## Hva dette gjør nå (v1.0)

✅ **LTE tilkobling** - Kobler til 4G nettverk (støtter eSIM/softSIM)
✅ **HTTP/HTTPS klient** - Sender data til Vercel server
✅ **GPS** - Leser koordinater og sender til server
✅ **FOTA** - Sjekker for firmware oppdateringer

## Hva som MANGLER (må legges til)

❌ **Bluetooth** - nRF9151 har IKKE Bluetooth! Du må bruke:
   - Ekstern nRF52/nRF53 chip for BT, eller
   - nRF9151 + nRF52/53 i gateway-modus (se `lte_ble_gateway` sample)

❌ **Audio streaming** - Krever:
   - Bluetooth chip for lydstreaming (A2DP)
   - AAC codec implementasjon (stort bibliotek)
   - Buffer management
   - Sanntids audio prosessering

❌ **SD kort** - Krever:
   - SPI driver for SD kort
   - FAT filsystem (Zephyr FS API)
   - DMA for rask lesing/skriving

❌ **Spotify** - Krever:
   - Spotify Web API integrasjon
   - OAuth autentisering
   - Lisenser for streaming

## Bygge og flashe

### Forutsetninger
- nRF Connect SDK installert (versjon 2.5.0 eller nyere)
- nRF9151 DK
- SIM kort eller eSIM konfigurert

### Bygg kommando
```bash
west build -b nrf9151dk_nrf9151_ns samples/cellular/minimal_vercel_client
```

### Flash kommando
```bash
west flash
```

### Se logger
```bash
minicom -D /dev/ttyACM0 -b 115200
# eller
screen /dev/ttyACM0 115200
```

## Konfigurasjon

### 1. Sett Vercel server URL

Endre i `src/main.c`:
```c
#define VERCEL_SERVER "your-app.vercel.app"
```

### 2. eSIM/SoftSIM konfigurasjon

Hvis du bruker eSIM, aktiver i `prj.conf`:
```
CONFIG_LTE_NETWORK_USE_FALLBACK=y
```

### 3. Vercel server API

Lag disse endepunktene på Vercel:

#### `/api/location` (POST)
Mottar GPS koordinater:
```json
{
  "lat": 59.9139,
  "lon": 10.7522,
  "alt": 12.5
}
```

#### `/api/fota/check` (POST)
Sjekker firmware versjon:
```json
{
  "version": 1
}
```

Response hvis oppdatering tilgjengelig:
```json
{
  "update_available": true,
  "version": 2,
  "url": "https://your-app.vercel.app/firmware/app_v2.bin"
}
```

## Neste steg for komplett løsning

### 1. Legg til Bluetooth (krever hardware endring)
- Bruk nRF9151 + nRF52840/nRF5340
- Implementer UART bridge mellom chips
- Se `samples/cellular/lte_ble_gateway`

### 2. Legg til SD kort
```c
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fat_fs.h>
// Implementer SPI driver og mount filsystem
```

### 3. Legg til audio streaming
- Trenger AAC codec (f.eks. libfdk-aac)
- Buffer management for sanntidsstreaming
- Minst 64KB RAM for audio buffers

### 4. Implementer full FOTA
- Download firmware fra server
- Skriv til sekundær MCUboot slot
- Valider og swap boot slots
- Se `samples/cellular/http_update/application_update`

## Estimert kodemengde for komplett løsning

| Funksjonalitet | Linjer kode |
|----------------|-------------|
| LTE + HTTP + GPS (nå) | ~250 |
| Bluetooth bridge | ~500 |
| Audio codec + streaming | ~2000 |
| SD kort + filsystem | ~300 |
| Full FOTA | ~400 |
| Spotify API | ~800 |
| **TOTALT** | **~4250** |

## Debugging

### Sjekk LTE tilkobling
```
AT+CEREG?
AT+CGDCONT?
```

### Sjekk GPS status
```
AT%XMODEMSLEEP=0
AT%XSYSTEMMODE=1,0,1,0
```

### Sjekk modem versjon
```
AT+CGMR
```

## Arkitektur for komplett løsning

```
┌─────────────┐
│  nRF9151    │
│  (LTE+GPS)  │
└──────┬──────┘
       │ UART
┌──────┴──────┐
│  nRF52840   │
│  (Bluetooth)│
└──────┬──────┘
       │ I2S/SPI
┌──────┴──────┐
│  Audio Chip │
│  (AAC codec)│
└─────────────┘

      │ SPI
┌─────┴──────┐
│  SD Card   │
└────────────┘
```

## Lisensiering

Merk: Spotify streaming krever:
- Spotify Developer Account
- Premium API tilgang
- Commerciell lisens for produksjon

## Support

For problemer med:
- **nRF SDK**: https://devzone.nordicsemi.com
- **Vercel**: https://vercel.com/docs
- **Dette prosjektet**: Sjekk koden, kommenter, eksperimenter!

## Viktige notater

⚠️ **Dette er et utgangspunkt**, ikke en komplett løsning!
⚠️ **50 linjer er umulig** for alle funksjoner
⚠️ **Bluetooth krever ekstra hardware**
⚠️ **Audio streaming er komplekst** og krever store ressurser
⚠️ **Test grundig** før produksjon
