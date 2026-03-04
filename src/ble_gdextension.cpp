#include "ble_gdextension.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <iostream>
#include <sstream>


using namespace godot;

BLE *BLE::singleton = nullptr;

BLE::BLE() {
    UtilityFunctions::print("BLE extension initialized.");
    singleton = this;

    connection = sdbus::createSystemBusConnection();
    adapter_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/org/bluez/hci0"));
    if (connection) {
        connection->enterEventLoopAsync();
    }
}

BLE::~BLE() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void BLE::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_scan"), &BLE::start_scan);
    ClassDB::bind_method(D_METHOD("stop_scan"), &BLE::stop_scan);
    ClassDB::bind_method(D_METHOD("get_discovered_devices"), &BLE::get_discovered_devices);
    ClassDB::bind_method(D_METHOD("get_connected_devices"), &BLE::get_connected_devices);
    ClassDB::bind_method(D_METHOD("connect_device", "address"), &BLE::connect_device);
    ClassDB::bind_method(D_METHOD("disconnect_device", "address"), &BLE::disconnect_device);
    ClassDB::bind_method(D_METHOD("read_all_characteristics", "address"), &BLE::read_all_characteristics);
    ClassDB::bind_method(D_METHOD("read_characteristic", "device_address", "char_uuid"), &BLE::read_characteristic);
    ClassDB::bind_method(D_METHOD("subscribe_to_characteristic", "device_address", "char_uuid"), &BLE::subscribe_to_characteristic);
    ClassDB::bind_method(D_METHOD("get_characteristic_value", "device_address", "char_uuid"), &BLE::get_characteristic_value);
    ClassDB::bind_method(D_METHOD("get_characteristic_value_bytes", "device_address", "char_uuid"), &BLE::get_characteristic_value_bytes);


    // Signals
    ADD_SIGNAL(MethodInfo("characteristic_value_updated",
        PropertyInfo(Variant::STRING, "device_address"),
        PropertyInfo(Variant::STRING, "characteristic_uuid"),
        PropertyInfo(Variant::PACKED_BYTE_ARRAY, "value")
    ));

    ADD_SIGNAL(MethodInfo("device_disconnected",
        PropertyInfo(Variant::STRING, "device_address")
    ));

}

void BLE::start_scan() {
    try {
        UtilityFunctions::print("Starting BLE scan via D-Bus...");

        adapter_proxy->callMethod("StartDiscovery").onInterface("org.bluez.Adapter1").dontExpectReply();

        UtilityFunctions::print("BLE scan started.");
    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error: ", e.getName().c_str(), ": ", e.getMessage().c_str());
    }
}

void BLE::stop_scan() {
    try {
        if (!adapter_proxy) {
            UtilityFunctions::printerr("Cannot stop scan: No active adapter proxy.");
            return;
        }

        adapter_proxy->callMethod("StopDiscovery").onInterface("org.bluez.Adapter1").dontExpectReply();

        UtilityFunctions::print("BLE scan stopped.");
    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error: ", e.getName().c_str(), ": ", e.getMessage().c_str());
    }
}

PackedStringArray BLE::get_discovered_devices() {
    PackedStringArray devices;

    try {
        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        for (const auto &[objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                const auto &properties = device_it->second;

                std::string address;
                std::string name;

                auto addr_it = properties.find("Address");
                if (addr_it != properties.end()) {
                    address = addr_it->second.get<std::string>();
                }

                auto name_it = properties.find("Name");
                if (name_it != properties.end()) {
                    name = name_it->second.get<std::string>();
                }

                if (!address.empty()) {
                    std::string entry = !name.empty()
                        ? name + " (" + address + ")"
                        : address;
                    devices.append(String(entry.c_str()));
                }
            }
        }

    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error in get_discovered_devices: ", e.getName().c_str(), ": ", e.getMessage().c_str());
    }

    return devices;
}

