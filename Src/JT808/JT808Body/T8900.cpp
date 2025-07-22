#include "T8900.h"

T8900::T8900() : peripheralStatus(nullptr), peripheralSystem(nullptr) {}

T8900& T8900::build() {
    if (peripheralStatus) {
        message.key = PeripheralStatus::key;
        message.value = std::static_pointer_cast<void>(peripheralStatus);
    } else if (peripheralSystem) {
        message.key = PeripheralSystem::key;
        message.value = std::static_pointer_cast<void>(peripheralSystem);
    }
    return *this;
}
