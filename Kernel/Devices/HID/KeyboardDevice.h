#pragma once

// includes
#include <AK/CircularQueue.h>
#include <AK/DoublyLinkedList.h>
#include <AK/Types.h>
#include <Kernel/API/KeyCode.h>
#include <Kernel/Devices/CharacterDevice.h>
#include <Kernel/Devices/HID/HIDDevice.h>
#include <Kernel/Interrupts/IRQHandler.h>
#include <Kernel/Random.h>

namespace Kernel {

class KeyboardDevice : public HIDDevice {
public:
    using Event = KeyEvent;

    virtual ~KeyboardDevice() override;

    // ^CharacterDevice
    virtual KResultOr<size_t> read(FileDescription&, u64, UserOrKernelBuffer&, size_t) override;
    virtual bool can_read(const FileDescription&, size_t) const override;
    virtual KResultOr<size_t> write(FileDescription&, u64, const UserOrKernelBuffer&, size_t) override;
    virtual bool can_write(const FileDescription&, size_t) const override { return true; }

    // ^HIDDevice
    virtual Type instrument_type() const { return Type::Keyboard; }

    // ^Device
    virtual mode_t required_mode() const override { return 0440; }

    //FIXME: It should be something like String::formatted("keyboard{}", minor())
    // instead of a fixed string like this
    virtual String device_name() const override { return "keyboard"; }

    void update_modifier(u8 modifier, bool state)
    {
        if (state)
            m_modifiers |= modifier;
        else
            m_modifiers &= ~modifier;
    }

protected:
    KeyboardDevice();
    mutable SpinLock<u8> m_queue_lock;
    CircularQueue<Event, 16> m_queue;
    // ^CharacterDevice
    virtual const char* class_name() const override { return "KeyboardDevice"; }

    u8 m_modifiers { 0 };
    bool m_caps_lock_on { false };
    bool m_num_lock_on { false };
    bool m_has_e0_prefix { false };

    void key_state_changed(u8 raw, bool pressed);
};
}
