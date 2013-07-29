#ifndef ANDROID_SKIWINCONFIG_H
#define ANDROID_SKIWINCONFIG_H
namespace android {
 /**
   * Defines the width of the horizontal scrollbar and the height of the vertical scrollbar in
   * dips
   */
   static  int SCROLL_BAR_SIZE = 10;

  /**
   * Duration of the fade when scrollbars fade away in milliseconds
   */
   static  int SCROLL_BAR_FADE_DURATION = 250;

  /**
   * Default delay before the scrollbars fade in milliseconds
   */
   static  int SCROLL_BAR_DEFAULT_DELAY = 300;

  /**
   * Defines the length of the fading edges in dips
   */
   static  int FADING_EDGE_LENGTH = 12;

  /**
   * Defines the duration in milliseconds of the pressed state in child
   * components.
   */
   static  int PRESSED_STATE_DURATION = 64;

  /**
   * Defines the default duration in milliseconds before a press turns into
   * a long press
   */
   static  int DEFAULT_LONG_PRESS_TIMEOUT = 500;

  /**
   * Defines the time between successive key repeats in milliseconds.
   */
   static  int KEY_REPEAT_DELAY = 50;

  /**
   * Defines the duration in milliseconds a user needs to hold down the
   * appropriate button to bring up the global actions dialog (power off,
   * lock screen, etc).
   */
   static  int GLOBAL_ACTIONS_KEY_TIMEOUT = 500;

  /**
   * Defines the duration in milliseconds we will wait to see if a touch event
   * is a tap or a scroll. If the user does not move within this interval, it is
   * considered to be a tap.
   */
   static  int TAP_TIMEOUT = 180;

  /**
   * Defines the duration in milliseconds we will wait to see if a touch event
   * is a jump tap. If the user does not complete the jump tap within this interval, it is
   * considered to be a tap.
   */
   static  int JUMP_TAP_TIMEOUT = 500;

  /**
   * Defines the duration in milliseconds between the first tap's up event and
   * the second tap's down event for an interaction to be considered a
   * double-tap.
   */
   static  int DOUBLE_TAP_TIMEOUT = 300;

  /**
   * Defines the maximum duration in milliseconds between a touch pad
   * touch and release for a given touch to be considered a tap (click) as
   * opposed to a hover movement gesture.
   */
   static  int HOVER_TAP_TIMEOUT = 150;

  /**
   * Defines the maximum distance in pixels that a touch pad touch can move
   * before being released for it to be considered a tap (click) as opposed
   * to a hover movement gesture.
   */
   static  int HOVER_TAP_SLOP = 20;

  /**
   * Defines the duration in milliseconds we want to display zoom controls in response
   * to a user panning within an application.
   */
   static  int ZOOM_CONTROLS_TIMEOUT = 3000;

  /**
   * Inset in dips to look for touchable content when the user touches the edge of the screen
   */
   static  int EDGE_SLOP = 12;

  /**
   * Distance a touch can wander before we think the user is scrolling in dips.
   * Note that this value defined here is only used as a fallback by legacy/misbehaving
   * applications that do not provide a Context for determining density/configuration-dependent
   * values.
   *
   * To alter this value, see the configuration resource config_viewConfigurationTouchSlop
   * in frameworks/base/core/res/res/values/config.xml or the appropriate device resource overlay.
   * It may be appropriate to tweak this on a device-specific basis in an overlay based on
   * the characteristics of the touch panel and firmware.
   */
   static  int TOUCH_SLOP = 8;

  /**
   * Distance the first touch can wander before we stop considering this event a double tap
   * (in dips)
   */
   static  int DOUBLE_TAP_TOUCH_SLOP = TOUCH_SLOP;

  /**
   * Distance a touch can wander before we think the user is attempting a paged scroll
   * (in dips)
   *
   * Note that this value defined here is only used as a fallback by legacy/misbehaving
   * applications that do not provide a Context for determining density/configuration-dependent
   * values.
   *
   * See the note above on {@link #TOUCH_SLOP} regarding the dimen resource
   * config_viewConfigurationTouchSlop. ViewConfiguration will report a paging touch slop of
   * config_viewConfigurationTouchSlop * 2 when provided with a Context.
   */
   static  int PAGING_TOUCH_SLOP = TOUCH_SLOP * 2;

  /**
   * Distance in dips between the first touch and second touch to still be considered a double tap
   */
   static  int DOUBLE_TAP_SLOP = 100;

  /**
   * Distance in dips a touch needs to be outside of a window's bounds for it to
   * count as outside for purposes of dismissing the window.
   */
   static  int WINDOW_TOUCH_SLOP = 16;

