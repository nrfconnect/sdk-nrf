#!/bin/sh

# Install JLink if license was accepted and JLink does not exist
if [ "${ACCEPT_JLINK_LICENSE}" = "1" ] ; then
  echo "License accepted, Jlink will be installed"
  dpkg -i /jlink/JLink_Linux.deb
fi
