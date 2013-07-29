#ifndef _UI_SkiWinEventInputListener_H
#define _UI_SkiWinEventInputListener_H

#include <androidfw/Input.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>

#include <limits.h>
#include <android_runtime/AndroidRuntime.h>

#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/threads.h>

#include <input/InputManager.h>
#include <input/PointerController.h>
#include <input/SpriteController.h>

namespace android {

/*
 * An implementation of the listener interface that dispaths 
 * input events to SkOSWindow objects.
 */
class SkiWinEventInputListener : public InputListenerInterface, 
                                 public InputReaderPolicyInterface {
protected:
    virtual ~SkiWinEventInputListener();

public:
    SkiWinEventInputListener(const SkOSWindow * & window, sp<Looper>& looper);

    virtual void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args);
    virtual void notifyKey(const NotifyKeyArgs* args);
    virtual void notifyMotion(const NotifyMotionArgs* args);
    virtual void notifySwitch(const NotifySwitchArgs* args);
    virtual void notifyDeviceReset(const NotifyDeviceResetArgs* args);

    /* Gets the input reader configuration. */
    virtual void getReaderConfiguration(InputReaderConfiguration* outConfig) = 0;

    /* Gets a pointer controller associated with the specified cursor device (ie. a mouse). */
    virtual sp<PointerControllerInterface> obtainPointerController(int32_t deviceId) = 0;

    /* Notifies the input reader policy that some input devices have changed
     * and provides information about all current input devices.
     */
    virtual void notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices) = 0;

    /* Gets the keyboard layout for a particular input device. */
    virtual sp<KeyCharacterMap> getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor) = 0;

    /* Gets a user-supplied alias for a particular input device, or an empty string if none. */
    virtual String8 getDeviceAlias(const InputDeviceIdentifier& identifier) = 0;

private:
    SkOSWindow * mWindow;
    sp<Looper> mLooper;
    Mutex mLock;
    
    struct Locked {
        // Display size information.
        DisplayViewport internalViewport;
        DisplayViewport externalViewport;

        // System UI visibility.
        int32_t systemUiVisibility;

        // Pointer speed.
        int32_t pointerSpeed;

        // True if pointer gestures are enabled.
        bool pointerGesturesEnabled;

        // Show touches feature enable/disable.
        bool showTouches;

        // Sprite controller singleton, created on first use.
        sp<SpriteController> spriteController;

        // Pointer controller singleton, created and destroyed as needed.
        wp<PointerController> pointerController;
    } mLocked;

    void updateInactivityTimeoutLocked(const sp<PointerController>& controller);
};

}
#endif /* _UI_SkiWinEventInputListener_H */