  /**
   * Minimum velocity to initiate a fling, as measured in dips per second
   */
   static  int MINIMUM_FLING_VELOCITY = 50;

  /**
   * Maximum velocity to initiate a fling, as measured in dips per second
   */
   static  int MAXIMUM_FLING_VELOCITY = 8000;

  /**
   * Delay before dispatching a recurring accessibility event in milliseconds.
   * This delay guarantees that a recurring event will be send at most once
   * during the {@link #SEND_RECURRING_ACCESSIBILITY_EVENTS_INTERVAL_MILLIS} time
   * frame.
   */
   static  long SEND_RECURRING_ACCESSIBILITY_EVENTS_INTERVAL_MILLIS = 100;

  /**
   * The maximum size of View's drawing cache, expressed in bytes. This size
   * should be at least equal to the size of the screen in ARGB888 format.
   */
   static  int MAXIMUM_DRAWING_CACHE_SIZE = 480 * 800 * 4; // ARGB8888

  /**
   * The coefficient of friction applied to flings/scrolls.
   */
   static  float SCROLL_FRICTION = 0.015f;

  /**
   * Max distance in dips to overscroll for edge effects
   */
   static  int OVERSCROLL_DISTANCE = 0;

  /**
   * Max distance in dips to overfling for edge effects
   */
   static  int OVERFLING_DISTANCE = 6;

   static  int config_virtualKeyQuietTimeMillis = 250;

   // Default input dispatching timeout in nanoseconds.

   static  long DEFAULT_INPUT_DISPATCHING_TIMEOUT_NANOS = 5000 * 1000000L;

  // Window flags from WindowManager.LayoutParams
    enum {
        FLAG_ALLOW_LOCK_WHILE_SCREEN_ON     = 0x00000001,
        FLAG_DIM_BEHIND        = 0x00000002,
        FLAG_BLUR_BEHIND        = 0x00000004,
        FLAG_NOT_FOCUSABLE      = 0x00000008,
        FLAG_NOT_TOUCHABLE      = 0x00000010,
        FLAG_NOT_TOUCH_MODAL    = 0x00000020,
        FLAG_TOUCHABLE_WHEN_WAKING = 0x00000040,
        FLAG_KEEP_SCREEN_ON     = 0x00000080,
        FLAG_LAYOUT_IN_SCREEN   = 0x00000100,
        FLAG_LAYOUT_NO_LIMITS   = 0x00000200,
        FLAG_FULLSCREEN      = 0x00000400,
        FLAG_FORCE_NOT_FULLSCREEN   = 0x00000800,
        FLAG_DITHER             = 0x00001000,
        FLAG_SECURE             = 0x00002000,
        FLAG_SCALED             = 0x00004000,
        FLAG_IGNORE_CHEEK_PRESSES    = 0x00008000,
        FLAG_LAYOUT_INSET_DECOR = 0x00010000,
        FLAG_ALT_FOCUSABLE_IM = 0x00020000,
        FLAG_WATCH_OUTSIDE_TOUCH = 0x00040000,
        FLAG_SHOW_WHEN_LOCKED = 0x00080000,
        FLAG_SHOW_WALLPAPER = 0x00100000,
        FLAG_TURN_SCREEN_ON = 0x00200000,
        FLAG_DISMISS_KEYGUARD = 0x00400000,
        FLAG_SPLIT_TOUCH = 0x00800000,
        FLAG_HARDWARE_ACCELERATED = 0x01000000,
        FLAG_HARDWARE_ACCELERATED_SYSTEM = 0x02000000,
        FLAG_SLIPPERY = 0x04000000,
        FLAG_NEEDS_MENU_KEY = 0x08000000,
        FLAG_KEEP_SURFACE_WHILE_ANIMATING = 0x10000000,
        FLAG_COMPATIBLE_WINDOW = 0x20000000,
        FLAG_SYSTEM_ERROR = 0x40000000,
    };

