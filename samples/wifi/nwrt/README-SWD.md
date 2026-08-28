# SWD host control (shadow v8)

Three **fixed** RAM blocks (`common`, `TX`, `RX`), each **`0x100` bytes** apart, plus a separate **RX capture buffer**. Control words (`submit`, `result`, `ack`) are at the **start** of each block (after `magic`/`version` on common), their addresses do not change when new params are appended in that block. Adding TX fields does **not** move RX.

`submit` packs **pending (low 16 bits) + seq (high 16 bits)** in one `w4`.

Host access is **`w4` only** (32-bit words). RX capture samples: **`0x2007b000`**, 16 KiB.

---

## Fixed base addresses (for board `nrf7120dk/nrf7120/cpuapp`)

| Block | Base | DT slot size |
|-------|------|----------------|
| **common** | `0x2007f000` | `0x100` |
| **TX** | `0x2007f100` | `0x100` |
| **RX** | `0x2007f200` | `0x100` |
| **rx_cap_buf** | `0x2007b000` | 16 KiB |

There are **`0x100`** (256 B) between each blocks. Unused bytes in each slot are reserved for future params (append only at the end of the param section).

**Sanity check:**

```text
mem32 0x2007f000, 2
```

Expect `4e575254 00000008` (magic + version **8**).

---

## Stable control addresses (should not change even when adding new params)

| Block | `submit` | `result` | `ack` |
|-------|----------|----------|-------|
| common | `0x2007f008` | `0x2007f00C` | `0x2007f010` |
| TX | `0x2007f100` | `0x2007f104` | `0x2007f108` |
| RX | `0x2007f200` | `0x2007f204` | `0x2007f208` |

### `submit` word layout

| Bits | Field |
|------|--------|
| `[15:0]` | pending flags |
| `[31:16]` | seq (optional; see ATE vs lab below) |

`submit = (seq << 16) | pending`

Firmware clears **`submit` to `0`** when the command finishes.

---

## Memory layout

Each field is **`uint32_t`** (4 bytes).

### Common (`0x2007f000`)

| Address | Field |
|---------|--------|
| `0x2007f000` | `magic` (`0x4e575254`) |
| `0x2007f004` | `version` (`8`) |
| `0x2007f008` | `submit` |
| `0x2007f00C` | `result` |
| `0x2007f010` | `ack` |
| `0x2007f014` | `band_idx` |
| `0x2007f018` | `channel` |

### TX (`0x2007f100`)

| Address | Field |
|---------|--------|
| `0x2007f100` | `submit` |
| `0x2007f104` | `result` |
| `0x2007f108` | `ack` |
| `0x2007f10C` | `tx_power` |
| `0x2007f110` | `tx_tone_freq` |
| `0x2007f114` | `tx_pkt_len` |
| `0x2007f118` | `tx_pkt_num` |
| `0x2007f11C` | `tx_pkt_mcs` |
| `0x2007f120` | `tx_pkt_rate` |
| `0x2007f124` | `tx_pkt_tput_mode` |
| `0x2007f128` | `enable_tx` (`0`/`1` for `PROG`) |
| `0x2007f12C` | `tx_tone` (`0`/`1` for `TONE`) |

### RX (`0x2007f200`)

| Address | Field |
|---------|--------|
| `0x2007f200` | `submit` |
| `0x2007f204` | `result` |
| `0x2007f208` | `ack` |
| `0x2007f20C` | `cap_timeout_status` |
| `0x2007f210` | `lna_gain` |
| `0x2007f214` | `bb_gain` |
| `0x2007f218` | `capture_length` (in hex, max **5461** (0x1555) samples) |
| `0x2007f21C` | `capture_timeout` |
| `0x2007f220` | `enable_rx` (`0`/`1` for `PROG`) |
| `0x2007f224` | `rx_cap_type` (0 ADC, 1 STAT_PKT, 2 DYN_PKT) |

After **`RX_CAP`**, read **`capture_length x 3`** bytes from **`0x2007b000`** (e.g. from `0x2007b000` to `0x2007b2FF` for 256 samples, because 256x3 = 768 = 0x300).

