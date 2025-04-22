In this test suite instance of the MSPI is connected together with two peripherals.
One emulated SPI slave peripheral and one onboard QSPI memory.
MSPI instance works as a controller, SPIS is configured as a slave peripheral.
In each test, both instances get identical configuration (CPOL, CPHA, bitrate, etc.).

Four GPIO loopbacks are required (see overlay for nrf54l15dk for reference):
1. MSPI_SCK connected with spi21-SPIS_SCK,
2. MSPI_DQ0 connected with spi21-SPIS_MOSI,
3. MSPI_DQ1 connected with spi21-SPIS_MISO,
4. MSPI_CS1 connected with spi21-SPIS_CSN.
