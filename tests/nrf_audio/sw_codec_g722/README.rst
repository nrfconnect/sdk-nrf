G.722 codec selection tests
##########################

Run from the SDK workspace with the configured Zephyr toolchain environment::

   west twister -T nrf/tests/nrf_audio/sw_codec_g722 -p native_sim/native/64

Check the G.722 Kconfig dependencies with the SDK's Python environment::

   PYTHONPATH=zephyr/scripts/kconfig python nrf/tests/nrf_audio/sw_codec_g722/test_kconfig.py

This evaluates the actual G.722 selection stanza with isolated prerequisite
symbols. It checks native 16 kHz and converted 48 kHz, requires conversion at
48 kHz, and rejects a 32-bit converter at 48 kHz. Native 16 kHz bypasses conversion.
Full application configuration/build checks are still needed to validate
integration with the remaining Kconfig choices.

The tests compile the real codec selector, G.722 wrapper and library, channel modifier,
sample rate converter and unicast preset store. Linker wrappers inject failures
at codec context creation and converter initialization without replacing codec
processing. AddressSanitizer checks the native test execution.

The two configurations cover normal routing and mono-to-all-receivers duplication.
Tests check frame sizes, reference encoder continuity, sparse locations,
zero-location mono duplication and loss-mask handling, stereo interleaving,
partial stereo aggregates with a missing stream, 48/16 kHz conversion, exact
silence after loss, allocation rollback,
direction-specific lifecycle, malformed LTVs, vendor identity and preset adaptation.
These are not hardware timing, long-duration soak or reference-vector conformance
tests. Bluetooth profile checks use application helpers rather than simulating
remote ASCS transactions.
