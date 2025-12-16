This test was initially dedicated to nrf54ls05b which has SWD and GPIO shared on the same package pin:
- SWDIO at P1.29,
- SWDCLK at P1.30.

When `tampc` node has property `swd-pins-as-gpios` then SWD functionality shall be disabled
some time after the reset and package pin shall behave as an ordinary GPIO.

SWD interface can be temporarily enabled with
nrfutil device x-gpio-swd-muxing-disable [--serial-number ${SEGGER_ID}]

Test code checks that:
 - selected GPIO can be configured as input;
 - selected GPIO can be configured as output;
 - when pin is configured as output HIGH, reading state of the pin returns HIGH;
 - when pin is configured as output LOW, reading state of the pin returns LOW;

PyTest checks that:
 - console contains confirmation that ztest completed successfully,
 - west attach doesn't work (SWD interface is disabled),
 - nrfutil device core-info fails,
 - nrfutil device x-gpio-swd-muxing-disable executes correctly,
 - nrfutil device core-info returns chip specific data.
