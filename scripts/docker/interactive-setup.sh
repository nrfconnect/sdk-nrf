# Run this only once
if [ ! -f /jlink/license_processed ] && [ ! -f /usr/bin/JLinkExe ] ; then
  # Do not ask if shell is not interactive
  if [ ! -z "$PS1" ] ; then
    echo "To install JLink you have to accept following license"
    printf '\n'
    cat '/jlink/license.txt'
    printf '\n'
    while true; do
      read -p "Do you accept these Terms of Use (y/n)? " yn
      case $yn in
          [Yy]* ) export ACCEPT_JLINK_LICENSE=1; break;;
          [Nn]* ) export ACCEPT_JLINK_LICENSE=0; break;;
          * ) echo "Invalid input. Please use y/n";;
      esac
    done
  fi
  if [[ "${ACCEPT_JLINK_LICENSE}" == "1" ]]; then
    /jlink/install.sh
  fi
  touch /jlink/license_processed
fi
