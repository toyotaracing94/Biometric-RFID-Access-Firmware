import os

def get_config_path():
    return os.path.join("src", "config", "Config.h")

def inject_config(vin_number: str, ssid: str, password: str):
    config_file = get_config_path()
    try:
        if not os.path.exists(config_file):
            print(f"❌ Error: Could not find {config_file}. Make sure the path is correct.")
            return
        
        with open(config_file, 'r') as file:
            lines = file.readlines()
        
        config_entries = {
            "#define VIN": f'#define VIN "{vin_number}"\n',
            "#define WIFI_SSID": f'#define WIFI_SSID "{ssid}"\n',
            "#define WIFI_PASSWORD": f'#define WIFI_PASSWORD "{password}"\n'
        }
        
        found_keys = {key: False for key in config_entries}
        
        for i, line in enumerate(lines):
            for key in config_entries:
                if line.startswith(key):
                    lines[i] = config_entries[key]
                    found_keys[key] = True
        
        for key, found in found_keys.items():
            if not found:
                lines.append(config_entries[key])
        
        with open(config_file, 'w') as file:
            file.writelines(lines)
        
        print(f"✅ Successfully injected VIN, WiFi SSID, and Password to {config_file}")
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    vin_number = input("Enter Device VIN: ")
    ssid = input("Enter WiFi SSID: ")
    password = input("Enter WiFi Password: ")
    inject_config(vin_number, ssid, password)