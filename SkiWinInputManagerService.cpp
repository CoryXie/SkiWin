/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "SkiWinInputManager"

//#define LOG_NDEBUG 0

// Log debug messages about InputReaderPolicy
#define DEBUG_INPUT_READER_POLICY 1

// Log debug messages about InputDispatcherPolicy
#define DEBUG_INPUT_DISPATCHER_POLICY 1

#include <limits.h>
#include <android_runtime/AndroidRuntime.h>

#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/threads.h>

#include <input/InputManager.h>
#include <input/PointerController.h>
#include <input/SpriteController.h>

#include "MessageQueue.h"

#include "SkiWinConfig.h"

#include "SkiWin.h"


namespace android {

/* Pointer icon styles.
 * Must match the definition in android.view.PointerIcon.
 */
enum {
    POINTER_ICON_STYLE_CUSTOM = -1,
    POINTER_ICON_STYLE_NULL = 0,
    POINTER_ICON_STYLE_ARROW = 1000,
    POINTER_ICON_STYLE_SPOT_HOVER = 2000,
    POINTER_ICON_STYLE_SPOT_TOUCH = 2001,
    POINTER_ICON_STYLE_SPOT_ANCHOR = 2002,
};

/*
 * Describes a pointer icon.
 */
struct PointerIcon {
    inline PointerIcon() {
        reset();
    }

    int32_t style;
    SkBitmap bitmap;
    float hotSpotX;
    float hotSpotY;

    inline bool isNullIcon() {
        return style == POINTER_ICON_STYLE_NULL;
    }

    inline void reset() {
        style = POINTER_ICON_STYLE_NULL;
        bitmap.reset();
        hotSpotX = 0;
        hotSpotY = 0;
    }
};

// --- Global functions ---

void loadSystemIconAsSprite(const void* contextObj, int32_t style,
        SpriteIcon* outSpriteIcon) {
        
    PointerIcon pointerIcon = PointerIcon();
    pointerIcon.bitmap.copyTo(&outSpriteIcon->bitmap, SkBitmap::kARGB_8888_Config);
    outSpriteIcon->hotSpotX = pointerIcon.hotSpotX;
    outSpriteIcon->hotSpotY = pointerIcon.hotSpotY;
}

enum {
    WM_ACTION_PASS_TO_USER = 1,
    WM_ACTION_WAKE_UP = 2,
    WM_ACTION_GO_TO_SLEEP = 4,
};


// --- SkiWinInputManager ---

extern sp <InputManager> gInputManager;

class SkiWinInputManager : public virtual RefBase,
    public virtual InputReaderPolicyInterface,
    public virtual InputDispatcherPolicyInterface,
    public virtual PointerControllerPolicyInterface {
protected:
    virtual ~SkiWinInputManager();

public:
    SkiWinInputManager(const void * contextObj, const void * serviceObj, const sp<Looper>& looper);

    inline sp<InputManager> getInputManager() const { return mInputManager; }

    inline void setWin(SkiWin *  win) { mWin = win; }

    void dump(String8& dump);

    void setDisplayViewport(bool external, const DisplayViewport& viewport);

    status_t registerInputChannel(const sp<InputChannel>& inputChannel,
            const sp<InputWindowHandle>& inputWindowHandle, bool monitor);
    status_t unregisterInputChannel(const sp<InputChannel>& inputChannel);

    void setInputWindows(Vector<sp<InputWindowHandle> >& windowHandles);
    void setFocusedApplication(sp<InputApplicationHandle>& applicationHandle);
    void setInputDispatchMode(bool enabled, bool frozen);
    void setSystemUiVisibility(int32_t visibility);
    void setPointerSpeed(int32_t speed);
    void setShowTouches(bool enabled);

    /* --- InputReaderPolicyInterface implementation --- */

    virtual void getReaderConfiguration(InputReaderConfiguration* outConfig);
    virtual sp<PointerControllerInterface> obtainPointerController(int32_t deviceId);
    virtual void notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices);
    virtual sp<KeyCharacterMap> getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor);
    virtual String8 getDeviceAlias(const InputDeviceIdentifier& identifier);

    /* --- InputDispatcherPolicyInterface implementation --- */

    virtual void notifySwitch(nsecs_t when, uint32_t switchValues, uint32_t switchMask,
            uint32_t policyFlags);
    virtual void notifyConfigurationChanged(nsecs_t when);
    virtual nsecs_t notifyANR(const sp<InputApplicationHandle>& inputApplicationHandle,
            const sp<InputWindowHandle>& inputWindowHandle);
    virtual void notifyInputChannelBroken(const sp<InputWindowHandle>& inputWindowHandle);
    virtual bool filterInputEvent(const InputEvent* inputEvent, uint32_t policyFlags);
    virtual void getDispatcherConfiguration(InputDispatcherConfiguration* outConfig);
    virtual bool isKeyRepeatEnabled();
    virtual void interceptKeyBeforeQueueing(const KeyEvent* keyEvent, uint32_t& policyFlags);
    virtual void interceptMotionBeforeQueueing(nsecs_t when, uint32_t& policyFlags);
    virtual nsecs_t interceptKeyBeforeDispatching(
            const sp<InputWindowHandle>& inputWindowHandle,
            const KeyEvent* keyEvent, uint32_t policyFlags);
    virtual bool dispatchUnhandledKey(const sp<InputWindowHandle>& inputWindowHandle,
            const KeyEvent* keyEvent, uint32_t policyFlags, KeyEvent* outFallbackKeyEvent);
    virtual void pokeUserActivity(nsecs_t eventTime, int32_t eventType);
    virtual bool checkInjectEventsPermissionNonReentrant(
            int32_t injectorPid, int32_t injectorUid);

    /* --- PointerControllerPolicyInterface implementation --- */

    virtual void loadPointerResources(PointerResources* outResources);

