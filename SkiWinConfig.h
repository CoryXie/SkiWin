#ifndef ANDROID_SKIWINCONFIG_H
#define ANDROID_SKIWINCONFIG_H
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

extern bool android_server_PowerManagerService_isScreenOn();
extern bool android_server_PowerManagerService_isScreenBright();
extern void android_server_PowerManagerService_userActivity(nsecs_t eventTime, int32_t eventType);
extern void android_server_PowerManagerService_wakeUp(nsecs_t eventTime);
extern void android_server_PowerManagerService_goToSleep(nsecs_t eventTime);

class SkiWin;
extern int SkiWinInputManagerInit(const void * serviceObj, const void * contextObj, sp<MessageQueue>& messageQueue);
extern void SkiWinInputManagerStart(int ptr);
extern void SkiWinInputManagerSetWin(int ptr, sp<SkiWin>& win);

#endif

