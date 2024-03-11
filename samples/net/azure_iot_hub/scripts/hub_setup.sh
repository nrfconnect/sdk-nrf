#!/bin/bash

err_trap() {
    echo -e "\033[0;31mERROR: exitcode $? on line $1\033[0m"
    exit 1
}

trap 'err_trap $LINENO' ERR

cert_tool.py() {
    python3 $ZEPHYR_BASE/../nrf/scripts/cert_tool.py $@
}

# ensure internal field separators behave the same in zsh and bash (for az tsv output)
IFS=$' \t\n'

setup_hub=1
create_ca_certs=1
setup_dps=0

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --skip-hub)
            setup_hub=0
            shift 1
            ;;
        --skip-ca-certs)
            create_ca_certs=0
            shift 1
            ;;
        --use-dps)
            setup_dps=1
            shift 1
            ;;
        *|-h|--help)
            echo "Usage: $0 [--skip-hub] [--skip-ca-certs] [--use-dps]"
            echo "Options:"
            echo "  --skip-hub       Skip IoT Hub setup"
            echo "  --skip-ca-certs  Skip CA Certificates setup"
            echo "  --use-dps        Setup using DPS"
            exit 0
            ;;
    esac
done

echo Azure Config:
echo location: $AZ_LOCATION
echo resource group: $AZ_RGROUP
echo IoT Hub Name: $AZ_HUB
echo CA Cert Name: $AZ_CA_CERT

if [[ $setup_hub -eq 0 ]]; then
    echo "IoT Hub setup skipped"
fi

if [[ $create_ca_certs -eq 0 ]]; then
    echo "CA Certificate creation skipped"
fi

if [[ $setup_dps -eq 1 ]]; then
    echo DPS Name: $AZ_DPS_NAME
    echo Enrollment Group Name: $AZ_ENROLLMENT_GROUP
else
    echo "DPS setup skipped"
fi

if [[ $setup_hub -eq 1 ]]; then
    echo "Creating resource group"
    az group create --name $AZ_RGROUP --location $AZ_LOCATION

    for i in {1..10}; do
        if ! az group show --name $AZ_RGROUP; then
            echo "Resource group not found, retrying"
        else
            rgroup_status=$(az group show \
                --name $AZ_RGROUP \
                --query "properties.provisioningState" \
                --output tsv\
            )
            if [[ $rgroup_status == "Succeeded" ]]; then
                break
            fi
            echo "Waiting for resource group to be ready"
        fi
        sleep 10
    done

    echo "Creating IoT Hub"
    az iot hub create --resource-group $AZ_RGROUP --name $AZ_HUB --sku F1 --partition-count 2

    for i in {1..10}; do
        if ! az iot hub show --name $AZ_HUB --resource-group $AZ_RGROUP; then
            echo "IoT Hub not found, retrying"
        else
            hub_status=$(az iot hub show \
                --name $AZ_HUB \
                --resource-group $AZ_RGROUP \
                --query "properties.provisioningState" \
                --output tsv\
            )
            if [[ $hub_status == "Succeeded" ]]; then
                break
            fi
            echo "Waiting for IoT Hub to be ready"
        fi
        sleep 10
    done

    # TODO abort if retries exeeded
fi

if [[ $setup_dps -eq 1 ]]; then
    if ! az iot dps show --name $AZ_DPS_NAME --resource-group $AZ_RGROUP; then
        echo Create the DPS instance
        az iot dps create --name $AZ_DPS_NAME --resource-group $AZ_RGROUP
    else
        echo "DPS instance $AZ_DPS_NAME already exists"
    fi

    if ! az iot dps linked-hub show --dps-name $AZ_DPS_NAME --resource-group $AZ_RGROUP \
            --linked-hub $AZ_HUB; then
        echo Link the IoT Hub to the DPS instance
        az iot dps linked-hub create --dps-name $AZ_DPS_NAME --hub-name $AZ_HUB \
            --resource-group $AZ_RGROUP
    else
        echo "IoT Hub $AZ_HUB is already linked to DPS $AZ_DPS_NAME"
    fi