PackedStringArray BLE::get_connected_devices() {
    PackedStringArray devices;

    try {
        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        for (const auto &[objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                const auto &properties = device_it->second;

                bool connected = false;
                auto connected_it = properties.find("Connected");
                if (connected_it != properties.end()) {
                    connected = connected_it->second.get<bool>();
                }

                if (connected) {
                    std::string address;
                    std::string name;

                    auto addr_it = properties.find("Address");
                    if (addr_it != properties.end()) {
                        address = addr_it->second.get<std::string>();
                    }

                    auto name_it = properties.find("Name");
                    if (name_it != properties.end()) {
                        name = name_it->second.get<std::string>();
                    }

                    if (!address.empty()) {
                        std::string entry = !name.empty()
                            ? name + " (" + address + ")"
                            : address;
                        devices.append(String(entry.c_str()));
                    }
                }
            }
        }

    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error in get_connected_devices: ", e.getName().c_str(), ": ", e.getMessage().c_str());
    }

    return devices;
}

String BLE::connect_device(String address) {
    try {
        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;
        
        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        for (const auto &[objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                const auto &properties = device_it->second;
                auto addr_it = properties.find("Address");
                if (addr_it != properties.end() && addr_it->second.get<std::string>() == std::string(address.utf8().get_data())) {
                    auto device_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath(objectPath));
                    device_proxy->callMethod("Connect").onInterface("org.bluez.Device1");
                    return "Connected to " + address;
                }
            }
        }

        return "Device not found: " + address;
    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error in connect_device: ", e.getName().c_str(), ": ", e.getMessage().c_str());
        return "Connection failed to " + address;
    }
}

String BLE::disconnect_device(String address) {
    try {
        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        for (const auto &[objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                auto addr_it = device_it->second.find("Address");
                if (addr_it != device_it->second.end() && addr_it->second.get<std::string>() == std::string(address.utf8().get_data())) {
                    auto device_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), objectPath);
                    device_proxy->callMethod("Disconnect").onInterface("org.bluez.Device1");
                    return "Disconnected from " + address;
                }
            }
        }

        return "Device not found: " + address;
    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error in disconnect_device: ", e.getName().c_str(), ": ", e.getMessage().c_str());
        return "Failed to disconnect from " + address;
    }
}

Dictionary BLE::read_all_characteristics(String address) {
    Dictionary result;

    try {
        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        std::string target_device_path;

        // Step 1: Find the device path
        for (const auto &[objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                auto addr_it = device_it->second.find("Address");
                if (addr_it != device_it->second.end() &&
                    addr_it->second.get<std::string>() == std::string(address.utf8().get_data())) {
                    target_device_path = objectPath;
                    break;
                }
            }
        }

        if (target_device_path.empty()) {
            UtilityFunctions::printerr("Device not found.");
            return result;
        }

        // Step 2: Loop over characteristics
        for (const auto &[objectPath, interfaces] : managedObjects) {
            if (objectPath.rfind(target_device_path, 0) == 0) {
                auto char_it = interfaces.find("org.bluez.GattCharacteristic1");
                if (char_it != interfaces.end()) {
                    std::string uuid = "(unknown UUID)";
                    auto uuid_it = char_it->second.find("UUID");
                    if (uuid_it != char_it->second.end()) {
                        uuid = uuid_it->second.get<std::string>();
                    }

                    auto char_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), objectPath);
                    std::map<std::string, sdbus::Variant> options;
                    std::vector<uint8_t> value;

                    try {
                        char_proxy->callMethod("ReadValue")
                            .onInterface("org.bluez.GattCharacteristic1")
                            .withArguments(options)
                            .storeResultsTo(value);

                        PackedByteArray byte_array;
                        std::stringstream hex_stream;
                        for (uint8_t b : value) {
                            byte_array.append(b);
                            hex_stream << std::hex << std::uppercase << (int)b << " ";
                        }

                        Dictionary entry;
                        entry["value"] = byte_array;
                        entry["hex"] = String(hex_stream.str().c_str());
                        entry["path"] = String(objectPath.c_str());

                        result[String(uuid.c_str())] = entry;
                    } catch (const sdbus::Error &e) {
                        UtilityFunctions::printerr("Failed to read UUID ", uuid.c_str(), ": ", e.getMessage().c_str());
                    }
                }
            }
        }
    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error in read_all_characteristics: ", e.getName().c_str(), ": ", e.getMessage().c_str());
    }

    return result;
}



