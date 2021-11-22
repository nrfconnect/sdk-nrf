if [ $# -eq 0 ]
  then
    echo "No arguments supplied. Run as: create_key.sh <device-id>."
    exit 1
fi
DEVICE_ID=$1
openssl ecparam -genkey -name prime256v1 -noout -out ${DEVICE_ID}-ec_private.pem
openssl ec -in ${DEVICE_ID}-ec_private.pem -pubout -out ${DEVICE_ID}-ec_public.pem
