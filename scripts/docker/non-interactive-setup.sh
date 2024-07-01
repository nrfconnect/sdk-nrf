#! /bin/bash

# Set toolchain environemnt
if [ -z "${ZEPHYR_SDK_INSTALL_DIR}" ]; then
  source /opt/toolchain-env.sh
fi

# Install JLink if it is not installed and license was accepted
if [[ "${ACCEPT_JLINK_LICENSE}" == "1" ]] && [ ! -f /usr/bin/JLinkExe ] ; then
  /jlink/install.sh
fi