private:
    sp<InputManager> mInputManager;

    const void * mContextObj;
    const void * mServiceObj;
    SkiWin * mWin;
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
    void handleInterceptActions(int wmActions, nsecs_t when, uint32_t& policyFlags);
    void ensureSpriteControllerLocked();

    // Power manager interactions.
    bool isScreenOn();
    bool isScreenBright();
};

SkiWinInputManager::SkiWinInputManager(const void * contextObj,
        const void * serviceObj, const sp<Looper>& looper) :
        mLooper(looper) {

    mContextObj = contextObj;
    mServiceObj = serviceObj;

	{
	AutoMutex _l(mLock);
	mLocked.systemUiVisibility = ASYSTEM_UI_VISIBILITY_STATUS_BAR_VISIBLE;
	mLocked.pointerSpeed = 0;
	mLocked.pointerGesturesEnabled = true;
	mLocked.showTouches = false;
	}

        sp<EventHub> eventHub = new EventHub();
        mInputManager = new InputManager(eventHub, this, this);      
  
}

SkiWinInputManager::~SkiWinInputManager() {
    mContextObj = NULL;
    mServiceObj = NULL;
}

void SkiWinInputManager::dump(String8& dump) {
    mInputManager->getReader()->dump(dump);
    dump.append("\n");

    mInputManager->getDispatcher()->dump(dump);
    dump.append("\n");
}

void SkiWinInputManager::setDisplayViewport(bool external, const DisplayViewport& viewport) {
    bool changed = false;
    {
        AutoMutex _l(mLock);

        DisplayViewport& v = external ? mLocked.externalViewport : mLocked.internalViewport;
        if (v != viewport) {
            changed = true;
            v = viewport;

            if (!external) {
                sp<PointerController> controller = mLocked.pointerController.promote();
                if (controller != NULL) {
                    controller->setDisplayViewport(
                            viewport.logicalRight - viewport.logicalLeft,
                            viewport.logicalBottom - viewport.logicalTop,
                            viewport.orientation);
                }
            }
        }
    }

    if (changed) {
        mInputManager->getReader()->requestRefreshConfiguration(
                InputReaderConfiguration::CHANGE_DISPLAY_INFO);
    }
}

status_t SkiWinInputManager::registerInputChannel(
        const sp<InputChannel>& inputChannel,
        const sp<InputWindowHandle>& inputWindowHandle, bool monitor) {
    return mInputManager->getDispatcher()->registerInputChannel(
            inputChannel, inputWindowHandle, monitor);
}

status_t SkiWinInputManager::unregisterInputChannel(
        const sp<InputChannel>& inputChannel) {
    return mInputManager->getDispatcher()->unregisterInputChannel(inputChannel);
}

