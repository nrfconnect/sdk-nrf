This sample shall be manually compiled and flashed onto two boards that are connected together.
Pay attention to the IO standard - use voltage converter when connecting
nrf52/nrf53 (3V logic) with nrf54l/nrf54h (1.8V logic).

One board shall be compiled with CONFIG_ROLE_HOST=1 (called below as a HOST).
While the other board shall be compiled with CONFIG_ROLE_HOST=0 (called below as a DEVICE).

HOST uses SPIM instance to exchange data.
DEVICE uses SPIS instance to exchange data.
SPI works in full duplex - it sends and receives data at the same time.

General description:
HOST generates data, sends it to the DEVICE.
DEVICE receives data and sends it back to HOST in next transaction.
HOST verifies that received data is equal to data sent in previous transaction.
DEVICE verifies that each byte is greater by DELTA from previous byte.

Detailed test scenario for HOST:
1. HOST generates data N to be send in transaction N.
   Data consists of DATA_FIELD_LEN (16) bytes.
   Value of the next byte is greater by DELTA (1) from the previous byte.
2. HOST exchanges data with the DEVICE (transaction N).
   HOST sends data N to the DEVICE.
   At the same time HOST receives from the DEVICE previously sent data (N-1).
3. HOST verifies that received data matches with what was sent in previous transaction.
4. HOST reports status of data validation:
   Success:
        [01:07:14.912,484] <inf> app: Run 1 - PASS
   Failure:
        [00:00:00.014,807] <err> app: FAIL: rx[0] = 32, expected 21
        [00:00:00.014,833] <err> app: FAIL: rx[1] = 33, expected 213
        [00:00:00.014,838] <err> app: FAIL: rx[2] = 34, expected 10
        [01:08:04.435,863] <inf> app: Run 0 - FAILED
5. HOST sleeps for 500 ms.

Detailed test scenario for DEVICE:
1. DEVICE exchanges data with the HOST (transaction N).
   DEVICE receives new data N.
   At the same time DEVICE sends to HOST previously received data (N-1).
2. DEVICE verifies that byte[i] is equal to (byte[i-1] + DELTA).
3. DEVICE reports status of data validation:
   Success:
        [00:00:49.424,665] <inf> app: Run 1 - PASS
   Failure:
        [00:00:00.899,520] <err> app: FAIL: rx[0] = 80, expected 1
        [00:00:00.899,543] <inf> app: Run 0 - FAILED

Four connections between two boards are needed.
GPIO numbers can be found in the overlay file for the target.
HOST:        DEVICE:
SPIM_CS   -  SPIS_CS
SPIM_SCK  -  SPIS_SCK
SPIM_MISO -  SPIS_MISO
SPIM_MOSI -  SPIS_MOSI

When nrf54l15dk is HOST and nrf54h20dk is DEVICE then:
      nrf54l      nrf54h
CS:   P2.10   -   P0.11
SCK:  P1.13   -   P0.01
MISO: P1.11   -   P0.07
MOSI: P1.09   -   P0.09

When nrf54l15dk is DEVICE and nrf54h20dk is HOST then:
      nrf54l      nrf54h
CS:   P1.14   -   P0.10
SCK:  P1.12   -   P0.00
MISO: P1.10   -   P0.06
MOSI: P1.08   -   P0.08

Note:
1. First transaction after power on is always failed.
2. Timestamp and run counter is local to board:
   - they are not synchronized between HOST and DEVICE;
   - values are zeroed when board is reset;