String BLE::read_characteristic(String device_address, String char_uuid) {
    try {
        char_uuid = char_uuid.to_lower().strip_edges();

        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        std::string target_device_path;

        // Step 1: Find the device path
        for (const auto &[objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                auto addr_it = device_it->second.find("Address");
                if (addr_it != device_it->second.end() && addr_it->second.get<std::string>() == std::string(device_address.utf8().get_data())) {
                    target_device_path = objectPath;
                    break;
                }
            }
        }

        if (target_device_path.empty()) {
            return "Device not found";
        }

        // Step 2: Search for GattCharacteristic under this device
        for (const auto &[objectPath, interfaces] : managedObjects) {
            if (objectPath.rfind(target_device_path, 0) == 0) {
                auto char_it = interfaces.find("org.bluez.GattCharacteristic1");
                if (char_it != interfaces.end()) {
                    auto uuid_it = char_it->second.find("UUID");
                    if (uuid_it != char_it->second.end() &&
                        uuid_it->second.get<std::string>() == std::string(char_uuid.utf8().get_data())) {

                        auto char_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath(objectPath));
                        std::map<std::string, sdbus::Variant> options;
                        std::vector<uint8_t> value;

                        char_proxy->callMethod("ReadValue")
                            .onInterface("org.bluez.GattCharacteristic1")
                            .withArguments(options)
                            .storeResultsTo(value);

                        std::stringstream ss;
                        for (auto b : value) {
                            ss << std::hex << std::uppercase << (int)b << " ";
                        }
                        return String("Value: ") + ss.str().c_str();
                    }
                }
            }
        }

        return "Characteristic not found";
    } catch (const sdbus::Error &e) {
        UtilityFunctions::printerr("D-Bus error in read_characteristic: ", e.getName().c_str(), ": ", e.getMessage().c_str());
        return "Failed to read from " + char_uuid + " on " + device_address;
    }
}

void BLE::on_properties_changed(const std::string& interface, const std::map<std::string, sdbus::Variant>& changed_properties, const std::vector<std::string>& invalidated_properties, const std::string& object_path) {

    if (interface != "org.bluez.GattCharacteristic1") {
        return;
    }

    auto value_it = changed_properties.find("Value");
    if (value_it == changed_properties.end()) {
        return;
    }

    try {
        auto subscription_it = characteristic_subscriptions.find(object_path);
        if (subscription_it == characteristic_subscriptions.end()) {
            return;
        }

        const auto& subscription = subscription_it->second;
        std::vector<uint8_t> value = value_it->second.get<std::vector<uint8_t>>();

        PackedByteArray byte_array;
        for (uint8_t b : value) {
            byte_array.append(b);
        }

        // Emit the signal to Godot
        emit_signal("characteristic_value_updated", subscription.address, subscription.uuid, byte_array);
        
    } catch (const sdbus::Error& e) {
        UtilityFunctions::printerr("Error processing PropertiesChanged signal: ", e.getName().c_str(), ": ", e.getMessage().c_str());
    }
}