void SkiWinInputManager::getReaderConfiguration(InputReaderConfiguration* outConfig) {

    int virtualKeyQuietTime = config_virtualKeyQuietTimeMillis;

	outConfig->virtualKeyQuietTime = milliseconds_to_nanoseconds(virtualKeyQuietTime);

    outConfig->excludedDeviceNames.clear();

    int hoverTapTimeout = HOVER_TAP_TIMEOUT;
	int doubleTapTimeout = DOUBLE_TAP_TIMEOUT;
	int longPressTimeout = DEFAULT_LONG_PRESS_TIMEOUT;
	
	outConfig->pointerGestureTapInterval = milliseconds_to_nanoseconds(hoverTapTimeout);
	
	// We must ensure that the tap-drag interval is significantly shorter than
	// the long-press timeout because the tap is held down for the entire duration
	// of the double-tap timeout.
	int tapDragInterval = max(min(longPressTimeout - 100,
			  doubleTapTimeout), hoverTapTimeout);
	
	outConfig->pointerGestureTapDragInterval =
			  milliseconds_to_nanoseconds(tapDragInterval);

    int hoverTapSlop = HOVER_TAP_SLOP;

	outConfig->pointerGestureTapSlop = hoverTapSlop;

	{ // acquire lock
	AutoMutex _l(mLock);

	outConfig->pointerVelocityControlParameters.scale = exp2f(mLocked.pointerSpeed
	        * POINTER_SPEED_EXPONENT);
	outConfig->pointerGesturesEnabled = mLocked.pointerGesturesEnabled;

	outConfig->showTouches = mLocked.showTouches;

	outConfig->setDisplayInfo(false /*external*/, mLocked.internalViewport);
	outConfig->setDisplayInfo(true /*external*/, mLocked.externalViewport);
	} // release lock
}

sp<PointerControllerInterface> SkiWinInputManager::obtainPointerController(int32_t deviceId) {
    AutoMutex _l(mLock);

    sp<PointerController> controller = mLocked.pointerController.promote();
    if (controller == NULL) {
        ensureSpriteControllerLocked();

        controller = new PointerController(this, mLooper, mLocked.spriteController);
        mLocked.pointerController = controller;

        DisplayViewport& v = mLocked.internalViewport;
        controller->setDisplayViewport(
                v.logicalRight - v.logicalLeft,
                v.logicalBottom - v.logicalTop,
                v.orientation);
		
		// need to load user specified pointer icon
		controller->setPointerIcon(SpriteIcon());

        updateInactivityTimeoutLocked(controller);
    }
    return controller;
}

void SkiWinInputManager::ensureSpriteControllerLocked() {
    if (mLocked.spriteController == NULL) {
		
        int layer = -1; /* getPointerLayer */
		
        mLocked.spriteController = new SpriteController(mLooper, layer);
    }
}

void SkiWinInputManager::notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices) {
	ALOGD("notifyInputDevicesChanged");
}

sp<KeyCharacterMap> SkiWinInputManager::getKeyboardLayoutOverlay(
        const String8& inputDeviceDescriptor) {
        
	ALOGD("getKeyboardLayoutOverlay");
	
	sp<KeyCharacterMap> result;

    KeyCharacterMap::load(String8("/system/usr/keylayout/qwerty.kl"),
		KeyCharacterMap::FORMAT_OVERLAY, &result);

    return result;
}

String8 SkiWinInputManager::getDeviceAlias(const InputDeviceIdentifier& identifier) {

    String8 result = identifier.uniqueId;
    return result;
}

void SkiWinInputManager::notifySwitch(nsecs_t when,
        uint32_t switchValues, uint32_t switchMask, uint32_t policyFlags) {
#if DEBUG_INPUT_DISPATCHER_POLICY
    ALOGD("notifySwitch - when=%lld, switchValues=0x%08x, switchMask=0x%08x, policyFlags=0x%x",
            when, switchValues, switchMask, policyFlags);
#endif

    if (mWin != NULL)
		mWin->notifySwitch(when, switchValues, switchMask);
}

void SkiWinInputManager::notifyConfigurationChanged(nsecs_t when) {
#if DEBUG_INPUT_DISPATCHER_POLICY
    ALOGD("notifyConfigurationChanged - when=%lld", when);
#endif
    if (mWin != NULL)
		mWin->notifyConfigurationChanged(when);
}