---

## Pending flags (low 16 bits of `submit`)

### Common

| Bit | Value | Action |
|-----|-------|--------|
| `CONF_INIT` | 1 | Reset `conf_params` defaults |
| `RADIO_INIT` | 2 | `radio_test_init(band_idx, channel)` |

### TX

| Bit | Value | Action |
|-----|-------|--------|
| `APPLY` | 1 | Copy TX shadow &rarr; `conf_params` |
| `PROG` | 2 | `nrf_wifi_rt_fmac_prog_tx()` (packet TX, set `enable_tx`) |
| `TONE` | 4 | CW tone (does **not** need `PROG`, set `tx_tone`) |

### RX

| Bit | Value | Action |
|-----|-------|--------|
| `APPLY` | 1 | Copy RX shadow &rarr; `conf_params` |
| `PROG` | 2 | `nrf_wifi_rt_fmac_prog_rx()` |
| `STATS` | 4 | Fill `nwrt_shadow_stats` |
| `RX_CAP` | 8 | `nrf_wifi_rt_fmac_rf_test_rx_cap()` &rarr; `0x2007b000` |

---

## ATE operator flow (no seq / no ack)

Write **pending only** in the low 16 bits (upper 16 bits = 0). Firmware auto-assigns
seq internally. Same binary as lab debug, mode is chosen by how you write `submit`.

1. `w4` param words you need.
2. `w4` `submit` = **pending** (e.g. `2` for `RADIO_INIT`).
3. Poll until done, either:
   - **`mem32 <submit>`** until **`0`** (command consumed), then read **`result`**, or
   - **`mem32 <result>`** until **not `-256`** (`NWRT_SHADOW_RESULT_RUNNING`).
4. **`result`**: `0` = OK; negative = error (see table below).

To re-run the same command, write `submit` again after it cleared to `0`.

### ATE examples

**Radio init 2.4 GHz channel 6** (RADIO_INIT always needs to be called)

```text
w4 0x2007f014, 0     # band_idx = 0 (2.4G)
w4 0x2007f018, 6     # channel = 6 (2437 MHz)
w4 0x2007f008, 2     # RADIO_INIT (2)
mem32 0x2007f008, 1
mem32 0x2007f00C, 1
```

**CW tone, power 12**

```text
w4 0x2007f10C, 12    # tx_power = 12
w4 0x2007f12C, 1     # tx_tone = 1
w4 0x2007f100, 5     # APPLY|TONE (1|4)
mem32 0x2007f100, 1
mem32 0x2007f104, 1
```

**Stop CW tone**

```text
w4 0x2007f12C, 0     # tx_tone = 0
w4 0x2007f100, 4     # TONE (4)
mem32 0x2007f100, 1
mem32 0x2007f104, 1
```

**Packet TX, `tx_power = 12`, start** (not working)

```text
w4 0x2007f10C, 12    # tx_power = 12
w4 0x2007f128, 1     # enable_tx = 1
w4 0x2007f100, 3     # APPLE|PROG (1|2)
mem32 0x2007f100, 1
mem32 0x2007f104, 1
```

**Stop Packet TX**

```text
w4 0x2007f128, 0     # enable_tx = 0
w4 0x2007f100, 2     # PROG (2)
mem32 0x2007f100, 1
mem32 0x2007f104, 1
```

**RX start**

```text
w4 0x2007f220, 1     # enable_rx = 1
w4 0x2007f200, 2     # PROG (2)
mem32 0x2007f200, 1
mem32 0x2007f204, 1
```

**Stop RX**

```text
w4 0x2007f220, 0     # enable_rx = 0
w4 0x2007f200, 2     # PROG (2)
mem32 0x2007f200, 1
mem32 0x2007f204, 1
```

**RX ADC capture 1024 samples**

```text
w4 0x2007f218, 400   # capture_length = 1024 (0x400)
w4 0x2007f224, 0     # rx_cap_type = ADC
w4 0x2007f200, 9     # APPLY|RX_CAP (1|8)
mem32 0x2007f200, 1
mem32 0x2007f204, 1
mem32 0x2007f20C, 1
```

