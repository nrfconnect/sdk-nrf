.. _gs_installing_forking:

Forking the |NCS| repositories
##############################

If you want to change any of the code that is provided by the |NCS| and contribute this updated code back to the |NCS|, you should create your own forks of the |NCS| repositories and clone these forks instead of the base repositories.

To fork and clone the repositories, complete the following steps:

1. Fork the four |NCS| repositories to your own GitHub account by clicking the **Fork** button in the upper right-hand corner of each repository page:

   a. |ncs_zephyr_repo|
   #. |ncs_repo|
   #. |ncs_mcuboot_repo|
   #. |ncs_nrfxlib_repo|

#. Create a folder named ``ncs``.
   This folder will hold all |NCS| repositories.
#. Open a GIT bash or terminal window in the ``ncs`` folder.
#. Clone your forks by entering the following commands, where *<username>* must be replaced with your GitHub user name:

   .. code-block:: console

      git clone https://github.com/<username>/fw-nrfconnect-zephyr.git zephyr

      git clone https://github.com/<username>/fw-nrfconnect-mcuboot.git mcuboot

      git clone https://github.com/<username>/fw-nrfconnect-nrf.git nrf

      git clone https://github.com/<username>/nrfxlib.git nrfxlib

#. To link your local repositories to the |NCS| repositories, add remotes that point to the original repositories:

   .. code-block:: console

      cd zephyr
      git remote add ncs https://github.com/NordicPlayground/fw-nrfconnect-zephyr.git

      cd ../mcuboot
      git remote add ncs https://github.com/NordicPlayground/fw-nrfconnect-mcuboot.git

      cd ../nrf
      git remote add ncs https://github.com/NordicPlayground/fw-nrfconnect-nrf.git

      cd ../nrfxlib
      git remote add ncs https://github.com/NordicPlayground/nrfxlib.git

#. Optionally, add remotes to the upstream repositories for Zephyr and Mcuboot:

   .. code-block:: console

      cd ../zephyr
      git remote add upstream https://github.com/zephyrproject-rtos/zephyr.git

      cd ../mcuboot
      git remote add upstream https://github.com/runtimeco/mcuboot.git

.. include:: gs_ins_windows.rst
   :start-after: dirstructure_start
   :end-before: dirstructure_end

To be able to build |NCS| applications for nRF91, check out branch ``nrf91`` in the fw-nrfconnect-zephyr repository:

   .. code-block:: console

	cd ../zephyr
	git checkout nrf91

Pull requests related to nRF91 (for example, for nRF9160 SiP or board support, drivers, and so on) in the fw-nrfconnect-zephyr repository must be submitted against the ``nrf91`` branch.

.. note::
   The ``nrf91`` branch is temporary.
   Code changes to support the nRF9160 SiP and the nRF9160 DK board will be submitted upstream to the official Zephyr repository shortly after the v0.3.0 release.