nsecs_t SkiWinInputManager::notifyANR(const sp<InputApplicationHandle>& inputApplicationHandle,
        const sp<InputWindowHandle>& inputWindowHandle) {
#if DEBUG_INPUT_DISPATCHER_POLICY
    ALOGD("notifyANR");
#endif

    long newTimeout = 0;

    if (mWin != NULL)
		newTimeout = mWin->notifyANR(inputApplicationHandle, inputWindowHandle);

    return newTimeout;
}

void SkiWinInputManager::notifyInputChannelBroken(const sp<InputWindowHandle>& inputWindowHandle) {
#if DEBUG_INPUT_DISPATCHER_POLICY
    ALOGD("notifyInputChannelBroken");
#endif
    if (mWin != NULL)
		mWin->notifyInputChannelBroken(inputWindowHandle);
}

void SkiWinInputManager::getDispatcherConfiguration(InputDispatcherConfiguration* outConfig) {

    int keyRepeatTimeout = DEFAULT_LONG_PRESS_TIMEOUT;
	
    outConfig->keyRepeatTimeout = milliseconds_to_nanoseconds(keyRepeatTimeout);

    int keyRepeatDelay = KEY_REPEAT_DELAY;
	
    outConfig->keyRepeatDelay = milliseconds_to_nanoseconds(keyRepeatDelay);
}

bool SkiWinInputManager::isKeyRepeatEnabled() {
    // Only enable automatic key repeating when the screen is on.
    return isScreenOn();
}

void SkiWinInputManager::setInputWindows(Vector<sp<InputWindowHandle> >& windowHandles) {
		
    mInputManager->getDispatcher()->setInputWindows(windowHandles);

    // Do this after the dispatcher has updated the window handle state.
    bool newPointerGesturesEnabled = true;
    size_t numWindows = windowHandles.size();
    for (size_t i = 0; i < numWindows; i++) {
        const sp<InputWindowHandle>& windowHandle = windowHandles.itemAt(i);
        const InputWindowInfo* windowInfo = windowHandle->getInfo();
        if (windowInfo && windowInfo->hasFocus && (windowInfo->inputFeatures
                & InputWindowInfo::INPUT_FEATURE_DISABLE_TOUCH_PAD_GESTURES)) {
            newPointerGesturesEnabled = false;
        }
    }

    uint32_t changes = 0;
    { // acquire lock
        AutoMutex _l(mLock);

        if (mLocked.pointerGesturesEnabled != newPointerGesturesEnabled) {
            mLocked.pointerGesturesEnabled = newPointerGesturesEnabled;
            changes |= InputReaderConfiguration::CHANGE_POINTER_GESTURE_ENABLEMENT;
        }
    } // release lock

    if (changes) {
        mInputManager->getReader()->requestRefreshConfiguration(changes);
    }
}

void SkiWinInputManager::setFocusedApplication(sp<InputApplicationHandle>& applicationHandle) {	
	if (applicationHandle != NULL)
    	mInputManager->getDispatcher()->setFocusedApplication(applicationHandle);
}

void SkiWinInputManager::setInputDispatchMode(bool enabled, bool frozen) {
    mInputManager->getDispatcher()->setInputDispatchMode(enabled, frozen);
}

void SkiWinInputManager::setSystemUiVisibility(int32_t visibility) {
    AutoMutex _l(mLock);

    if (mLocked.systemUiVisibility != visibility) {
        mLocked.systemUiVisibility = visibility;

        sp<PointerController> controller = mLocked.pointerController.promote();
        if (controller != NULL) {
            updateInactivityTimeoutLocked(controller);
        }
    }
}

void SkiWinInputManager::updateInactivityTimeoutLocked(const sp<PointerController>& controller) {
    bool lightsOut = mLocked.systemUiVisibility & ASYSTEM_UI_VISIBILITY_STATUS_BAR_HIDDEN;
    controller->setInactivityTimeout(lightsOut
            ? PointerController::INACTIVITY_TIMEOUT_SHORT
            : PointerController::INACTIVITY_TIMEOUT_NORMAL);
}

void SkiWinInputManager::setPointerSpeed(int32_t speed) {
    { // acquire lock
        AutoMutex _l(mLock);

        if (mLocked.pointerSpeed == speed) {
            return;
        }

        ALOGI("Setting pointer speed to %d.", speed);
        mLocked.pointerSpeed = speed;
    } // release lock

    mInputManager->getReader()->requestRefreshConfiguration(
            InputReaderConfiguration::CHANGE_POINTER_SPEED);
}

