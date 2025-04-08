High-Performance Framework GPIO 2-Pin Test
##########################################

This application tests the HPF GPIO subsystem using a hardware configuration
where two GPIOs are directly wired together. The test pins are
identified through a test-specific devicetree binding in the `dts/`
subdirectory, implemented for specific boards by overlay files in the
`boards/` directory.

Only boards for which an overlay is present can pass this test.  Boards
without an overlay, or for which the required wiring is not provided,
will fail with an error like this:

    Validate device GPIO_0
    Check HPF GPIO output 10 connected to GPIO_1 input 14
    FATAL output pin not wired to input pin? (out high => in low)

No special build options are required to make use of the overlay.
