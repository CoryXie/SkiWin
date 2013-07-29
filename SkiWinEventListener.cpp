#define LOG_TAG "SkiWinEventInputListener"

//#define LOG_NDEBUG 0


#include <SkWindow.h>
#include <SkApplication.h>
#include <cutils/log.h>
#include "SkiWinEventListener.h"

namespace android {

// The exponent used to calculate the pointer speed scaling factor.
// The scaling factor is calculated as 2 ^ (speed * exponent),
// where the speed ranges from -7 to + 7 and is supplied by the user.
static const float POINTER_SPEED_EXPONENT = 1.0f / 4;

template<typename T>
inline static T min(const T& a, const T& b) {
    return a < b ? a : b;
}

template<typename T>
inline static T max(const T& a, const T& b) {
    return a > b ? a : b;
}

// --- SkiWinEventInputListener ---

SkiWinEventInputListener::SkiWinEventInputListener(SkOSWindow * & window,
		sp<Looper>& looper) :
        mWindow(window), mLooper(looper){
	{
	AutoMutex _l(mLock);
	mLocked.systemUiVisibility = ASYSTEM_UI_VISIBILITY_STATUS_BAR_VISIBLE;
	mLocked.pointerSpeed = 0;
	mLocked.pointerGesturesEnabled = true;
	mLocked.showTouches = false;
	}
}

SkiWinEventInputListener::~SkiWinEventInputListener() {
}

void SkiWinEventInputListener::notifyConfigurationChanged(
        const NotifyConfigurationChangedArgs* args) {
	ALOGD("notifyConfigurationChanged\n");
}

void SkiWinEventInputListener::notifyKey(const NotifyKeyArgs* args) {
	ALOGD("notifyKey\n");
}

void SkiWinEventInputListener::notifyMotion(const NotifyMotionArgs* args) {
	ALOGD("notifyMotion\n");
}

void SkiWinEventInputListener::notifySwitch(const NotifySwitchArgs* args) {
	ALOGD("notifySwitch\n");
}

void SkiWinEventInputListener::notifyDeviceReset(const NotifyDeviceResetArgs* args) {
	ALOGD("notifyDeviceReset\n");
}

void SkiWinEventInputListener::getReaderConfiguration(InputReaderConfiguration* outConfig) {

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

void SkiWinEventInputListener::updateInactivityTimeoutLocked(const sp<PointerController>& controller) {
    bool lightsOut = mLocked.systemUiVisibility & ASYSTEM_UI_VISIBILITY_STATUS_BAR_HIDDEN;
    controller->setInactivityTimeout(lightsOut
            ? PointerController::INACTIVITY_TIMEOUT_SHORT
            : PointerController::INACTIVITY_TIMEOUT_NORMAL);
}

sp<PointerControllerInterface> SkiWinEventInputListener::obtainPointerController(int32_t deviceId) {
    AutoMutex _l(mLock);

    sp<PointerController> controller = mLocked.pointerController.promote();
    if (controller == NULL) {
		if (mLocked.spriteController == NULL) {
			
			int layer = -1; /* getPointerLayer */
			
			mLocked.spriteController = new SpriteController(mLooper, layer);
		}

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

void SkiWinEventInputListener::notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices) {
	ALOGD("notifyInputDevicesChanged");
}

sp<KeyCharacterMap> SkiWinEventInputListener::getKeyboardLayoutOverlay(
        const String8& inputDeviceDescriptor) {
        
	ALOGD("getKeyboardLayoutOverlay");
	
	sp<KeyCharacterMap> result;

    KeyCharacterMap::load(String8("/system/usr/keychars/qwerty2.kcm"),
		KeyCharacterMap::FORMAT_OVERLAY, &result);

    return result;
}

String8 SkiWinEventInputListener::getDeviceAlias(const InputDeviceIdentifier& identifier) {
	ALOGD("getDeviceAlias");

    String8 result = identifier.uniqueId;
    return result;
}

}

