/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "SkiWin"

#include <cutils/properties.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <utils/Log.h>
#include <utils/threads.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
# include <sys/resource.h>
#endif

#include <SkTypeface.h>
#include <SkTemplates.h>
#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkEvent.h>
#include <SkCanvas.h>
#include <SkWindow.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include "SkiWin.h"

#include "net/base/host_resolver.h"
#include "googleurl/src/url_util.h"
#include "chrome/common/net/url_fetcher.h"

using namespace android;
#if 0
class WebURLFetcher : public URLFetcher::Delegate
{
public:
	WebURLFetcher(){id = 0;};
	
    // This will be called when the URL has been fetched, successfully or not.
    // |response_code| is the HTTP response code (200, 404, etc.) if
    // applicable.  |url|, |status| and |data| are all valid until the
    // URLFetcher instance is destroyed.
    void OnURLFetchComplete(const URLFetcher* source,
                                    const GURL& url,
                                    const net::URLRequestStatus& status,
                                    int response_code,
                                    const ResponseCookies& cookies,
                                    const std::string& data) 
	{
	printf("OnURLFetchComplete\n");
	}

void GetURL(std::string &url)
	{
	// To use this class, create an instance with the desired URL and a pointer to
	// the object to be notified when the URL has been loaded:

    URLFetcher* fetcher = URLFetcher::Create(id++, GURL(url),
                                        URLFetcher::GET, this);
	fetcher->Start();
	}

private:
	int id;
};
#endif

///////////////////////////////////////////
/////////////// SkEvent impl //////////////
///////////////////////////////////////////

void SkEvent::SignalQueueTimer(SkMSec) {}

void SkEvent::SignalNonEmptyQueue() {}

///////////////////////////////////////////
///////////// SkOSWindow impl /////////////
///////////////////////////////////////////

void SkOSWindow::onSetTitle(const char title[])
    {
    printf("View Title %s\n", title);
    }

void SkOSWindow::onHandleInval(const SkIRect& rect)
    {
    }


void SkOSWindow::onPDFSaved(const char title[], const char desc[],
                            const char path[])
    {

    }

// ---------------------------------------------------------------------------
extern void WebURLTest();
int main(int argc, char** argv)
    {
#if defined(HAVE_PTHREADS)
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);
#endif

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();
	#if 0
	WebURLFetcher web;
	std::string url("http://www.google.com.hk");
	web.GetURL(url);
	#endif

    // create the SkiWin object
    sp<SkiWin> skiwin = new SkiWin();

    IPCThreadState::self()->joinThreadPool();

    return 0;
    }
