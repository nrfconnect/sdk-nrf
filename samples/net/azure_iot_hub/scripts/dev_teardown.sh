#!/bin/bash

device=""
common_name=""
family=""

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

        -cn|--common-name)
            common_name="$2"
            shift 2
            ;;

        *|-h|--help)
            echo "Usage: $0 -d|--device <device> -f|--family <nrf91|nrf70> [options]"
            echo "Required arguments:"
            echo "  -d, --device <device>             Path to the device serial port (only nrf91)"
            echo "  -f, --family <nrf91|nrf70>        Device family"
            echo "Options:"
            echo "  -cn, --common-name <common_name>  Common name for the device certificate"

            exit 0
            ;;
    esac
done

echo Azure Config:
echo resource group: $AZ_RGROUP
echo IoT Hub Name: $AZ_HUB
echo CA Cert Name: $AZ_CA_CERT
echo DPS Name: $AZ_DPS_NAME
echo Enrollment Group Name: $AZ_ENROLLMENT_GROUP
echo nrf91 Device: $device
echo Sec-tag: $SEC_TAG
echo Secondary Sec-tag: $SEC_TAG2

if [[ -z $device && $family == "nrf91" ]]; then
    echo "Error: Missing device argument"
    exit 1
fi

echo -e "This script will delete device related resources from azure, the certs folder and " \
        "key/cert in the modem"

read -p "Do you want to continue? (y/n) " -n 1 -r
echo   # move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborting"
    exit 1
fi

if [[ $family == "nrf91" ]]; then
    if ! nrfcredstore $device list; then
        echo "Error: nrfcredstore failed. Is device connected and flashed with at_client?"
        exit 1
    fi

    # delete existing keys in SEC_TAG and SEC_TAG2
    for cert in CLIENT_KEY CLIENT_CERT ROOT_CA_CERT; do
        for tag in $SEC_TAG $SEC_TAG2; do
            existing_key=$(nrfcredstore $device list --tag $tag --type $cert | tail -n +2 | wc -l)
            if [[ $existing_key -gt 0 ]]; then
                echo "Deleting $cert from $tag"
                nrfcredstore $device delete $tag $cert
            fi
        done
    done
fi

if [[ -n $common_name ]]; then
    echo Deleting device identity and dps enrollment for \"$common_name\"
    az iot hub device-identity list --hub-name $AZ_HUB --resource-group $AZ_RGROUP \
        --query "[?deviceId=='$cn_from_cert'].deviceId" --output tsv | \
        xargs -I {} az iot hub device-identity delete --hub-name $AZ_HUB \
        --resource-group $AZ_RGROUP --device-id {}

    enrollment_id=$(az iot dps enrollment list --dps-name $AZ_DPS_NAME \
        --resource-group $AZ_RGROUP \
        --query "[?registrationId=='$cn_from_cert'].registrationId" --output tsv)
    if [[ -n $enrollment_id ]]; then
        az iot dps enrollment delete --enrollment-id $enrollment_id \
            --dps-name $AZ_DPS_NAME --resource-group $AZ_RGROUP
    fi
else
    echo "Common name not provided."
    read -p "Do you want to delete all device identities and dps enrollments? (y/n) " -n 1 -r
    echo   # move to a new line
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        az iot hub device-identity list --hub-name $AZ_HUB --resource-group $AZ_RGROUP \
            --query "[].deviceId" --output tsv | \
            xargs -I {} az iot hub device-identity delete --hub-name $AZ_HUB \
            --resource-group $AZ_RGROUP --device-id {}

        az iot dps enrollment list --dps-name $AZ_DPS_NAME --resource-group $AZ_RGROUP \
            --query "[].registrationId" --output tsv | \
            xargs -I {} az iot dps enrollment delete --enrollment-id {} --dps-name $AZ_DPS_NAME \
            --resource-group $AZ_RGROUP
    fi
fi

rm -rf certs

if [[ -f BaltimoreCyberTrustRoot.crt.pem ]]; then
    rm BaltimoreCyberTrustRoot.crt.pem*
fi

if [[ -f DigiCertGlobalRootG2.crt.pem ]]; then
    rm DigiCertGlobalRootG2.crt.pem*
fi
