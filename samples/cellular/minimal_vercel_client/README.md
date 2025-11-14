# Minimal nRF9151 Vercel Client

**Kraftig forbedret versjon** basert på `nrf_cloud_multi_service` arkitektur, tilpasset for Vercel REST API.

## ✨ Viktige forbedringer (v2.0)

### Arkitektur fra `nrf_cloud_multi_service`

Denne versjonen bruker **bevist patterns** fra Nordic's `nrf_cloud_multi_service` sample (3241 linjer), men forenklet til **~400 linjer** uten nRF Cloud avhengigheter:

| Pattern | Fra multi_service | I vår implementasjon |
|---------|-------------------|----------------------|
| **Location library** | `location_tracking.c` (159 linjer) | `init_location_tracking()` - Støtter GNSS + Cellular + WiFi fallback |
| **Connection management** | `cloud_connection.c` (700 linjer) | `lte_handler()` - State tracking og event handling |
| **Periodic sampling** | `application.c` (478 linjer) | `application_thread()` - Main loop pattern |
| **Date/time sync** | Integrert | `date_time_update_async()` - Automatisk NTP synkronisering |
| **HTTP client** | `message_queue.c` pattern | `http_post_json()` - Direkte REST til Vercel |

### Hva dette gir deg

✅ **Location library** - Automatisk fallback: GNSS → Cellular → WiFi
✅ **Connection state** - Vent på LTE før sending
✅ **Timestamping** - Unix timestamps på alle meldinger
✅ **Robust error handling** - Timeouts, retries, state management
✅ **Periodisk FOTA** - Rate-limited firmware sjekker
✅ **Production-ready patterns** - Testet arkitektur fra Nordic

## Hva dette gjør (v2.0)

```
┌─────────────────────────────────────────┐
│  main() → application_thread()          │
│  - Initialize modem                     │
│  - Wait for LTE connection              │
│  - Start location tracking (periodic)   │
│  - Check FOTA updates (periodic)        │
└─────────────────────────────────────────┘
           │
           ├─→ Location Library
           │   ├─ GNSS (primary)
           │   ├─ Cellular (fallback)
           │   └─ WiFi (if available)
           │
           ├─→ HTTP Client → Vercel
           │   ├─ POST /api/location
           │   └─ POST /api/fota/check
           │
           └─→ LTE Connection Manager
               └─ State tracking + auto-reconnect
```

### Implementert

- ✅ **LTE tilkobling** - Med state tracking og event handling
- ✅ **Location library** - GNSS + Cellular fallback (ikke bare GNSS!)
- ✅ **HTTP/HTTPS** - REST client til Vercel
- ✅ **Date/time sync** - NTP automatisk synkronisering
- ✅ **FOTA sjekker** - Periodisk med rate limiting
- ✅ **MCUboot** - Bootloader for sikker FOTA
- ✅ **Timestamping** - Unix timestamps på alle meldinger

### Fortsatt mangler (fra v1.0)

❌ **Bluetooth** - nRF9151 har IKKE Bluetooth chip
❌ **Audio streaming** - Krever BT + AAC codec
❌ **SD kort** - Krever SPI driver + FAT FS
❌ **Spotify** - Krever OAuth + Web API
❌ **Full FOTA** - Download og flash ikke implementert (kun sjekk)

## Sammenligning: v1.0 vs v2.0

| Feature | v1.0 (250 linjer) | v2.0 (400 linjer) |
|---------|-------------------|-------------------|
| GPS | Direkte `nrf_modem_gnss` | **Location library** (GNSS + Cellular + WiFi) |
| Connection | Enkel wait-loop | **State management** med events |
| Timestamping | Ingen | **date_time library** med NTP |
| HTTP | Basis REST client | **Error handling** + state check |
| FOTA | Enkel sjekk | **Rate limited** med timestamp tracking |
| Error handling | Minimal | **Robust** fra multi_service patterns |
| Code quality | Prototype | **Production-ready** patterns |

## Bygge og flashe

### Forutsetninger
- nRF Connect SDK 2.5.0+
- nRF9151 DK
- SIM kort eller eSIM

