# Isochronous channels and ACL tests

These tests are made to verify BIS and ACL simultaneous data transmission.
Compared to other tests, these run as integration tests involving both the controller, host and application layer.

These tests are run as part of nRF Connect SDK CI with specific configurations.
They should not be treated as a reference for general use, as they are designed for this CI only and fault injection purposes.

## Requirements

The tests support the `nrf5340_audio_dk` and 5340 bsim target.

Tests can be run in parallel by being in the current tests folder, such as bsim/tests and running <userpath>/zephyr/tests/bsim/run_parallel.sh from that location.
