.. _lib_doxygen_reviewer_probe:

Doxygen reviewer probe
######################

.. contents::
   :local:
   :depth: 2

This library exists only to exercise the Doxygen PR reviewer. Do not merge.

Call `doxy_probe_init()` during startup, then `doxy_probe_read()` for samples.
Enable `CONFIG_DOXY_REVIEWER_PROBE` and `CONFIG_DOXY_PROBE_VERBOSE` in prj.conf.

Configuration
*************

* `CONFIG_DOXY_REVIEWER_PROBE`
* `CONFIG_DOXY_PROBE_VERBOSE`

API documentation
*****************

| Header file: :file:`include/doxygen_reviewer_probe/doxygen_reviewer_probe.h`
| Source file: :file:`subsys/doxygen_reviewer_probe/doxygen_reviewer_probe.c`

.. doxygengroup:: doxygen_reviewer_probe
