#! /bin/bash

if [ -z "${ZEPHYR_SDK_INSTALL_DIR}" ]; then
  source /opt/toolchain-env.sh
fi
exec "$@"