void SkiWinInputManager::setShowTouches(bool enabled) {
    { // acquire lock
        AutoMutex _l(mLock);

        if (mLocked.showTouches == enabled) {
            return;
        }

        ALOGI("Setting show touches feature to %s.", enabled ? "enabled" : "disabled");
        mLocked.showTouches = enabled;
    } // release lock

    mInputManager->getReader()->requestRefreshConfiguration(
            InputReaderConfiguration::CHANGE_SHOW_TOUCHES);
}

bool SkiWinInputManager::isScreenOn() {
    return android_server_PowerManagerService_isScreenOn();
}

bool SkiWinInputManager::isScreenBright() {
    return android_server_PowerManagerService_isScreenBright();
}

bool SkiWinInputManager::filterInputEvent(const InputEvent* inputEvent, uint32_t policyFlags) {
	bool pass = true;
	
    switch (inputEvent->getType()) {
    case AINPUT_EVENT_TYPE_KEY:
		if (mWin != NULL)
        	pass = mWin->filterInputEvent(static_cast<const KeyEvent*>(inputEvent));
        break;
    case AINPUT_EVENT_TYPE_MOTION:
		if (mWin != NULL)
        	pass= mWin->filterInputEvent(static_cast<const MotionEvent*>(inputEvent));
        break;
    default:
        return true; // dispatch the event normally
    }

    return pass;
}

void SkiWinInputManager::interceptKeyBeforeQueueing(const KeyEvent* keyEvent,
        uint32_t& policyFlags) {
    // Policy:
    // - Ignore untrusted events and pass them along.
    // - Ask the window manager what to do with normal events and trusted injected events.
    // - For normal events wake and brighten the screen if currently off or dim.
    if ((policyFlags & POLICY_FLAG_TRUSTED)) {
        nsecs_t when = keyEvent->getEventTime();
        bool isScreenOn = this->isScreenOn();
        bool isScreenBright = this->isScreenBright();

		int wmActions = 0;

		if (mWin != NULL)
			wmActions = mWin->interceptKeyBeforeQueueing(keyEvent, policyFlags, isScreenOn);

        if (!(policyFlags & POLICY_FLAG_INJECTED)) {
            if (!isScreenOn) {
                policyFlags |= POLICY_FLAG_WOKE_HERE;
            }

            if (!isScreenBright) {
                policyFlags |= POLICY_FLAG_BRIGHT_HERE;
            }
        }

        handleInterceptActions(wmActions, when, /*byref*/ policyFlags);
    } else {
        policyFlags |= POLICY_FLAG_PASS_TO_USER;
    }
}

void SkiWinInputManager::interceptMotionBeforeQueueing(nsecs_t when, uint32_t& policyFlags) {
    // Policy:
    // - Ignore untrusted events and pass them along.
    // - No special filtering for injected events required at this time.
    // - Filter normal events based on screen state.
    // - For normal events brighten (but do not wake) the screen if currently dim.
    if ((policyFlags & POLICY_FLAG_TRUSTED) && !(policyFlags & POLICY_FLAG_INJECTED)) {
        if (isScreenOn()) {
            policyFlags |= POLICY_FLAG_PASS_TO_USER;

            if (!isScreenBright()) {
                policyFlags |= POLICY_FLAG_BRIGHT_HERE;
            }
        } else {
			int wmActions = 0;
			
			if (mWin != NULL)
				wmActions = mWin->interceptMotionBeforeQueueingWhenScreenOff(policyFlags);

            policyFlags |= POLICY_FLAG_WOKE_HERE | POLICY_FLAG_BRIGHT_HERE;
            handleInterceptActions(wmActions, when, /*byref*/ policyFlags);
        }
    } else {
        policyFlags |= POLICY_FLAG_PASS_TO_USER;
    }
}

void SkiWinInputManager::handleInterceptActions(int wmActions, nsecs_t when,
        uint32_t& policyFlags) {
    if (wmActions & WM_ACTION_GO_TO_SLEEP) {
#if DEBUG_INPUT_DISPATCHER_POLICY
        ALOGD("handleInterceptActions: Going to sleep.");
#endif
        android_server_PowerManagerService_goToSleep(when);
    }

    if (wmActions & WM_ACTION_WAKE_UP) {
#if DEBUG_INPUT_DISPATCHER_POLICY
        ALOGD("handleInterceptActions: Waking up.");
#endif
        android_server_PowerManagerService_wakeUp(when);
    }

    if (wmActions & WM_ACTION_PASS_TO_USER) {
        policyFlags |= POLICY_FLAG_PASS_TO_USER;
    } else {
#if DEBUG_INPUT_DISPATCHER_POLICY
        ALOGD("handleInterceptActions: Not passing key to user.");
#endif
    }
}