    // Window types from WindowManager.LayoutParams
    enum {
        FIRST_APPLICATION_WINDOW = 1,
        TYPE_BASE_APPLICATION   = 1,
        TYPE_APPLICATION        = 2,
        TYPE_APPLICATION_STARTING = 3,
        LAST_APPLICATION_WINDOW = 99,
        FIRST_SUB_WINDOW        = 1000,
        TYPE_APPLICATION_PANEL  = FIRST_SUB_WINDOW,
        TYPE_APPLICATION_MEDIA  = FIRST_SUB_WINDOW+1,
        TYPE_APPLICATION_SUB_PANEL = FIRST_SUB_WINDOW+2,
        TYPE_APPLICATION_ATTACHED_DIALOG = FIRST_SUB_WINDOW+3,
        TYPE_APPLICATION_MEDIA_OVERLAY  = FIRST_SUB_WINDOW+4,
        LAST_SUB_WINDOW         = 1999,
        FIRST_SYSTEM_WINDOW     = 2000,
        TYPE_STATUS_BAR         = FIRST_SYSTEM_WINDOW,
        TYPE_SEARCH_BAR         = FIRST_SYSTEM_WINDOW+1,
        TYPE_PHONE              = FIRST_SYSTEM_WINDOW+2,
        TYPE_SYSTEM_ALERT       = FIRST_SYSTEM_WINDOW+3,
        TYPE_KEYGUARD           = FIRST_SYSTEM_WINDOW+4,
        TYPE_TOAST              = FIRST_SYSTEM_WINDOW+5,
        TYPE_SYSTEM_OVERLAY     = FIRST_SYSTEM_WINDOW+6,
        TYPE_PRIORITY_PHONE     = FIRST_SYSTEM_WINDOW+7,
        TYPE_SYSTEM_DIALOG      = FIRST_SYSTEM_WINDOW+8,
        TYPE_KEYGUARD_DIALOG    = FIRST_SYSTEM_WINDOW+9,
        TYPE_SYSTEM_ERROR       = FIRST_SYSTEM_WINDOW+10,
        TYPE_INPUT_METHOD       = FIRST_SYSTEM_WINDOW+11,
        TYPE_INPUT_METHOD_DIALOG= FIRST_SYSTEM_WINDOW+12,
        TYPE_WALLPAPER          = FIRST_SYSTEM_WINDOW+13,
        TYPE_STATUS_BAR_PANEL   = FIRST_SYSTEM_WINDOW+14,
        TYPE_SECURE_SYSTEM_OVERLAY = FIRST_SYSTEM_WINDOW+15,
        TYPE_DRAG               = FIRST_SYSTEM_WINDOW+16,
        TYPE_STATUS_BAR_SUB_PANEL  = FIRST_SYSTEM_WINDOW+17,
        TYPE_POINTER            = FIRST_SYSTEM_WINDOW+18,
        TYPE_NAVIGATION_BAR     = FIRST_SYSTEM_WINDOW+19,
        TYPE_VOLUME_OVERLAY = FIRST_SYSTEM_WINDOW+20,
        TYPE_BOOT_PROGRESS = FIRST_SYSTEM_WINDOW+21,
        LAST_SYSTEM_WINDOW      = 2999,
    };

    enum {
        INPUT_FEATURE_DISABLE_TOUCH_PAD_GESTURES = 0x00000001,
        INPUT_FEATURE_NO_INPUT_CHANNEL = 0x00000002,
        INPUT_FEATURE_DISABLE_USER_ACTIVITY = 0x00000004,
    };

class InputManager;    
extern bool android_server_PowerManagerService_isScreenOn();
extern bool android_server_PowerManagerService_isScreenBright();
extern void android_server_PowerManagerService_userActivity(nsecs_t eventTime, int32_t eventType);
extern void android_server_PowerManagerService_wakeUp(nsecs_t eventTime);
extern void android_server_PowerManagerService_goToSleep(nsecs_t eventTime);
extern sp <InputManager> android_server_GetInputManager(void);

class SkiWin;
extern int SkiWinInputManagerInit(const void * serviceObj, const void * contextObj, sp<MessageQueue>& messageQueue);
extern void SkiWinInputManagerStart(int ptr);
extern void SkiWinInputManagerSetWin(int ptr, int win);
extern void SkiWinInputManagerRegisterInputChannel(int ptr, 
	const sp<InputChannel>& inputChannel, sp<InputWindowHandle>& inputWindowHandle, bool monitor);
extern void SkiWinInputManagerSetFocusedApplication(int ptr, sp<InputApplicationHandle>& applicationHandle);
extern void SkiWinInputManagerSetInputWindows(int ptr, Vector<sp<InputWindowHandle> >& windowHandles);

class SkiWinInputApplicationHandle : public InputApplicationHandle {
public:
    SkiWinInputApplicationHandle(int win);
    virtual ~SkiWinInputApplicationHandle();

    virtual bool updateInfo();
private:
	SkiWin * mWin;
};

class SkiWinInputWindowHandle : public InputWindowHandle {
public:
    SkiWinInputWindowHandle(int win, const sp<InputApplicationHandle>& inputApplicationHandle);
    virtual ~SkiWinInputWindowHandle();

    virtual bool updateInfo();
	
private:
	SkiWin * mWin;
};

}
#endif