String BLE::subscribe_to_characteristic(String device_address, String char_uuid, bool enable) {
    try {
        char_uuid = char_uuid.to_lower().strip_edges();        

        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        std::string target_device_path;
        std::string target_char_path;

        // Step 1: Find the device path
        for (const auto& [objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                auto addr_it = device_it->second.find("Address");
                if (addr_it != device_it->second.end() && addr_it->second.get<std::string>() == std::string(device_address.utf8().get_data())) {
                    target_device_path = objectPath;
                    break;
                }
            }
        }

        if (target_device_path.empty()) {
            UtilityFunctions::printerr("Device not found: ", device_address);
            return "Device not found: " + device_address;
        }

        // Step 2: Find the characteristic path
        for (const auto& [objectPath, interfaces] : managedObjects) {
            if (objectPath.rfind(target_device_path, 0) == 0) {
                auto char_it = interfaces.find("org.bluez.GattCharacteristic1");
                if (char_it != interfaces.end()) {
                    auto uuid_it = char_it->second.find("UUID");
                    if (uuid_it != char_it->second.end() && uuid_it->second.get<std::string>() == std::string(char_uuid.utf8().get_data())) {
                        target_char_path = objectPath;
                        break;
                    }
                }
            }
        }

        if (target_char_path.empty()) {
            UtilityFunctions::printerr("Characteristic not found: ", char_uuid);
            return "Characteristic not found: " + char_uuid;
        }

        // Step 3: Handle subscription or unsubscription
        auto subscription_it = characteristic_subscriptions.find(target_char_path);
        bool is_subscribed = subscription_it != characteristic_subscriptions.end();

        if (enable && !is_subscribed) {
            // Subscribe to the characteristic
            auto char_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath(target_char_path));

            // Subscribe to PropertiesChanged signal
            char_proxy->uponSignal("PropertiesChanged")
                .onInterface("org.freedesktop.DBus.Properties")
                .call([this, target_char_path](const std::string& interface, 
                                               const std::map<std::string, 
                                               sdbus::Variant>& changed_properties, 
                                               const std::vector<std::string>& invalidated_properties) 
                {
                    this->on_properties_changed(interface, changed_properties, invalidated_properties, target_char_path);
                });

            // Start notifications
            char_proxy->callMethod("StartNotify")
                .onInterface("org.bluez.GattCharacteristic1");

            // Store subscription info
            SubscriptionInfo info;
            info.proxy = std::move(char_proxy);
            info.address = device_address;
            info.uuid = char_uuid;
            characteristic_subscriptions[target_char_path] = std::move(info);

            UtilityFunctions::print("Subscribed to characteristic: ", char_uuid, " on device: ", device_address);
            return "Subscribed to " + char_uuid;
        } else if (!enable && is_subscribed) {
            // Unsubscribe from the characteristic
            auto& subscription = subscription_it->second;
            subscription.proxy->callMethod("StopNotify")
                .onInterface("org.bluez.GattCharacteristic1");

            // Remove subscription
            characteristic_subscriptions.erase(target_char_path);

            UtilityFunctions::print("Unsubscribed from characteristic: ", char_uuid, " on device: ", device_address);
            return "Unsubscribed from " + char_uuid;
        } else if (enable && is_subscribed) {
            UtilityFunctions::print("Already subscribed to characteristic: ", char_uuid);
            return "Already subscribed to " + char_uuid;
        } else {
            UtilityFunctions::print("Not subscribed to characteristic: ", char_uuid);
            return "Not subscribed to " + char_uuid;
        }
    } catch (const sdbus::Error& e) {
        UtilityFunctions::printerr("D-Bus error in subscribe_to_characteristic: ", e.getName().c_str(), ": ", e.getMessage().c_str());
        return "Failed to " + String(enable ? "subscribe to " : "unsubscribe from ") + char_uuid + " on " + device_address;
    }
}

