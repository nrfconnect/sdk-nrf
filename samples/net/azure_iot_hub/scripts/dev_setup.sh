#!/bin/bash

err_trap() {
    echo -e "\033[0;31mERROR: exitcode $? on line $1\033[0m"
    exit 1
}

trap 'err_trap $LINENO' ERR

cert_tool.py() {
    python3 $ZEPHYR_BASE/../nrf/scripts/cert_tool.py $@
}

wrap_in_quotes() {
    file=$1
    if [[ -f $file ]]; then
        if grep -qe "^\-\-\-\-\-BEGIN" $file; then
            sed -i'.org' 's/.*/"&\\n"/' $file
        fi
    else
        echo "Error: File not found: $file"
        exit 1
    fi
}

device=""
common_name=""
family=""
key_method="modem"
setup_dps=0

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -d|--device)
            device="$2"
            if [[ ! -e $device ]]; then
                echo "Error: Device not found: $device"
                exit 1
            fi
            shift 2
            ;;

        -f|--family)
            family="$2"
            if [[ $family != "nrf91" && $family != "nrf70" ]]; then
                echo "Invalid family: $family. Valid values are 'nrf91' or 'nrf70'"
                exit 1
            fi
            shift 2
            ;;

        -km|--key-method)
            key_method="$2"
            if [[ $key_method != "modem" && $key_method != "script" ]]; then
                echo "Invalid key method: $key_method. Valid values are 'modem' or 'script'"
                exit 1
            fi
            shift 2
            ;;

        -cn|--common-name)
            common_name="$2"
            shift 2
            ;;

        --use-dps)
            setup_dps=1
            shift 1
            ;;

        *|-h|--help)
            echo "Usage: $0 -d|--device <device> -f|--family <nrf91|nrf70> [options]"
            echo "Required arguments:"
            echo "  -d, --device <device>             Path to the device serial port (only nrf91)"
            echo "  -f, --family <nrf91|nrf70>        nRF family"
            echo "Optional arguments:"
            echo "  -km, --key-method <modem|script>  Key generation method"
            echo "  -cn, --common-name <common_name>  Common name for the device certificate"
            echo "  --use-dps                         Setup device for DPS"

            exit 0
            ;;
    esac
done

if [[ -z $family ]]; then
    echo "Error: Missing family argument"
    exit 1
fi

if [[ $family == "nrf91" && -z $device ]]; then
    echo "Error: Missing device argument"
    exit 1
fi

if [[ $family == "nrf70" ]]; then
    key_method="script"
fi

if [[ $key_method == "script" && -z $common_name ]]; then
    echo "Error: Common name is required when using script key method"
    exit 1
fi

echo Azure Config:
echo location: $AZ_LOCATION
echo resource group: $AZ_RGROUP
echo IoT Hub Name: $AZ_HUB
echo CA Cert Name: $AZ_CA_CERT
echo Common Name: $common_name
echo nrf Device: $device
echo nrf Family: $family
echo Sec-tag: $SEC_TAG
echo Secondary Sec-tag: $SEC_TAG2
echo Key method: $key_method

if [[ -z $AZ_LOCATION || -z $AZ_RGROUP || -z $AZ_HUB || \
      -z $AZ_CA_CERT || -z $SEC_TAG || -z $SEC_TAG2 ]]; then
    echo "Please set the environment variables for the Azure IoT Hub"
    echo "AZ_LOCATION, AZ_RGROUP, AZ_HUB, AZ_CA_CERT, SEC_TAG, SEC_TAG2"
    echo "Usage: $0 -d|--device <device> [-km|--key-method <modem|script>] "\
         "[-cn|--common-name <common_name>] [--register-device-id]"
    exit 1
fi

if [[ $family == "nrf70" && $key_method == "modem" ]]; then
    echo "Error: Modem key method is not supported for nRF70 family"
    exit 1
fi

if [[ $family == "nrf91" ]]; then
    if ! nrfcredstore $device list; then
        echo "Error: nrfcredstore failed. Please check that the device is connected,"
        echo "flashed with at_client and no other serial terminals are connected."
        exit 1
    fi
fi