**Read **3072 (1024 * 3)** bytes from `0x2007b000`**

```text
mem32 0x2007b000 768    # 768 = (1024 * 3) / 4
```

---

## Lab debug flow (explicit seq + ack)

Put **seq in the upper 16 bits** and poll **`ack == seq`**. Same firmware as ATE,
if the upper half of `submit` is non-zero, that seq is used as-is.

1. `w4` params.
2. `w4` `submit` = `pending | (seq << 16)`, increment **seq** every command.
3. Poll **`mem32 <ack>`** until **`ack == seq`**.
4. Read **`result`**.

**Radio init 2.4 GHz channel 6 (seq = 1)** (RADIO_INIT always needs to be called)

```text
w4 0x2007f014, 0
w4 0x2007f018, 6
w4 0x2007f008, 0x00010002
mem32 0x2007f010, 1
mem32 0x2007f00C, 1
```

**CW tone (seq = 2)** (`APPLY|TONE` = 5)

```text
w4 0x2007f10C, 12
w4 0x2007f12C, 1
w4 0x2007f100, 0x00020005
mem32 0x2007f108, 1
mem32 0x2007f104, 1
```

**Stop CW tone (seq = 3)** (`TONE` = 4)

```text
w4 0x2007f12C, 0
w4 0x2007f100, 0x00030004
mem32 0x2007f108, 1
mem32 0x2007f104, 1
```

**Start Packet TX (seq = 4)** (`APPLY|PROG` = 3)

```text
w4 0x2007f10C, 12
w4 0x2007f128, 1
w4 0x2007f100, 0x00040003
mem32 0x2007f108, 1
mem32 0x2007f104, 1
```

**Stop Packet TX (seq = 5)** (`PROG` = 2)

```text
w4 0x2007f128, 0
w4 0x2007f100, 0x00050002
mem32 0x2007f108, 1
mem32 0x2007f104, 1
```

**Start RX (seq = 6)** (`PROG` = 2)

```text
w4 0x2007f220, 1
w4 0x2007f200, 0x00060002
mem32 0x2007f208, 1
mem32 0x2007f204, 1
```

**Stop RX (seq = 7)** (`PROG` = 2)

```text
w4 0x2007f220, 0
w4 0x2007f200, 0x00070002
mem32 0x2007f208, 1
mem32 0x2007f204, 1
```

**RX ADC capture 1024 samples (seq = 8)** (`APPLY|RX_CAP` = 9)

```text
w4 0x2007f218, 400
w4 0x2007f224, 0
w4 0x2007f200, 0x00080009
mem32 0x2007f208, 1
mem32 0x2007f204, 1
mem32 0x2007f20C, 1
```

Then read **768** bytes from **`0x2007b000`**.

---

### SWD-only test build (no Wi-Fi driver)

Does **not** link or initialize the nRF71 Wi-Fi driver (avoids IPC hang). Shadow poll
runs immediately, RF commands return **`result = -19`**.

```bash
west build -p -b nrf7120dk/nrf7120/cpuapp -- -DSB_CONFIG_WIFI_NRF70=n -DCONFIG_SOC_NRF71_WIFI_DAP=y
```

Verify the flashed image has no Wi-Fi driver:

```bash
arm-zephyr-eabi-nm build/app/zephyr/zephyr.elf | grep wifi_ipc_host_tx_send
```

No output = correct SWD-only image.

Quick protocol check (`CONF_INIT`, seq `1`):

```text
w4 0x2007f008, 0x00010001
mem32 0x2007f010, 1
mem32 0x2007f00C, 1
```

(`0x2007f010` (ack) = `1`, `0x2007f00C` (result) = `-19`)

---

## Result codes

| Value | Meaning |
|-------|---------|
| `0` | OK |
| `-256` | Running (`NWRT_SHADOW_RESULT_RUNNING`, ATE poll until not this) |
| `-5` | FMAC / driver failed |
| `-16` | Busy |
| `-19` | RPU not ready |
| `-22` | Invalid args / magic / version |
