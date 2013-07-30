#define LOG_TAG "SkiWinEventInputListener"

//#define LOG_NDEBUG 0


#include <SkWindow.h>
#include <SkApplication.h>
#include <cutils/log.h>
#include "SkiWinEventListener.h"
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#define REPORT_FUNCTION() ALOGV("%s\n", __PRETTY_FUNCTION__);

namespace android {

class SkiWinPointerControllerPolicy : public PointerControllerPolicyInterface
	{
	public:

    static const size_t bitmap_width = 64;
    static const size_t bitmap_height = 64;

    SkiWinPointerControllerPolicy()
		{
		bitmap.setConfig(
		    SkBitmap::kARGB_8888_Config,
		    bitmap_width,
		    bitmap_height);
		bitmap.allocPixels();

		// Icon for spot touches
		bitmap.eraseARGB(125, 0, 255, 0);
		spotTouchIcon = SpriteIcon(
		                    bitmap,
		                    bitmap_width/2,
		                    bitmap_height/2);
		// Icon for anchor touches
		bitmap.eraseARGB(125, 0, 0, 255);
		spotAnchorIcon = SpriteIcon(
		                     bitmap,
		                     bitmap_width/2,
		                     bitmap_height/2);
		// Icon for hovering touches
		bitmap.eraseARGB(125, 255, 0, 0);
		spotHoverIcon = SpriteIcon(
		                    bitmap,
		                    bitmap_width/2,
		                    bitmap_height/2);
		}

    void loadPointerResources(PointerResources* outResources)
	    {
	    outResources->spotHover = spotHoverIcon.copy();
	    outResources->spotTouch = spotTouchIcon.copy();
	    outResources->spotAnchor = spotAnchorIcon.copy();
	    }

    SpriteIcon spotHoverIcon;
    SpriteIcon spotTouchIcon;
    SpriteIcon spotAnchorIcon;
    SkBitmap bitmap;
	};

class SkiWinInputReaderPolicyInterface : public InputReaderPolicyInterface
	{
	public:
	    static const int32_t internal_display_id = ISurfaceComposer::eDisplayIdMain;
	    static const int32_t external_display_id = ISurfaceComposer::eDisplayIdHdmi;

	    SkiWinInputReaderPolicyInterface(
	        SkiWinInputConfiguration* configuration,
	        const sp<Looper>& looper)
	        : mLooper(looper),
	          mTouchPointerVisible(configuration->touchPointerLayer)
		{
		mInputReaderConfig.showTouches = configuration->touchPointerVisible;

		auto display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);

		DisplayInfo info;

		SurfaceComposerClient::getDisplayInfo(display, &info);

		DisplayViewport viewport;

		viewport.setNonDisplayViewport(info.w, info.h);

		viewport.displayId = ISurfaceComposer::eDisplayIdMain;

		mInputReaderConfig.setDisplayInfo(false, /* external */ viewport);

		}

	    void getReaderConfiguration(InputReaderConfiguration* outConfig)
	    {
	    *outConfig = mInputReaderConfig;
	    }

	    sp<PointerControllerInterface> obtainPointerController(int32_t deviceId)
		{
		(void) deviceId;

		sp<SpriteController> sprite_controller(
		    new SpriteController(
		        mLooper,
		        mTouchPointerVisible));

		sp<PointerController> pointer_controller(
		    new PointerController(
		        sp<SkiWinPointerControllerPolicy>(new SkiWinPointerControllerPolicy()),
		        mLooper,
		        sprite_controller));

		pointer_controller->setPresentation(PointerControllerInterface::PRESENTATION_SPOT);

		int32_t w, h, o;

		auto display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);

		DisplayInfo info;

		SurfaceComposerClient::getDisplayInfo(display, &info);

		pointer_controller->setDisplayViewport(info.w, info.h, info.orientation);

		return pointer_controller;
		}

	    virtual void notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices)
		{
	    mInputDevices = inputDevices;
	    }

	    virtual sp<KeyCharacterMap> getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor)
		{
	    return NULL;
	    }

	    virtual String8 getDeviceAlias(const InputDeviceIdentifier& identifier)
		{
	    return String8::empty();
	    }

	private:
	    sp<Looper> mLooper;
	    int mTouchPointerVisible;
	    InputReaderConfiguration mInputReaderConfig;
	    Vector<InputDeviceInfo> mInputDevices;
	};