if [[ $family == "nrf91" ]]; then
    existing_keys=$(nrfcredstore $device list --tag $SEC_TAG | tail -n +2 | wc -l)
    echo "Existing keys in sec-tag $SEC_TAG: $existing_keys"
    if [[ $existing_keys -gt 0 ]]; then
        echo "Error: sec-tag $SEC_TAG is not empty"
        echo "Please delete the existing keys first"
        exit 1
    fi

    existing_keys=$(nrfcredstore $device list --tag $SEC_TAG2 | tail -n +2 | wc -l)
    echo "Existing keys in sec-tag $SEC_TAG2: $existing_keys"
    if [[ $existing_keys -gt 0 ]]; then
        echo "Error: sec-tag $SEC_TAG2 is not empty"
        echo "Please delete the existing keys first"
        exit 1
    fi
fi

mkdir -p certs

if [[ $key_method == "modem" && $family == "nrf91" ]]; then
    echo "Generating keys using modem"

    if [[ -z $common_name ]]; then
        nrfcredstore $device generate $SEC_TAG certs/client-csr.der
    else
        nrfcredstore $device generate $SEC_TAG certs/client-csr.der --attributes "CN=$common_name"
    fi

    openssl req -inform DER -in certs/client-csr.der -outform PEM -out certs/client-csr.pem

    cert_tool.py sign
elif [[ $key_method == "script" ]]; then
    echo "Generating keys using script"
    cert_tool.py client_key
    cert_tool.py csr --common-name $common_name
    cert_tool.py sign

    if [[ $family == "nrf91" ]]; then
        nrfcredstore $device write $SEC_TAG CLIENT_KEY certs/private-key.pem
    fi
fi

subject=$(openssl x509 -in certs/client-cert.pem -noout -subject)
cn_from_cert=$(echo $subject | sed -n 's/.*CN *= *\([^,]*\).*/\1/p')
echo "Common Name from the device certificate: $cn_from_cert"
if [[ -z $cn_from_cert ]]; then
    echo "Error Common Name from the device certificate is empty"
    exit 1
fi

# the full-chain certificate is required for group enrollment to work
cat certs/client-cert.pem ca/sub-ca-cert.pem ca/root-ca-cert.pem > certs/full-chain.pem
if [[ $family == "nrf91" ]]; then
    nrfcredstore $device write $SEC_TAG CLIENT_CERT certs/full-chain.pem
elif [[ $family == "nrf70" ]]; then
    mv certs/client-cert.pem certs/client-cert.pem.org
    cp certs/full-chain.pem certs/client-cert.pem
fi

if [[ $setup_dps -eq 0 ]]; then
    az iot hub device-identity create -n $AZ_HUB -d $cn_from_cert --am x509_ca
fi

wget https://cacerts.digicert.com/DigiCertGlobalRootG2.crt.pem
wget https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem

if [[ $family == "nrf91" ]]; then
    nrfcredstore $device write $SEC_TAG ROOT_CA_CERT DigiCertGlobalRootG2.crt.pem
    nrfcredstore $device write $SEC_TAG2 ROOT_CA_CERT BaltimoreCyberTrustRoot.crt.pem
elif [[ $family == "nrf70" ]]; then
    mv DigiCertGlobalRootG2.crt.pem certs/ca-cert.pem
    mv BaltimoreCyberTrustRoot.crt.pem certs/ca-cert-2.pem
fi

echo "Azure IoT Hub setup complete"
echo "Please update the project configuration:"
echo CONFIG_MQTT_HELPER_SEC_TAG=$SEC_TAG
echo CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG=$SEC_TAG2

if [[ $family == "nrf70" ]]; then
    echo "To automatically provision certificates from the certs/ folder:"
    echo CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES=y
fi

if [[ $setup_dps -eq 1 ]]; then
    id_scope=$(az iot dps show --name $AZ_DPS_NAME --resource-group $AZ_RGROUP \
                --query "properties.idScope" --output tsv)
    echo CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE=\"$id_scope\"
fi

if [[ -z $common_name ]]; then
    if [[ $family == "nrf91" ]]; then
        echo CONFIG_MODEM_JWT=y
        echo CONFIG_AZURE_IOT_HUB_SAMPLE_DEVICE_ID_USE_HW_ID=y
        echo CONFIG_HW_ID_LIBRARY_SOURCE_UUID=y
    elif [[ $family == "nrf70" ]]; then
        echo HW_ID_LIBRARY_SOURCE_NET_MAC=y
    fi
else
    echo CONFIG_AZURE_IOT_HUB_DPS_REG_ID=\"$common_name\"
fi

