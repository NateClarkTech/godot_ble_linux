#ifndef BLE_GDEXTENSION_H
#define BLE_GDEXTENSION_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <sdbus-c++/sdbus-c++.h>
#include <memory>

class BLE : public godot::RefCounted {
    GDCLASS(BLE, godot::RefCounted)

private:
    static BLE *singleton;
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IProxy> adapter_proxy;

    // Map to store characteristic subscriptions: object path -> {proxy, address, uuid}
    struct SubscriptionInfo {
        std::unique_ptr<sdbus::IProxy> proxy;
        godot::String address;
        godot::String uuid;
    };

    std::map<std::string, SubscriptionInfo> characteristic_subscriptions;

    // Signal handler for PropertiesChanged
    void on_properties_changed(const std::string& interface,
                              const std::map<std::string, sdbus::Variant>& changed_properties,
                              const std::vector<std::string>& invalidated_properties,
                              const std::string& object_path);

protected:
    static void _bind_methods();

public:
    BLE();
    ~BLE();

    static BLE *get_singleton() { return singleton; }

    void start_scan();
    void stop_scan();
    godot::PackedStringArray get_discovered_devices();
    godot::PackedStringArray get_connected_devices();
    godot::String connect_device(godot::String address);
    godot::String disconnect_device(godot::String address);
    godot::Dictionary read_all_characteristics(godot::String address);
    godot::String read_characteristic(godot::String device_address, godot::String char_uuid);
    godot::String subscribe_to_characteristic(godot::String device_address, godot::String char_uuid, bool enable);
    godot::Dictionary get_characteristic_value(godot::String device_address, godot::String char_uuid);
    godot::PackedByteArray get_characteristic_value_bytes(godot::String device_address, godot::String char_uuid);

};

#endif // BLE_GDEXTENSION_H
