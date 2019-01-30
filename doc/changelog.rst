.. _changelog:

Change log
##########

This change log contains information about the current state of the |NCS| repositories.

For information about stable releases, see the release notes for the respective release.

.. important::
   The |NCS| follows `Semantic Versioning 2.0.0`_.
   A "99" in the version number of this documentation indicates continuous updates on the master branch since the previous major and minor release.
   There might be additional changes on the master branch that are not listed here.

   This changelog only shows information that is different from the information in the latest release notes.


Repositories
************
.. list-table::
   :header-rows: 1

   * - Component
     - Branch
   * - `fw-nrfconnect-nrf <https://github.com/NordicPlayground/fw-nrfconnect-nrf>`_
     - master
   * - `nrfxlib <https://github.com/NordicPlayground/nrfxlib>`_
     - master
   * - `fw-nrfconnect-zephyr <https://github.com/NordicPlayground/fw-nrfconnect-zephyr>`_
     - master
   * - `fw-nrfconnect-mcuboot <https://github.com/NordicPlayground/fw-nrfconnect-mcuboot>`_
     - master


Major changes
*************

The following list contains the most important changes since the last release:

* All updates needed to develop for the nRF9160 SiP have been merged to the master branch of the fw-nrfconnect-zephyr repository.
  The nrf91 branch will therefore be deleted in the near future.



The following documentation has been added or updated:

* :ref:`bt_conn_ctx_readme` (added)
* :ref:`dk_buttons_and_leds_readme` (added)
* :ref:`asset_tracker` (updated)
