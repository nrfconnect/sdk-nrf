# Vercel Server Setup Eksempel

Dette dokumentet viser hvordan sette opp en Vercel server som kan kommunisere med nRF9151 enheten.

## Forutsetninger

- Node.js installert (versjon 18+)
- Vercel account: https://vercel.com
- Vercel CLI: `npm install -g vercel`

## Prosjekt struktur

```
vercel-nrf-server/
├── api/
│   ├── location.js       # GPS koordinater endpoint
│   ├── fota/
│   │   └── check.js      # FOTA check endpoint
│   └── audio/
│       ├── upload.js     # Lyd upload endpoint
│       └── stream.js     # Lyd streaming endpoint
├── public/
│   └── firmware/
│       └── app_update.bin # FOTA firmware filer
├── package.json
└── vercel.json
```

## 1. Opprett prosjekt

```bash
mkdir vercel-nrf-server
cd vercel-nrf-server
npm init -y
```

## 2. Installer dependencies

```bash
npm install @vercel/node dotenv
```

## 3. Lag API endpoints

### `/api/location.js` - GPS koordinater

```javascript
// api/location.js
export default async function handler(req, res) {
  // Kun tillat POST
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  try {
    const { lat, lon, alt } = req.body;

    // Valider data
    if (typeof lat !== 'number' || typeof lon !== 'number') {
      return res.status(400).json({ error: 'Invalid coordinates' });
    }

    // Logg koordinater
    console.log(`GPS Update: Lat ${lat}, Lon ${lon}, Alt ${alt}m`);

    // Her kan du:
    // - Lagre til database (Vercel KV, Supabase, etc.)
    // - Sende til Google Maps API for geocoding
    // - Sende til routing API for ruteveiledning
    // - Triggere webhooks

    // Eksempel: Hent adresse fra Google Maps
    const address = await getAddress(lat, lon);

    // Response
    return res.status(200).json({
      success: true,
      message: 'Location received',
      location: { lat, lon, alt },
      address: address,
      timestamp: new Date().toISOString()
    });

  } catch (error) {
    console.error('Location error:', error);
    return res.status(500).json({ error: 'Internal server error' });
  }
}

// Google Maps Geocoding (krever API key)
async function getAddress(lat, lon) {
  const apiKey = process.env.GOOGLE_MAPS_API_KEY;
  if (!apiKey) return null;

  try {
    const url = `https://maps.googleapis.com/maps/api/geocode/json?latlng=${lat},${lon}&key=${apiKey}`;
    const response = await fetch(url);
    const data = await response.json();

    if (data.results && data.results[0]) {
      return data.results[0].formatted_address;
    }
  } catch (error) {
    console.error('Geocoding error:', error);
  }
  return null;
}
```

### `/api/fota/check.js` - FOTA firmware check

```javascript
// api/fota/check.js
export default async function handler(req, res) {
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  try {
    const { version } = req.body;

    // Valider version
    if (typeof version !== 'number') {
      return res.status(400).json({ error: 'Invalid version' });
    }

    console.log(`FOTA check from device version ${version}`);

    // Definer latest version
    const LATEST_VERSION = 2;
    const FIRMWARE_URL = `${req.headers.origin || 'https://your-app.vercel.app'}/firmware/app_update.bin`;

    // Sjekk om update er tilgjengelig
    if (version < LATEST_VERSION) {
      return res.status(200).json({
        update_available: true,
        version: LATEST_VERSION,
        url: FIRMWARE_URL,
        size: 524288, // bytes
        checksum: 'sha256:abc123...', // SHA256 hash
        release_notes: 'Bug fixes and improvements',
        mandatory: false
      });
    }

    // Ingen update tilgjengelig
    return res.status(200).json({
      update_available: false,
      version: version,
      message: 'Device is up to date'
    });

  } catch (error) {
    console.error('FOTA check error:', error);
    return res.status(500).json({ error: 'Internal server error' });
  }
}
```

### `/api/audio/upload.js` - Audio upload (for fremtidig bruk)

```javascript
// api/audio/upload.js
export const config = {
  api: {
    bodyParser: {
      sizeLimit: '10mb', // Tillat store audio filer
    },
  },
};

export default async function handler(req, res) {
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  try {
    const { audio_data, format, duration } = req.body;

    // Valider audio data (base64 encoded)
    if (!audio_data || !format) {
      return res.status(400).json({ error: 'Missing audio data' });
    }

    console.log(`Audio upload: ${format}, ${duration}s, ${audio_data.length} bytes`);

    // Her kan du:
    // - Lagre til cloud storage (AWS S3, Vercel Blob, etc.)
    // - Transkribere med speech-to-text API
    // - Prosessere audio med ML modell
    // - Sende til tredjepartssystemer

    // Response
    return res.status(200).json({
      success: true,
      message: 'Audio received',
      audio_id: Date.now().toString(),
      timestamp: new Date().toISOString()
    });

  } catch (error) {
    console.error('Audio upload error:', error);
    return res.status(500).json({ error: 'Internal server error' });
  }
}
```

## 4. Lag `vercel.json` konfigurasjon

```json
{
  "version": 2,
  "builds": [
    {
      "src": "api/**/*.js",
      "use": "@vercel/node"
    }
  ],
  "routes": [
    {
      "src": "/api/(.*)",
      "dest": "/api/$1"
    },
    {
      "src": "/firmware/(.*)",
      "dest": "/public/firmware/$1"
    }
  ],
  "headers": [
    {
      "source": "/api/(.*)",
      "headers": [
        {
          "key": "Access-Control-Allow-Origin",
          "value": "*"
        },
        {
          "key": "Access-Control-Allow-Methods",
          "value": "GET, POST, OPTIONS"
        },
        {
          "key": "Access-Control-Allow-Headers",
          "value": "Content-Type"
        }
      ]
    }
  ]
}
```

## 5. Deploy til Vercel

```bash
# Login
vercel login