nsecs_t SkiWinInputManager::interceptKeyBeforeDispatching(
        const sp<InputWindowHandle>& inputWindowHandle,
        const KeyEvent* keyEvent, uint32_t policyFlags) {
    // Policy:
    // - Ignore untrusted events and pass them along.
    // - Filter normal events and trusted injected events through the window manager policy to
    //   handle the HOME key and the like.
    nsecs_t result = 0;
    if (policyFlags & POLICY_FLAG_TRUSTED) {
        // Note: inputWindowHandle may be null.
        
        if (keyEvent) {
            long delayMillis = 0;
            
	    if (mWin != NULL)
	    mWin->interceptKeyBeforeDispatching(
                    inputWindowHandle, keyEvent, policyFlags);
            bool error = 0;
            if (!error) {
                if (delayMillis < 0) {
                    result = -1;
                } else if (delayMillis > 0) {
                    result = milliseconds_to_nanoseconds(delayMillis);
                }
            }
        } else {
            ALOGE("Failed to obtain key event object for interceptKeyBeforeDispatching.");
        }
    }
    return result;
}

bool SkiWinInputManager::dispatchUnhandledKey(const sp<InputWindowHandle>& inputWindowHandle,
        const KeyEvent* keyEvent, uint32_t policyFlags, KeyEvent* outFallbackKeyEvent) {
    // Policy:
    // - Ignore untrusted events and do not perform default handling.
    bool result = false;
    if (policyFlags & POLICY_FLAG_TRUSTED) {
		
        // Note: inputWindowHandle may be null.
		if (keyEvent)
			{
			KeyEvent* fallbackKeyEvent = NULL;

			if (mWin != NULL)
				fallbackKeyEvent = mWin->dispatchUnhandledKey(inputWindowHandle,
										  keyEvent, policyFlags);
			
			// Note: outFallbackKeyEvent may be the same object as keyEvent.
			if (fallbackKeyEvent)
				{
				*outFallbackKeyEvent = *fallbackKeyEvent;
				result = true;
				}
			}
    }
    return result;
}

void SkiWinInputManager::pokeUserActivity(nsecs_t eventTime, int32_t eventType) {
    android_server_PowerManagerService_userActivity(eventTime, eventType);
}


bool SkiWinInputManager::checkInjectEventsPermissionNonReentrant(
        int32_t injectorPid, int32_t injectorUid) {
        
  	bool result = true;
	
	if (mWin != NULL)
		result = mWin->checkInjectEventsPermission(injectorPid, injectorUid);

    return result;
}

void SkiWinInputManager::loadPointerResources(PointerResources* outResources) {
    loadSystemIconAsSprite(mContextObj, POINTER_ICON_STYLE_SPOT_HOVER,
            &outResources->spotHover);
    loadSystemIconAsSprite(mContextObj, POINTER_ICON_STYLE_SPOT_TOUCH,
            &outResources->spotTouch);
    loadSystemIconAsSprite(mContextObj, POINTER_ICON_STYLE_SPOT_ANCHOR,
            &outResources->spotAnchor);
}

// ----------------------------------------------------------------------------

int SkiWinInputManagerInit(const void * serviceObj, const void * contextObj, sp<MessageQueue>& messageQueue) {
    SkiWinInputManager* im = new SkiWinInputManager(contextObj, serviceObj,
            messageQueue->getLooper());
    return reinterpret_cast<int>(im);
}

void SkiWinInputManagerStart(int ptr) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    status_t result = im->getInputManager()->start();
    if (result) {
        ALOGE("Input manager could not be started.");
    }
}

void SkiWinInputManagerSetWin(int ptr, int win) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->setWin(reinterpret_cast<SkiWin*>(win));
}