class SkiWinInputListener : public InputListenerInterface
	{
	public:

		SkiWinInputListener(SkiWinEventCallback* callback) :
			callback(callback)
			{
			}

		void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args)
			{
		    REPORT_FUNCTION();
		    (void) args;
			}

		void notifyKey(const NotifyKeyArgs* args)
			{
		    REPORT_FUNCTION();

		    callback->pfNotifyKey(args, callback->context);
			}

		void notifyMotion(const NotifyMotionArgs* args)
			{
		    REPORT_FUNCTION();

		    callback->pfNotifyMotion(args, callback->context);
			}

		void notifySwitch(const NotifySwitchArgs* args)
			{
		    REPORT_FUNCTION();
		    
		    callback->pfNotifySwitch(args, callback->context);
			}

		void notifyDeviceReset(const NotifyDeviceResetArgs* args)
			{
		    REPORT_FUNCTION();
		    (void) args;
			}

	private:

	SkiWinEventCallback* callback;

	};

class SkiWinInputListenerLooperThread : public Thread
	{
	public:

	    SkiWinInputListenerLooperThread(const sp<Looper>& looper) : mLooper(looper)
		    {
		    }

	private:
	    bool threadLoop()
	    	{
#define DEFAULT_POLL_TIMEOUT_MS 500
	        if (ALOOPER_POLL_ERROR == mLooper->pollAll(DEFAULT_POLL_TIMEOUT_MS))
	            return false;
	        return true;
	    	}

	    sp<Looper> mLooper;
	};

class SkiWinInputManager : public RefBase
	{
	public:

    SkiWinInputManager(SkiWinEventCallback* callback, SkiWinInputConfiguration* configuration)
        : mLooper(new Looper(false)),
          mLooperThread(new SkiWinInputListenerLooperThread(mLooper)),
          mEventHub(new EventHub()),
          mInputReaderPolicy(new SkiWinInputReaderPolicyInterface(configuration, mLooper)),
          mInputListener(new SkiWinInputListener(callback)),
          mInputReader(new InputReader(
                           mEventHub,
                           mInputReaderPolicy,
                           mInputListener)),
          mInputReaderThread(new InputReaderThread(mInputReader))
    	{
    	}

    ~SkiWinInputManager()
    	{
        mInputReaderThread->requestExitAndWait();
    	}

	    sp<Looper> mLooper;
	    sp<SkiWinInputListenerLooperThread> mLooperThread;

	    sp<EventHubInterface> mEventHub;
	    sp<InputReaderPolicyInterface> mInputReaderPolicy;
	    sp<InputListenerInterface> mInputListener;
	    sp<InputReaderInterface> mInputReader;
	    sp<InputReaderThread> mInputReaderThread;

	    Condition mWaitCondition;
	    Mutex mWaitLock;
	};

sp<SkiWinInputManager> gSkiWinInputManager;

void SkiWinInputManagerInit(SkiWinEventCallback* listener, SkiWinInputConfiguration* config)
	{
    gSkiWinInputManager = new SkiWinInputManager(listener, config);
	}

void SkiWinInputManagerLoopOnce()
	{
	gSkiWinInputManager->mInputReader->loopOnce();
	}

void SkiWinInputManagerStart()
	{
    gSkiWinInputManager->mInputReaderThread->run();
    gSkiWinInputManager->mLooperThread->run();
	}

void SkiWinInputManagerStartAndWait(bool* flag)
	{
    gSkiWinInputManager->mInputReaderThread->run();
    gSkiWinInputManager->mLooperThread->run();

    while (!*flag)
    	{
        gSkiWinInputManager->mWaitCondition.waitRelative(
            gSkiWinInputManager->mWaitLock,
            10 * 1000 * 1000);
    	}
	}

void SkiWinInputManagerStop()
	{
    gSkiWinInputManager->mInputReaderThread->requestExitAndWait();
	}

void SkiWinInputManagerExit()
	{
    gSkiWinInputManager = NULL;
	}
}

