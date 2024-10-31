This sample shall be manually compiled and flashed onto two boards that are connected together.
Pay attention to the IO standard - use voltage converter when connecting
nrf52/nrf53 (3V logic) with nrf54l/nrf54h (1.8V logic).

One board shall be compiled with CONFIG_ROLE_HOST=1 (called below as a HOST).
While the other board shall be compiled with CONFIG_ROLE_HOST=0 (called below as a DEVICE).

HOST uses TWIM instance to exchange data.
DEVICE uses TWIS instance to exchange data.

General description:
HOST generates data and writes it to the DEVICE.
After reception, DEVICE verifies that each received byte is greater by DELTA from previous byte.
DEVICE prepares for read request from HOST - DEVICE will return data back to HOST.
HOST reads data from DEVICE and verifies that received data is equal to data that was sent.

Detailed test scenario for HOST:
1. HOST generates data to be send.
   Data consists of DATA_FIELD_LEN (16) bytes.
   Value of the next byte is greater by DELTA (1) from the previous byte.
2. HOST executes write to the DEVICE.
3. HOST executes read from the DEVICE.
4. HOST verifies that received data matches with what was written to the DEVICE.
5. HOST reports status of data validation:
   Success:
        [01:07:14.912,484] <inf> app: Run 1 - PASS
   Failure:
        [00:00:00.014,807] <err> app: FAIL: rx[0] = 32, expected 21
        [00:00:00.014,833] <err> app: FAIL: rx[1] = 33, expected 213
        [00:00:00.014,838] <err> app: FAIL: rx[2] = 34, expected 10
        [01:08:04.435,863] <inf> app: Run 0 - FAILED
6. HOST sleeps for 500 ms.

Detailed test scenario for DEVICE:
1. When HOST requests write to the DEVICE, DEVICE prepares for data reception.
2. After HOST writes data to the DEVICE, DEVICE verifies that byte[i] is equal to (byte[i-1] + DELTA).
3. DEVICE reports status of data validation:
   Success:
        [00:00:49.424,665] <inf> app: Run 1 - PASS
   Failure:
        [00:00:00.899,520] <err> app: FAIL: rx[0] = 80, expected 1
        [00:00:00.899,543] <inf> app: Run 0 - FAILED
4. When HOST requests read from the DEVICE, DEVICE prepares previously received data to be send back to HOST.
5. After HOST reads data from the DEVICE, DEVICE clears contents of its TWIS data buffer.

Two connections between two boards are needed.
GPIO numbers can be found in the overlay file for the target.
HOST:        DEVICE:
TWIM_SCL  -  TWIS_SCL
TWIM_SDA  -  TWIS_SDA

When nrf54l15dk is HOST and nrf54h20dk is DEVICE then:
      nrf54l      nrf54h
SCL:  P1.12   -   P1.03
SDA:  P1.08   -   P2.09

When nrf54l15dk is DEVICE and nrf54h20dk is HOST then:
      nrf54l      nrf54h
SCL:  P1.13   -   P1.02
SDA:  P1.09   -   P2.08

Note:
1. First transaction after power on:
   - is always failed on the DEVICE,
   - shall be PASS on HOST.
2. Timestamp and run counter is local to board:
   - they are not synchronized between HOST and DEVICE;
   - values are zeroed when board is reset;
