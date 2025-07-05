#!/usr/bin/env bash
set -e

cd ncs
OUT_FILE="nrf/scripts/requirements-fixed.txt"
echo "Writing frozen requirements to: $OUT_FILE"
echo "Log python version: $(python --version)"

TOPDIR=$(west topdir)
cd "$TOPDIR"

source ~/.local/bin/virtualenvwrapper.sh

rmvirtualenv pip-fixed-venv > /dev/null 2>&1
mkvirtualenv pip-fixed-venv > /dev/null 2>&1 || true
workon pip-fixed-venv > /dev/null 2>&1

pip3 install \
  --index-url https://files.nordicsemi.com/artifactory/api/pypi/nordic-pypi/simple \
  pip-compile-cross-platform==1.4.2+nordic.3 --upgrade > /dev/null 2>&1

pip-compile-cross-platform \
  bootloader/mcuboot/scripts/requirements.txt \
  zephyr/scripts/requirements.txt \
  nrf/scripts/requirements-west-ncs-sbom.txt \
  nrf/scripts/requirements-actions.in \
  nrf/scripts/requirements-extra.txt \
  nrf/scripts/requirements.txt \
  --output-file nrf/scripts/requirements-fixed.txt \
  --min-python-version 3.12 \
  --generate-hashes
