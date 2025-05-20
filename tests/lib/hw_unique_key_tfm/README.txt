Hardware Unique Key + TFM test
------------------------------

This flashes the Key Management Unit (KMU), then tests that calling the PSA crypto key derivation API gives the correct result.

Flashing the KMU needs a special since registers must be written in a specific order.

1. Write the KMU slot number to NRF_KMU->SELECTKEYSLOT
2. Write the key and destination (push target) address to the KMU
3. Write the permissions to the KMU

The flashing algorithm is run at the end of the build, not when west flash is called, since there is no hook in west flash to run it.
This means that the SEGGER id of the development kit must be passed both to west build and west flash.

Here is an example of passing the id to the build:

  west build -b nrf9160dk/nrf9160/ns -- -DCONFIG_HUK_TEST_BOARD_SNR=\"901234567\"