Dictionary BLE::get_characteristic_value(String device_address, String char_uuid) {
    Dictionary result;

    try {
        char_uuid = char_uuid.to_lower().strip_edges();
        
        // Get all managed objects from BlueZ
        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        std::string target_device_path;

        // Find the device path matching the address
        for (const auto& [objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                auto addr_it = device_it->second.find("Address");
                if (addr_it != device_it->second.end() && addr_it->second.get<std::string>() == std::string(device_address.utf8().get_data())) {
                    target_device_path = objectPath;
                    break;
                }
            }
        }

        if (target_device_path.empty()) {
            result["error"] = "Device not found";
            return result;
        }

        // Find the characteristic path matching the UUID
        for (const auto& [objectPath, interfaces] : managedObjects) {
            if (objectPath.rfind(target_device_path, 0) == 0) {
                auto char_it = interfaces.find("org.bluez.GattCharacteristic1");
                if (char_it != interfaces.end()) {
                    auto uuid_it = char_it->second.find("UUID");
                    if (uuid_it != char_it->second.end() &&
                        uuid_it->second.get<std::string>() == std::string(char_uuid.utf8().get_data())) {

                        // Create proxy and call ReadValue
                        auto char_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), objectPath);
                        std::map<std::string, sdbus::Variant> options;
                        std::vector<uint8_t> value;

                        char_proxy->callMethod("ReadValue")
                            .onInterface("org.bluez.GattCharacteristic1")
                            .withArguments(options)
                            .storeResultsTo(value);

                        PackedByteArray byte_array;
                        std::stringstream hex_stream;

                        for (uint8_t b : value) {
                            byte_array.append(b);
                            hex_stream << std::hex << std::uppercase << (int)b << " ";
                        }

                        result["value"] = byte_array;
                        result["hex"] = String(hex_stream.str().c_str());
                        result["path"] = String(objectPath.c_str());
                        result["uuid"] = char_uuid;
                        return result;
                    }
                }
            }
        }

        result["error"] = "Characteristic not found";
        return result;
    } catch (const sdbus::Error& e) {
        result["error"] = String("D-Bus error: ") + e.getMessage().c_str();
        return result;
    }
}

PackedByteArray BLE::get_characteristic_value_bytes(String device_address, String char_uuid) {
    PackedByteArray byte_array;

    try {
        char_uuid = char_uuid.to_lower().strip_edges();

        // Get all managed objects
        auto obj_mgr = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath("/"));
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> managedObjects;

        obj_mgr->callMethod("GetManagedObjects")
            .onInterface("org.freedesktop.DBus.ObjectManager")
            .storeResultsTo(managedObjects);

        std::string target_device_path;

        // Find device path
        for (const auto& [objectPath, interfaces] : managedObjects) {
            auto device_it = interfaces.find("org.bluez.Device1");
            if (device_it != interfaces.end()) {
                auto addr_it = device_it->second.find("Address");
                if (addr_it != device_it->second.end() &&
                    addr_it->second.get<std::string>() == std::string(device_address.utf8().get_data())) {
                    target_device_path = objectPath;
                    break;
                }
            }
        }

        if (target_device_path.empty()) {
            return byte_array;
        }

        // Find characteristic path
        for (const auto& [objectPath, interfaces] : managedObjects) {
            if (objectPath.rfind(target_device_path, 0) == 0) {
                auto char_it = interfaces.find("org.bluez.GattCharacteristic1");
                if (char_it != interfaces.end()) {
                    auto uuid_it = char_it->second.find("UUID");
                    if (uuid_it != char_it->second.end() &&
                        uuid_it->second.get<std::string>() == std::string(char_uuid.utf8().get_data())) {

                        auto char_proxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.bluez"), sdbus::ObjectPath(objectPath));
                        std::map<std::string, sdbus::Variant> options;
                        std::vector<uint8_t> value;

                        char_proxy->callMethod("ReadValue")
                            .onInterface("org.bluez.GattCharacteristic1")
                            .withArguments(options)
                            .storeResultsTo(value);

                        for (uint8_t b : value) {
                            byte_array.append(b);
                        }

                        return byte_array;
                    }
                }
            }
        }

    } catch (const sdbus::Error& e) {
        UtilityFunctions::printerr("D-Bus error in get_characteristic_bytes: ", e.getName().c_str(), ": ", e.getMessage().c_str());
    }

    return byte_array; // empty on error
}
