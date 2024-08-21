.. _nrf54h20-psa-rng:

nRF54H20 PSA RNG snippet (nrf54h20-psa-rng)
###############################

Overview
********

This snippet allows using the PSA RNG with nRF54H20. This will
allow using the random number generator in CRACEN through
the Zephyr random APIS. In the future this snippet will be removed
and PSA RNG will be the default option that does't require any
special configuration but we need to wait until the Secure domain
firmware with the PSA SSF server enabled is released and used
by everyone.
