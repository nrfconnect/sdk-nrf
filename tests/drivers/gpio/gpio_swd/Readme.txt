This test was initially dedicated to nrf54ls05b which has SWD and GPIO shared on the same package pin:
- SWDIO at P1.29,
- SWDCLK at P1.30.

SWD functionality shall be disabled some time after the reset and package pin shall behave as an ordinary GPIO.
Currently, this is blocked by lack of support in MDK - to be added in MDK 8.73.0.
Missing implementation, details available in NRFX-8363.

Test checks that:
 - selected GPIO can be configured as input;
 - selected GPIO can be configured as output;
 - when pin is configured as output HIGH, reading state of the pin returns HIGH;
 - when pin is configured as output LOW, reading state of the pin returns LOW;
