#!/bin/bash

delete_hub=1
delete_dps=1

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --keep-hub)
            delete_hub=0
            shift 1
            ;;
        --keep-dps)
            delete_dps=0
            shift 1
            ;;
        *|-h|--help)
            echo "Usage: $0 [--keep-hub] [--keep-dps]"
            echo "Options:"
            echo "  --keep-hub       Keep IoT Hub"
            echo "  --keep-dps       Keep DPS instance"
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

echo -e "This script will delete the above IoT Hub, resource group and related."

read -p "Do you want to continue? (y/n) " -n 1 -r
echo   # move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborting"
    exit 1
fi

if [[ $delete_dps -eq 1 ]]; then
    dps_id=$(az iot dps list --resource-group $AZ_RGROUP --query "[?name=='$AZ_DPS_NAME'].id" \
        --output tsv)
    if [[ -n $dps_id ]]; then
        echo "Deleting DPS instance: $AZ_DPS_NAME"
        az iot dps delete --name $AZ_DPS_NAME --resource-group $AZ_RGROUP
    fi
fi

if [[ $delete_hub -eq 1 ]]; then
    hub_id=$(az iot hub show --name $AZ_HUB --query id --output tsv)
    if [[ -n $hub_id ]]; then
        echo "Deleting IoT Hub with id: $hub_id"
        az iot hub delete --ids $hub_id
    fi

    rg_id=$(az group show --name $AZ_RGROUP --query id --output tsv)
    if [[ -n $rg_id ]]; then
        echo "Deleting resource group with id: $rg_id"
        echo "This can take a few minutes"
        az group delete --name $AZ_RGROUP --yes
    fi
fi

rm -rf ca