void SkiWinInputManagerSetDisplayViewport(int ptr, bool external,
        int displayId, int orientation,
        int logicalLeft, int logicalTop, int logicalRight, int logicalBottom,
        int physicalLeft, int physicalTop, int physicalRight, int physicalBottom,
        int deviceWidth, int deviceHeight) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    DisplayViewport v;
    v.displayId = displayId;
    v.orientation = orientation;
    v.logicalLeft = logicalLeft;
    v.logicalTop = logicalTop;
    v.logicalRight = logicalRight;
    v.logicalBottom = logicalBottom;
    v.physicalLeft = physicalLeft;
    v.physicalTop = physicalTop;
    v.physicalRight = physicalRight;
    v.physicalBottom = physicalBottom;
    v.deviceWidth = deviceWidth;
    v.deviceHeight = deviceHeight;
    im->setDisplayViewport(external, v);
}

int SkiWinInputManagerGetScanCodeState(int ptr, int deviceId, int sourceMask, int scanCode) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    return im->getInputManager()->getReader()->getScanCodeState(
            deviceId, uint32_t(sourceMask), scanCode);
}

int SkiWinInputManagerGetKeyCodeState(int ptr, int deviceId, int sourceMask, int keyCode) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    return im->getInputManager()->getReader()->getKeyCodeState(
            deviceId, uint32_t(sourceMask), keyCode);
}

int SkiWinInputManagerGetSwitchState(int ptr, int deviceId, int sourceMask, int sw) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    return im->getInputManager()->getReader()->getSwitchState(
            deviceId, uint32_t(sourceMask), sw);
}

bool SkiWinInputManagerHasKeys(int ptr, int deviceId, int sourceMask, int keyCodes[], bool outFlags[]) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    int32_t* codes = keyCodes;
    uint8_t* flags = reinterpret_cast<uint8_t*>(outFlags);
    size_t numCodes = sizeof(keyCodes)/sizeof(int);
	
    bool result;
	
    result = im->getInputManager()->getReader()->hasKeys(
                deviceId, uint32_t(sourceMask), numCodes, codes, flags);

    return result;
}

void SkiWinInputManagerHandleInputChannelDisposed(const sp<InputChannel>& inputChannel, void* data) {
    SkiWinInputManager* im = static_cast<SkiWinInputManager*>(data);

    ALOGW("Input channel object '%s' was disposed without first being unregistered with "
            "the input manager!", inputChannel->getName().string());
    im->unregisterInputChannel(inputChannel);
}

void SkiWinInputManagerRegisterInputChannel(int ptr, 
	const sp<InputChannel>& inputChannel, sp<InputWindowHandle>& inputWindowHandle, bool monitor) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    status_t status = im->registerInputChannel(inputChannel, inputWindowHandle, monitor);
    if (status) {
        ALOGE("Failed to register input channel.  status=%d", status);
        return;
    }

    if (! monitor) {
		//TODO:
        //android_view_InputChannel_setDisposeCallback(env, inputChannelObj,
        //        handleInputChannelDisposed, im);
    }
}

void SkiWinInputManagerUnregisterInputChannel(int ptr, const sp<InputChannel>& inputChannel) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    if (inputChannel == NULL) {
        return;
    }
	//TODO:
    //android_view_InputChannel_setDisposeCallback(env, inputChannelObj, NULL, NULL);

    status_t status = im->unregisterInputChannel(inputChannel);
    if (status && status != BAD_VALUE) { // ignore already unregistered channel
        ALOGE("Failed to unregister input channel.  status=%d", status);
    }
}

void SkiWinInputManagerSetInputFilterEnabled(int ptr, bool enabled) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->getInputManager()->getDispatcher()->setInputFilterEnabled(enabled);
}

int SkiWinInputManagerInjectInputEvent(int ptr, const InputEvent* inputEvent, int injectorPid, int injectorUid,
        int syncMode, int timeoutMillis, int policyFlags) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);
    int32_t type = inputEvent->getType();
    if (type == AINPUT_EVENT_TYPE_KEY) {

        const KeyEvent* keyEvent = static_cast<const KeyEvent*>(inputEvent);
        return im->getInputManager()->getDispatcher()->injectInputEvent(
                keyEvent, injectorPid, injectorUid, syncMode, timeoutMillis,
                uint32_t(policyFlags));
	}
    else if (type == AINPUT_EVENT_TYPE_MOTION)
	{
        const MotionEvent* motionEvent = static_cast<const MotionEvent*>(inputEvent);
        return im->getInputManager()->getDispatcher()->injectInputEvent(
                motionEvent, injectorPid, injectorUid, syncMode, timeoutMillis,
                uint32_t(policyFlags));
	}
  else
        return INPUT_EVENT_INJECTION_FAILED; 
    
}

