# Linux Bluetooth Extension for Godot  
A gdextension that adds some basic Bluetooth API for the godot engine. Useful for games that may need to communicate with custom hardware. This addon is likely incomplete in terms of functionality and needs improvemnet and should improved before being production ready, but good enough for testing.  
Currently functionality is limited to connecting to a device and reading or subscribing to a given bluetooth characteristic's value.  
Subscribing to a characteristic value change allows a single to be connected to when that characteristic changes, allowing responses to the read data.  

## Extension deps:
[sdbus-c++](https://github.com/Kistler-Group/sdbus-cpp)  
[BlueZ](https://www.bluez.org/)

## Useage:

Place a compiled version in ./addons folder in godot 4.5+ project

`BLE.start_scan() -> void` : turns on bluetooth scanning  
`BLE.stop_scan() -> void` : turns off bluetooth scanning  
`BLE.connect_device(address: String) -> String` : attempts to connect to bluetooth device with given mac address  
`BLE.disconnect_device(address: String) -> String` : attempts to disconnect to bluetooth device with given mac address  
`BLE.get_discovered_devices() -> PackedStringArray` : returns a PackedStringArray that has the names and mac addresses of discovered devices in the format of: "\<name> \(\<address>)" \|| "\<address>"   
`BLE.get_connected_devices() -> PackedStringArray` : returns a PackedStringArray that has the names and mac addresses of discovered devices in the format of: "\<name> \(\<address>)" \|| "\<address>"   
`BLE.read_all_characteristic(String address) -> Dictionary` : returns a Dictionary of all the characters known on the given bluetooth mac address (device should be connected first and be given a short period to send characteristic info to computer else none will be found)  
`BLE.read_characteristic(String device_address, String char_uuid) -> String` : returns a string describing a characteristic  
`BLE.subsribe_to_characteristic(String device_address, String char_uuid, bool enable) -> String` : subscribes to a given characteristic on a given bluetooth mac address (assuming device is connected and characteristic is known) TODO: document what enable does  
`BLE.get_characteristic_value(String device_address, String char_uuid) -> String` : gets the value of a given characteristic of a device as a string  
`BLE.get_characteristic_value_bytes(String device_address, String char_uuid) -> PackedByteArray` : gets the values of a given characteristic of a device as a PackedByteArray  
`Signal BLE.characteristic_value_updated(String device_address, characteristic_uuid: String, value: PackedByteArray)` : signle that can be used to send data to a function that is connected to that single in Godot  

## Sample Code:

```gdscript
func connect_accel_device_one(scan_duration: float = 5.0) -> bool:
  BLE.start_scan();
  ble_scanner_on = true
  var found_device = false
  
  # Wait for the scan to run for the specified duration
  await get_tree().create_timer(scan_duration).timeout
	
  for device in BLE.get_discovered_devices():
    if device.contains("EF:08:68:1C:85:9E"):
      print(BLE.connect_device("EF:08:68:1C:85:9E"))
      found_device = true

	if found_device:
		await get_tree().create_timer(2.0).timeout
		
		print(BLE.subscribe_to_characteristic("EF:08:68:1C:85:9E", "6e400003-b5a3-f393-e0a9-e50e24dcca9e", true))
		BLE.characteristic_value_updated.connect(_on_characteristic_value_updated)
		BLE.stop_scan()
		ble_scanner_on = false		
	
	self.bluetooth_device_address = "EF:08:68:1C:85:9E"
	self.set_process(true)
  return found_device
```

## Deps: 
Ensure you have both `sdbus` and `scons` installed. 
A better solution will come down the line maybe something like openBLE. 
For now we build for linux and bind to sdbus. Scons is for godot extension build system. 

## Build command:

```bash
scons platform=linux target=TARGET arch=ARCH

targets=[editor,template_debug,template_release]
arch=[x86_64,arm64]
```


Pick and choose what you need and when.

## In event of issues:

Most likely pain point is the latest godot-cpp tools. You need to specficically build against
version 4.5 since we are till on there. 

###  Check current godot-cpp version

```bash
cd godot-cpp && git status && git log -1 --oneline
```

### Update godot-cpp to match Godot 4.5

```bash
git fetch --all
git checkout 4.5.0  # or whatever tag matches your Godot version
```

Then rebuild everything

## Still having problems?

### Expected Build Output
After building, you should see in `bin/`:
- libble_gdextension.linux.editor.ARCH.so
- libble_gdextension.linux.template_debug.ARCH.so  
- libble_gdextension.linux.template_release.ARCH.so

### Known Issues

**Duplicate .gdextension:**

If Godot fails to load, check that there's NO .gdextension file in the `bin/` folder
only the one at the addon root should exist. If it is inside bin, move it to the root
of the addon. 

## TODO
Add more functionallity  
Clean up code and look for preformance improvements and bugs  
Port idea to other other systems
