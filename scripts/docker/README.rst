.. _nrf_toolchain_docker_image:

|NCS| toolchain Docker image
############################

.. contents::
   :local:
   :depth: 2

The image is based on Linux - Ubuntu 22.04 Docker image with the following additional tools:

* Git
* |NCS| toolchain bundle, which includes `nRF Util`_ and the ``device`` command
* ``toolchain-manager`` command in nRF Util
* SEGGER J-Link |jlink_ver| (installation requires accepting SEGGER License)

Building image
**************

To build a Docker image, you have to provide a ``VERSION`` argument containing the ID of the Linux toolchain bundle you want to install.
You can use `print_toolchain_checksum.sh`_ to get ID proper for your |NCS| revision.

.. code-block:: shell

    TOOLCHAIN_ID=$(../print_toolchain_checksum.sh)
    docker build --build-arg VERSION=$TOOLCHAIN_ID .

Accepting J-Link license
************************

To install and use J-Link software you have to accept `JLink License Agreement`_.

If the container is run in an interactive mode, you will be asked to accept the license at the start of the container.

You can also start the container with the ``ACCEPT_JLINK_LICENSE=1`` environment variable, and J-Link will be installed without prompting.

During post installation steps, J-Link configures ``udev`` roles.
This operation requires running a container with the ``--privileged`` flag to grant permissions needed for the container.

Running container
*****************

Toolchain environment variables are set only in Bash sessions.
Outside the Bash session, these variables are not set, and tools delivered in the toolchain bundle cannot be found on the path.
|NCS| toolchain image uses ``bash`` command as entry point.
If the entry point is not overwritten, you can start your Docker container in both interactive and non-interactive mode.

Locally
=======

To run the |NCS| toolchain container to build and flash HEX files, run the following command.

.. code-block:: none

    docker run -ti ghcr.io/nrfconnect/sdk-nrf-toolchain:<TAG> -v /dev:/dev --privileged -e ACCEPT_JLINK_LICENSE=1 bash

.. tip::

   You can also add the ``-v <PATH-TO-NCS-PROJECT-ON-DOCKER-HOST>:/ncs`` argument to the ``docker run`` command to mount the |NCS| project to the container.
   In such cases, you might encounter issues with repositories, ownership, and west project configuration.
   To resolve them, perform the following:

   * Run the ``git config --global --add safe.directory '*'`` command to mark mounted git repositories as trusted.
   * Run the ``west update`` command to import west commands delivered in the |NCS| project (for example, ``west build``)
   * Add ``--pristine`` argument to ``west build`` command to avoid path mismatch issues.

In CI
=====

CI systems usually run the Docker container in detached mode and can override the default entry point and command.
However, it is still possible to use the |NCS| toolchain images in pipelines.

GitHub Actions
--------------

GitHub action starts container with overwritten default entry point. That means you have to set ``bash`` as a default shell to load all required environment variables.

If the container is started with the ``ACCEPT_JLINK_LICENSE`` env variable set to ``1``, J-Link will be installed just before the first bash command is executed.

Example job configuration:

.. code-block:: yaml

    jobs:
      build-in-docker:
        runs-on: ubuntu-22.04
        container:
          image: ghcr.io/nrfconnect/sdk-nrf-toolchain:<TAG>  # steps in this job are executed inside sdk-nrf-toolchain container
          env:
            ACCEPT_JLINK_LICENSE: 1 # set if you want to install JLink
        defaults:
          run:
            shell: bash  # It is required to set `bash` as default shell
        steps:
          - name: Run command in NCS toolchain environment
            run: "west --version"  # This command is executed in bash shell `docker exec <container> bash -c west --version`
            # It will also install JLink if ACCEPT_JLINK_LICENSE is set to 1
