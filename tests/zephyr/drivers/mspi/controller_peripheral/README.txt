In this test suite instance of the MSPI is connected together with two peripherals.
One emulated SPI slave peripheral and one onboard QSPI memory.
MSPI instance works as a controller, SPIS is configured as a slave peripheral.
In each test, both instances get identical configuration (CPOL, CPHA, bitrate, etc.).

Four GPIO loopbacks are required (see overlay for nrf54l15dk for reference):
1. MSPIM_SCK connected with spi21-SPIS_SCK,
2. MSPIM_MISO connected with spi21-SPIS_MISO,
3. MSPIM_MOSI connected with spi21-SPIS_MOSI,
4. MSPI_CS1 gpio connected with spi21-SPIS_CSN.
