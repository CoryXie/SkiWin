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

namespace android
{

#define DEFAULT_POLL_TIMEOUT_MS 500

typedef void (*NotifyKeyCallback)(const NotifyKeyArgs* args, void* context);
typedef void (*NotifyMotionCallback)(const NotifyMotionArgs* args, void* context);
typedef void (*NotifySwitchCallback)(const NotifySwitchArgs* args, void* context);

struct SkiWinEventCallback
    {

    NotifyKeyCallback    pfNotifyKey;
    NotifyMotionCallback pfNotifyMotion;
    NotifySwitchCallback pfNotifySwitch;

    void* context;
    };

struct SkiWinInputConfiguration
    {
    bool touchPointerVisible;
    int touchPointerLayer;
    };

void SkiWinInputManagerInit(
    SkiWinEventCallback* gInputEventCallback,
    SkiWinInputConfiguration* configuration);

void SkiWinInputManagerStart();
void SkiWinInputManagerStop();
void SkiWinInputManagerExit();
}
#endif /* _UI_SkiWinEventInputListener_H */