void SkiWinInputManagerSetInputWindows(int ptr, Vector<sp<InputWindowHandle> >& windowHandles) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->setInputWindows(windowHandles);
}

void SkiWinInputManagerSetFocusedApplication(int ptr, sp<InputApplicationHandle>& applicationHandle) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->setFocusedApplication(applicationHandle);
}

void SkiWinInputManagerSetInputDispatchMode(int ptr, bool enabled, bool frozen) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->setInputDispatchMode(enabled, frozen);
}

void SkiWinInputManagerSetSystemUiVisibility(int ptr, int visibility) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->setSystemUiVisibility(visibility);
}

bool SkiWinInputManagerTransferTouchFocus(int ptr, 
	sp<InputChannel>& fromChannel, sp<InputChannel>& toChannel) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    if (fromChannel == NULL || toChannel == NULL) {
        return false;
    }

    return im->getInputManager()->getDispatcher()->transferTouchFocus(fromChannel, toChannel);
}

void SkiWinInputManagerSetPointerSpeed(int ptr, int speed) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->setPointerSpeed(speed);
}

void SkiWinInputManagerSetShowTouches(int ptr, bool enabled) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->setShowTouches(enabled);
}

void SkiWinInputManagerVibrate(int ptr, int deviceId, long long patternObj[],
        int repeat, int token) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    size_t patternSize = sizeof(patternObj)/sizeof(long);
    if (patternSize > MAX_VIBRATE_PATTERN_SIZE) {
        ALOGI("Skipped requested vibration because the pattern size is %d "
                "which is more than the maximum supported size of %d.",
                patternSize, MAX_VIBRATE_PATTERN_SIZE);
        return; // limit to reasonable size
    }

    long long * patternMillis = static_cast<long long *>(patternObj);
    nsecs_t pattern[patternSize];
    for (size_t i = 0; i < patternSize; i++) {
        pattern[i] = max(0LL, min(patternMillis[i],
                MAX_VIBRATE_PATTERN_DELAY_NSECS / 1000000LL)) * 1000000LL;
    }

    im->getInputManager()->getReader()->vibrate(deviceId, pattern, patternSize, repeat, token);
}

void SkiWinInputManagerCancelVibrate(int ptr, int deviceId, int token) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->getInputManager()->getReader()->cancelVibrate(deviceId, token);
}

void SkiWinInputManagerReloadKeyboardLayouts(int ptr) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->getInputManager()->getReader()->requestRefreshConfiguration(
            InputReaderConfiguration::CHANGE_KEYBOARD_LAYOUTS);
}

void SkiWinInputManagerReloadDeviceAliases(int ptr) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->getInputManager()->getReader()->requestRefreshConfiguration(
            InputReaderConfiguration::CHANGE_DEVICE_ALIAS);
}

 const char* SkiWinInputManagerDump(int ptr) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    String8 dump;
    im->dump(dump);
    return dump.string();
}

void SkiWinInputManagerMonitor(int ptr) {
    SkiWinInputManager* im = reinterpret_cast<SkiWinInputManager*>(ptr);

    im->getInputManager()->getReader()->monitor();
    im->getInputManager()->getDispatcher()->monitor();
}

SkiWinInputWindowHandle::SkiWinInputWindowHandle(int win,
        const sp<InputApplicationHandle>& inputApplicationHandle) :
        InputWindowHandle(inputApplicationHandle) {
	mWin = reinterpret_cast<SkiWin*>(win);
}

SkiWinInputWindowHandle::~SkiWinInputWindowHandle() {
}

bool SkiWinInputWindowHandle::updateInfo() {

    if (!mInfo) {
        mInfo = new InputWindowInfo();
    }

	if (mWin != NULL)
		return mWin->updateInputWindowInfo(mInfo);
	
    return false;
}

SkiWinInputApplicationHandle::SkiWinInputApplicationHandle(int win)  {
 mWin = reinterpret_cast<SkiWin*>(win);
}

SkiWinInputApplicationHandle::~SkiWinInputApplicationHandle() {
}

bool SkiWinInputApplicationHandle::updateInfo() {

    if (!mInfo) {
        mInfo = new InputApplicationInfo();
    }

    mInfo->name.setTo("SkiWin");

    mInfo->dispatchingTimeout = DEFAULT_INPUT_DISPATCHING_TIMEOUT_NANOS;

    return true;
}

} /* namespace android */