# Deploy
vercel

# Eller deploy til production
vercel --prod
```

## 6. Sett environment variables

```bash
# Via Vercel CLI
vercel env add GOOGLE_MAPS_API_KEY

# Eller via Vercel Dashboard:
# https://vercel.com/your-project/settings/environment-variables
```

## 7. Test endpoints

### Test location endpoint

```bash
curl -X POST https://your-app.vercel.app/api/location \
  -H "Content-Type: application/json" \
  -d '{"lat":59.9139,"lon":10.7522,"alt":12.5}'
```

Response:
```json
{
  "success": true,
  "message": "Location received",
  "location": {
    "lat": 59.9139,
    "lon": 10.7522,
    "alt": 12.5
  },
  "address": "Oslo, Norway",
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

### Test FOTA endpoint

```bash
curl -X POST https://your-app.vercel.app/api/fota/check \
  -H "Content-Type: application/json" \
  -d '{"version":1}'
```

Response:
```json
{
  "update_available": true,
  "version": 2,
  "url": "https://your-app.vercel.app/firmware/app_update.bin",
  "size": 524288,
  "checksum": "sha256:abc123...",
  "release_notes": "Bug fixes and improvements"
}
```

## 8. Oppdater nRF9151 firmware

I `src/main.c`, endre:
```c
#define VERCEL_SERVER "your-app.vercel.app"
```

## 9. Integrasjon med andre services

### Google Maps Directions API

```javascript
// Hent ruteveiledning
async function getDirections(lat, lon, destination) {
  const apiKey = process.env.GOOGLE_MAPS_API_KEY;
  const origin = `${lat},${lon}`;

  const url = `https://maps.googleapis.com/maps/api/directions/json?origin=${origin}&destination=${destination}&key=${apiKey}`;

  const response = await fetch(url);
  const data = await response.json();

  if (data.routes && data.routes[0]) {
    return {
      distance: data.routes[0].legs[0].distance.text,
      duration: data.routes[0].legs[0].duration.text,
      steps: data.routes[0].legs[0].steps.map(step => ({
        instruction: step.html_instructions,
        distance: step.distance.text
      }))
    };
  }
  return null;
}
```

### Spotify API (for fremtidig audio streaming)

```javascript
// Merk: Spotify streaming krever Premium API tilgang
async function getSpotifyTrack(accessToken) {
  const response = await fetch('https://api.spotify.com/v1/me/player/currently-playing', {
    headers: {
      'Authorization': `Bearer ${accessToken}`
    }
  });

  const data = await response.json();
  return data;
}
```

## 10. Database integration (Vercel KV)

```bash
npm install @vercel/kv
```

```javascript
// api/location.js
import { kv } from '@vercel/kv';

export default async function handler(req, res) {
  const { lat, lon, alt } = req.body;

  // Lagre til Vercel KV
  const deviceId = req.headers['x-device-id'] || 'default';
  await kv.set(`location:${deviceId}`, {
    lat, lon, alt,
    timestamp: Date.now()
  });

  // Hent siste 10 lokasjoner
  const locations = await kv.get(`locations:${deviceId}`);

  return res.json({ success: true });
}
```

## 11. Monitoring og logging

Vercel gir gratis logging. Se logs med:

```bash
vercel logs
```

Eller i Vercel Dashboard:
https://vercel.com/your-project/logs

## 12. Sikkerhet

### API Key authentication

```javascript
// api/location.js
export default async function handler(req, res) {
  const apiKey = req.headers['x-api-key'];

  if (apiKey !== process.env.DEVICE_API_KEY) {
    return res.status(401).json({ error: 'Unauthorized' });
  }

  // ... rest of handler
}
```

Oppdater nRF firmware:
```c
req_ctx.header_fields =
  "Content-Type: application/json\r\n"
  "X-API-Key: your-secret-key\r\n";
```

### Rate limiting

```bash
npm install @upstash/ratelimit
```

```javascript
import { Ratelimit } from '@upstash/ratelimit';
import { kv } from '@vercel/kv';

const ratelimit = new Ratelimit({
  redis: kv,
  limiter: Ratelimit.slidingWindow(10, '1 m'), // 10 requests per minute
});

export default async function handler(req, res) {
  const identifier = req.headers['x-device-id'];
  const { success } = await ratelimit.limit(identifier);

  if (!success) {
    return res.status(429).json({ error: 'Too many requests' });
  }

  // ... rest of handler
}
```

## Ressurser

- Vercel Docs: https://vercel.com/docs
- Google Maps API: https://developers.google.com/maps
- Spotify API: https://developer.spotify.com/
- Vercel KV: https://vercel.com/docs/storage/vercel-kv
- Rate Limiting: https://upstash.com/docs/redis/features/ratelimiting
