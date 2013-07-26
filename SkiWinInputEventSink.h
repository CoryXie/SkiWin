#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/threads.h>
#include <androidfw/InputTransport.h>

namespace android {

class SkiWinInputEventSink {
public:
	bool dispatchBatchedInputEventPending(); 
	bool dispatchInputEvent(int seq, KeyEvent* event);
	bool dispatchInputEvent(int seq, MotionEvent* event); 
};
}