### Bygg

```bash
west build -b nrf9151dk_nrf9151_ns samples/cellular/minimal_vercel_client -p
```

### Flash

```bash
west flash
```

### Se logger

```bash
minicom -D /dev/ttyACM0 -b 115200
```

## Konfigurasjon

### 1. Sett Vercel server URL

I `Kconfig`:
```kconfig
config VERCEL_SERVER_HOSTNAME
	string "Vercel server hostname"
	default "your-app.vercel.app"
```

Eller override i `boards/nrf9151dk_nrf9151_ns.conf`:
```
CONFIG_VERCEL_SERVER_HOSTNAME="my-app.vercel.app"
```

### 2. Juster GPS intervall

```
CONFIG_GPS_UPDATE_INTERVAL_SEC=60  # Default 60 sekunder
```

### 3. Juster FOTA sjekk intervall

```
CONFIG_FOTA_CHECK_INTERVAL_MIN=5  # Default 5 minutter
```

### 4. Enable WiFi positioning (krever nRF7002)

I `prj.conf`:
```
CONFIG_LOCATION_METHOD_WIFI=y
```

## Vercel Server API

Se `VERCEL_SERVER_EXAMPLE.md` for komplett implementasjon.

### `/api/location` (POST)

**Request:**
```json
{
  "lat": 59.9139,
  "lon": 10.7522,
  "alt": 12.5,
  "accuracy": 10.0,
  "method": "gnss",
  "ts": 1705320600
}
```

**Response:**
```json
{
  "success": true,
  "address": "Oslo, Norway",
  "timestamp": "2024-01-15T10:30:00Z"
}
```

### `/api/fota/check` (POST)

**Request:**
```json
{
  "version": 1
}
```

**Response (no update):**
```json
{
  "update_available": false,
  "version": 1
}
```

**Response (update available):**
```json
{
  "update_available": true,
  "version": 2,
  "url": "https://your-app.vercel.app/firmware/app_update.bin",
  "size": 524288,
  "checksum": "sha256:..."
}
```

## Kodestruktur

Koden er organisert i logiske seksjoner (pattern fra multi_service):

```c
/* HTTP/HTTPS Client */
static int http_post_json(const char *endpoint, const char *json_data);

/* Location Tracking */
static void location_event_handler(const struct location_event_data *event_data);
static int init_location_tracking(void);

/* FOTA Support */
static void check_fota_update(void);

/* LTE Connection Management */
static void lte_handler(const struct lte_lc_evt *const evt);
static int init_lte(void);

/* Main Application Thread */
static void application_thread(void);
int main(void);
```

## Debugging

### Enable debug logging

I `prj.conf`:
```
CONFIG_LOG_DEFAULT_LEVEL=4  # DEBUG level
CONFIG_LOCATION_LOG_LEVEL_DBG=y
CONFIG_LTE_LINK_CONTROL_LOG_LEVEL_DBG=y
```

### Vanlige problemer

#### GPS får ikke fix

```
# Sjekk at GNSS er aktivert
AT%XSYSTEMMODE=1,0,1,0

# Sjekk GNSS status
AT%XMONITOR

# For raskere fix, bruk assisted GNSS (krever cloud service)
```

#### LTE kobler ikke

```
# Sjekk network registration
AT+CEREG?

# Sjekk operator
AT+COPS?

# Sjekk signal strength
AT+CSQ
```

#### HTTP requests feiler

```
# Sjekk DNS
nslookup your-app.vercel.app

# Sjekk certificates (må være riktig root CA)
# Vercel bruker Let's Encrypt
```

## Performance og strømforbruk

### Gjeldende konfigurasjon

- GPS fix: Hver 60 sekund
- FOTA sjekk: Hver 5 minutt
- Idle strøm: ~5mA (med PSM)
- Aktiv strøm: ~100mA (under GNSS fix)

### Optimalisering for batteri

