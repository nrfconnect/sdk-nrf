.. _gs_installing_forking:

Forking the |NCS| repositories
##############################

If you want to change any of the code that is provided by the |NCS| and contribute this updated code back to the |NCS|, you should create your own forks of the |NCS| repositories.

To fork the repositories and update your :ref:`cloned repositories<cloning_the_repositories_win>` to use your fork, complete the following steps:

1. Fork the four |NCS| repositories to your own GitHub account by clicking the **Fork** button in the upper right-hand corner of each repository page:

   a. |ncs_repo|
   #. |ncs_zephyr_repo|
   #. |ncs_mcuboot_repo|
   #. |ncs_nrfxlib_repo|

#. Open a Git bash or terminal window in the ``ncs`` folder.
#. For each of the four repository folders, complete the following steps, replacing *Folder* with the respective folder name (``nrf``, ``zephyr``, ``mcuboot``, or ``nrfxlib``):

   a. Enter the respective folder:

      .. parsed-literal::
         :class: highlight

         cd *Folder*
   #. For ``nrf``, rename the default remote that points to the upstream |NCS| repository from ``origin`` to ``ncs``::

         git remote rename origin ncs

      You do not need to do this for the other repositories, because their default remotes are already named ``ncs``.
   #. Add the fork that you created as default remote with the name ``origin``.
      To do so, enter one of the following commands, replacing *Username* with your GitHub user name:

      For nrf:
         .. parsed-literal::
            :class: highlight

            git remote add origin https\://github.com/*Username*/fw-nrfconnect-nrf.git

      For zephyr:
         .. parsed-literal::
            :class: highlight

            git remote add origin https\://github.com/*Username*/fw-nrfconnect-zephyr.git

      For mcuboot:
         .. parsed-literal::
            :class: highlight

            git remote add origin https\://github.com/*Username*/fw-nrfconnect-mcuboot.git

      For nrfxlib:
         .. parsed-literal::
            :class: highlight

            git remote add origin https\://github.com/*Username*/nrfxlib.git
   #. Optionally, for ``zephyr`` and ``mcuboot``, add remotes for the upstream repositories:

      For zephyr:
         .. parsed-literal::
            :class: highlight

            git remote add upstream https\://github.com/zephyrproject-rtos/zephyr.git

      For mcuboot:
         .. parsed-literal::
            :class: highlight

            git remote add upstream https\://github.com/JuulLabs-OSS/mcuboot.git
   #. Verify the remote repositories by entering the following command::

        git remote -v

      The output should look similar to this example for zephyr:

      .. parsed-literal::
         :class: highlight

         ncs     https\://github.com/NordicPlayground/fw-nrfconnect-zephyr (fetch)
         ncs     https\://github.com/NordicPlayground/fw-nrfconnect-zephyr (push)
         origin   https\://github.com/*Username*/fw-nrfconnect-zephyr (fetch)
         origin   https\://github.com/*Username*/fw-nrfconnect-zephyr (push)
         upstream https\://github.com/zephyrproject-rtos/zephyr (fetch)
         upstream https\://github.com/zephyrproject-rtos/zephyr (push)

   #. Go back to the ``ncs`` folder::

         cd ..

You can now work on your local fork (``origin``), but also fetch and check out branches from ``ncs`` and ``upstream``.
