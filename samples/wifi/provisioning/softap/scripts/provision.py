import common_pb2
import requests
import argparse

def get_available_networks(verify_ssl=False):
    url = 'https://wifiprov.local/prov/networks'
    try:
        response = requests.get(url, verify=verify_ssl)
        response.raise_for_status()
        scan_results = common_pb2.ScanResults()
        scan_results.ParseFromString(response.content)
        return scan_results
    except requests.exceptions.HTTPError as err:
        print(f"HTTP error occurred: {err}")
    except requests.exceptions.RequestException as err:
        print(f"Error during requests to {url}: {err}")
    except Exception as err:
        print(f"An error occurred while parsing the protobuf response: {err}")
        print("Response content might not be a valid protobuf format.")
    return None

def select_network(scan_results):
    if not scan_results:
        print("No valid scan results to select from.")
        return None

    # Define a helper function to get the name of an enum value
    def get_enum_name(enum_field, value):
        return enum_field.DESCRIPTOR.values_by_number[value].name if value in enum_field.DESCRIPTOR.values_by_number else 'Unknown'

    for i, record in enumerate(scan_results.results):
        ssid = record.wifi.ssid.decode('utf-8') if record.wifi.ssid else "Unknown SSID"
        bssid = record.wifi.bssid.hex()
        rssi = record.rssi
        band = get_enum_name(common_pb2.Band, record.wifi.band) if record.wifi.HasField('band') else 'Not Specified'
        channel = record.wifi.channel
        auth = get_enum_name(common_pb2.AuthMode, record.wifi.auth) if record.wifi.HasField('auth') else 'Not Specified'

        # Printing all available details for each network
        print(f"{i}: SSID: {ssid}, BSSID: {bssid}, RSSI: {rssi}, Band: {band}, Channel: {channel}, Auth: {auth}")

    choice = int(input("Select the network (number): "))
    return scan_results.results[choice]

def configure_device(selected_record, passphrase, verify_ssl=False):
    if not selected_record or not passphrase:
        print("No network selected or passphrase provided.")
        return

    config = common_pb2.WifiConfig()
    config.wifi.CopyFrom(selected_record.wifi)
    config.passphrase = passphrase.encode()

    data = config.SerializeToString()
    print("Length of serialized data: ", len(data))

    url = 'https://wifiprov.local/prov/configure'
    headers = {'Content-Type': 'application/x-protobuf'}
    try:
        response = requests.post(url, headers=headers, data=data, verify=verify_ssl)
        response.raise_for_status()
        print("Configuration successful!")
        return response.status_code
    except requests.exceptions.HTTPError as err:
        print(f"HTTP error occurred: {err}")
    except requests.exceptions.RequestException as err:
        print(f"Error during requests to {url}: {err}")

def main():
    parser = argparse.ArgumentParser(description="Configure WiFi device", allow_abbrev=False)
    parser.add_argument("--certificate", help="Path to the server certificate for SSL verification", default=False)
    args = parser.parse_args()

    verify_ssl = args.certificate if args.certificate else False

    scan_results = get_available_networks(verify_ssl=verify_ssl)
    if scan_results:
        selected_record = select_network(scan_results)
        if selected_record:
            passphrase = input("Enter the passphrase for the network: ")
            if passphrase.strip():
                configure_device(selected_record, passphrase, verify_ssl=verify_ssl)
            else:
                print("Passphrase is required.")
        else:
            print("No network was selected.")
    else:
        print("Failed to retrieve scan results.")

if __name__ == "__main__":
    main()
