#include "register_types.h"

#include "ble_gdextension.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void initialize_ble(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<BLE>();

    // Create the singleton instance and register it
    BLE *ble_instance = memnew(BLE);
    Engine::get_singleton()->register_singleton("BLE", ble_instance);
}

void uninitialize_ble(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
    // Initialization.
    GDExtensionBool GDE_EXPORT ble_gdextension(const GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                GDExtensionClassLibraryPtr p_library,
                                                GDExtensionInitialization *r_initialization) {
    static GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_ble);
        init_obj.register_terminator(uninitialize_ble);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}