fi

if [[ $create_ca_certs -eq 1 ]]; then
    echo "Creating CA Certificates"
    cert_tool.py root_ca
    cert_tool.py sub_ca

    cat ca/sub-ca-cert.pem ca/root-ca-cert.pem > ca/sub-ca-chain.pem

    if [[ $setup_dps -eq 1 ]]; then
        echo Upload root CA cert to DPS
        cert_etag=$(az iot dps certificate create \
            --dps-name $AZ_DPS_NAME \
            --resource-group $AZ_RGROUP \
            --name $AZ_CA_CERT \
            --path ca/root-ca-cert.pem \
            --query etag \
            --output tsv\
        )

        echo Ask Azure for a verification code
        gen_code_res=$(az iot dps certificate generate-verification-code --dps-name $AZ_DPS_NAME \
            --resource-group $AZ_RGROUP --certificate-name $AZ_CA_CERT --etag $cert_etag \
            --query "to_array([properties.verificationCode,etag])" --output tsv)
        echo Result from generate-verification-code: $gen_code_res
        verification_code=$(echo $gen_code_res | cut -d ' ' -f 1)
        verify_etag=$(echo $gen_code_res | cut -d ' ' -f 2)
        echo Verification code: $verification_code
        echo Verify etag: $verify_etag

        echo Generate a new private key
        cert_tool.py client_key

        echo Create a CSR with the verification code as common name
        cert_tool.py csr --common-name $verification_code

        echo Sign the CSR with the root CA
        cert_tool.py sign_root

        echo Upload the verification certificate
        az iot dps certificate verify --dps-name $AZ_DPS_NAME --resource-group $AZ_RGROUP \
            --certificate-name $AZ_CA_CERT --etag $verify_etag --path certs/client-cert.pem
    else
        cert_etag=$(az iot hub certificate create --hub-name $AZ_HUB --name $AZ_CA_CERT \
            --path ca/root-ca-cert.pem --query etag --output tsv)

        echo Ask Azure for a verification code
        gen_code_res=$(az iot hub certificate generate-verification-code --hub-name $AZ_HUB \
            --name $AZ_CA_CERT --etag $cert_etag \
            --query "to_array([properties.verificationCode,etag])" --output tsv)
        echo Result from generate-verification-code: $gen_code_res
        verification_code=$(echo $gen_code_res | cut -d ' ' -f 1)
        verify_etag=$(echo $gen_code_res | cut -d ' ' -f 2)
        echo Verification code: $verification_code
        echo Verify etag: $verify_etag

        echo Generate a new private key
        cert_tool.py client_key

        echo Create a CSR with the verification code as common name
        cert_tool.py csr --common-name $verification_code

        echo Sign the CSR with the root CA
        cert_tool.py sign_root

        echo Upload the verification certificate
        az iot hub certificate verify --hub-name $AZ_HUB --name $AZ_CA_CERT --etag $verify_etag \
            --path certs/client-cert.pem
    fi
fi

hub_url=$(az iot hub show --name $AZ_HUB --query properties.hostName --output tsv)
if [[ $setup_dps -eq 1 ]]; then
    echo Create an enrollment group

    az iot dps enrollment-group create --dps-name $AZ_DPS_NAME --resource-group $AZ_RGROUP \
        --enrollment-id $AZ_ENROLLMENT_GROUP --certificate-path ca/sub-ca-cert.pem \
        --provisioning-status enabled --iot-hubs $hub_url --allocation-policy static
fi

echo "Azure IoT Hub setup complete"
echo "Please update the project configuration:"
echo CONFIG_MQTT_HELPER_SEC_TAG=$SEC_TAG
echo CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG=$SEC_TAG2

if [[ $setup_dps -eq 1 ]]; then
    id_scope=$(az iot dps show --name $AZ_DPS_NAME --resource-group $AZ_RGROUP \
                --query "properties.idScope" --output tsv)
    echo CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE=\"$id_scope\"
else
    echo CONFIG_AZURE_IOT_HUB_HOSTNAME=\"$hub_url\"
fi
