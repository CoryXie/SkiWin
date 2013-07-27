
#ifndef _ANDROID_SkiWinInputEventSink_H
#define _ANDROID_SkiWinInputEventSink_H
#include <android_runtime/AndroidRuntime.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/threads.h>
#include <androidfw/InputTransport.h>
#include "SkiWin.h"

namespace android {

class SkiWin;

class SkiWinInputEventSink  : public RefBase{
public:
        SkiWinInputEventSink(sp<SkiWin>& win);
       ~SkiWinInputEventSink(){};
	bool dispatchBatchedInputEventPending(); 
	bool dispatchInputEvent(int seq, KeyEvent* event);
	bool dispatchInputEvent(int seq, MotionEvent* event); 
private:
       sp<SkiWin> mWin;
};
}
#endif