I `prj.conf`:
```
# Enable Power Saving Mode (PSM)
CONFIG_LTE_PSM_REQ=y
CONFIG_LTE_PSM_REQ_RPTAU="00100001"  # 10 minutes
CONFIG_LTE_PSM_REQ_RAT="00000000"    # 0 seconds

# Enable extended DRX (eDRX)
CONFIG_LTE_EDRX_REQ=y

# Reduce GPS frequency
CONFIG_GPS_UPDATE_INTERVAL_SEC=300  # 5 minutes

# Use cellular positioning instead of GNSS
CONFIG_LOCATION_METHOD_GNSS=n
CONFIG_LOCATION_METHOD_CELLULAR=y
```

## Neste steg: Full implementasjon

### 1. Full FOTA implementasjon

Se `samples/cellular/http_update/application_update/` for referanse:

```c
#include <net/fota_download.h>

static void fota_download_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download completed, rebooting...");
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("FOTA download failed: %d", evt->error);
		break;
	}
}

int download_and_apply_fota(const char *url)
{
	return fota_download_start_with_image_type(
		url, NULL, -1, 0, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
}
```

### 2. Legg til Bluetooth (krever ekstra hardware)

**Anbefalt arkitektur:**
```
┌──────────────┐         UART        ┌──────────────┐
│   nRF9151    │<------------------->│   nRF52840   │
│  (LTE + GPS) │                     │  (Bluetooth) │
└──────────────┘                     └──────────────┘
                                            │
                                     ┌──────┴──────┐
                                     │  BT Audio   │
                                     │  Device     │
                                     └─────────────┘
```

Se `samples/cellular/lte_ble_gateway/` for gateway implementasjon.

### 3. Legg til SD kort

```c
#include <zephyr/fs/fs.h>
#include <ff.h>

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = "/SD:"
};

int init_sd_card(void)
{
	return fs_mount(&mp);
}
```

### 4. Legg til audio streaming

Krever:
- AAC codec bibliotek (libfdk-aac eller lignende)
- I2S driver for audio output
- Buffer management (minst 64KB)
- Bluetooth A2DP profil

Estimert tilleggskode: ~2000 linjer

### 5. Spotify integration

```javascript
// Vercel server side
const SpotifyWebApi = require('spotify-web-api-node');

app.post('/api/audio/stream', async (req, res) => {
	const track = await spotifyApi.getTrack(trackId);
	// Return audio stream URL (krever Premium API)
	res.json({ stream_url: track.preview_url });
});
```

## Ressurser

### Nordic Documentation
- [nRF Cloud Multi-Service Sample](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/cellular/nrf_cloud_multi_service/README.html)
- [Location Library](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/modem/location.html)
- [FOTA Updates](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/networking/fota_download.html)

### Eksempler i SDK
- `samples/cellular/nrf_cloud_multi_service/` - Komplett referanse (3241 linjer)
- `samples/cellular/http_update/application_update/` - Full FOTA implementasjon
- `samples/cellular/gnss/` - Dedikert GNSS sample
- `samples/cellular/lte_ble_gateway/` - LTE + Bluetooth gateway

### Vercel
- [Vercel Docs](https://vercel.com/docs)
- [Serverless Functions](https://vercel.com/docs/functions)
- [Edge Network](https://vercel.com/docs/edge-network)

## Estimert kompleksitet

| Funksjonalitet | Linjer kode | Status |
|----------------|-------------|--------|
| LTE + HTTP + GPS (v2.0) | ~400 | ✅ Implementert |
| Full FOTA download | +200 | 🔨 TODO |
| Bluetooth bridge | +500 | ❌ Krever HW |
| SD kort storage | +300 | ❌ TODO |
| Audio codec + streaming | +2000 | ❌ TODO |
| Spotify API | +800 | ❌ TODO |
| **TOTALT** | **~4200** | 10% ferdig |

## Lisens

Basert på Nordic Semiconductor samples (LicenseRef-Nordic-5-Clause).

Merk: Spotify streaming krever commercial lisens for produksjon.

## Support

- **Nordic DevZone**: https://devzone.nordicsemi.com
- **Vercel Discord**: https://vercel.com/discord
- **nRF Cloud Multi-Service**: https://github.com/nRFCloud/application-protocols
