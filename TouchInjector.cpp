// Copyright (C) 2026 Martin Kaltenbrunner <martin@tuio.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "TouchInjector.h"
#include <TuioCursor.h>
#include <TuioTime.h>
#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/IOKitLib.h>
#include <dispatch/dispatch.h>
#include <iostream>
#include <math.h>
#include <atomic>
#include <memory>

#define MAX_CONTACTS 10
#define DIGITIZER_MAX 32767

// IOHIDUserDevice is exported by IOKit but has no public SDK header.
// Creating a device requires com.apple.developer.hid.virtual.device.
extern "C" {
typedef struct __IOHIDUserDevice *IOHIDUserDeviceRef;
IOHIDUserDeviceRef IOHIDUserDeviceCreateWithProperties(CFAllocatorRef allocator, CFDictionaryRef properties, IOOptionBits options);
void IOHIDUserDeviceSetDispatchQueue(IOHIDUserDeviceRef device, dispatch_queue_t queue);
void IOHIDUserDeviceActivate(IOHIDUserDeviceRef device);
void IOHIDUserDeviceCancel(IOHIDUserDeviceRef device);
IOReturn IOHIDUserDeviceHandleReportAsync(IOHIDUserDeviceRef device, const uint8_t *report, CFIndex reportLength, IOReturn (*callback)(void *refcon, IOReturn result), void *refcon);
}

// ---------------------------------------------------------------------------
// Virtual HID digitizer backend (system-level multitouch, macOS 27+)
// ---------------------------------------------------------------------------

static void appendFinger(uint8_t *&d, int usage_page)
{
	static const uint8_t finger[] = {
		0x09, 0x22, 0xA1, 0x02,					// Usage (Finger), Collection (Logical)
		0x09, 0x42, 0x15, 0x00, 0x25, 0x01,	//   Tip Switch, 0..1
		0x75, 0x01, 0x95, 0x01, 0x81, 0x02,	//   1 bit input
		0x75, 0x07, 0x95, 0x01, 0x81, 0x03,	//   7 bits padding
		0x09, 0x51, 0x75, 0x08, 0x95, 0x01, 0x81, 0x02,	//   Contact ID
		0x05, 0x01, 0x09, 0x30, 0x09, 0x31,	//   Generic Desktop X, Y
		0x16, 0x00, 0x00, 0x26, 0xFF, 0x7F,	//   0..32767
		0x75, 0x10, 0x95, 0x02, 0x81, 0x02,	//   2x16 bit input
		0xC0									// End Collection
	};
	memcpy(d, finger, sizeof(finger));
	d += sizeof(finger);
}

class DigitizerInjector : public InjectorBackend {
public:
	DigitizerInjector() : device(NULL)
	{
		uint8_t desc[1024];
		uint8_t *d = desc;
		static const uint8_t head[] = {
			0x05, 0x0D, 0x09, 0x04, 0xA1, 0x01,	// Digitizer, Touch Screen, App
			0x85, 0x01							// Report ID 1
		};
		memcpy(d, head, sizeof(head)); d += sizeof(head);
		for (int i=0;i<MAX_CONTACTS;i++) {
			if (i) { *d++ = 0x05; *d++ = 0x0D; }
			appendFinger(d, 0x0D);
		}
		static const uint8_t tail[] = {
			0x05, 0x0D, 0x09, 0x55, 0x15, 0x00, 0x25, MAX_CONTACTS,
			0x75, 0x08, 0x95, 0x01, 0x81, 0x02,	// Contact Count
			0x09, 0x55, 0x25, MAX_CONTACTS,
			0x75, 0x08, 0x95, 0x01, 0xB1, 0x02,	// Contact Count Max (feature)
			0xC0
		};
		memcpy(d, tail, sizeof(tail)); d += sizeof(tail);

		CFDataRef rdesc = CFDataCreate(NULL, desc, d - desc);
		int vid = 0xFA77, pid = 0x0001, ver = 1, upage = 0x0D, usage = 0x04;
		CFNumberRef nvid  = CFNumberCreate(NULL, kCFNumberIntType, &vid);
		CFNumberRef npid  = CFNumberCreate(NULL, kCFNumberIntType, &pid);
		CFNumberRef nver  = CFNumberCreate(NULL, kCFNumberIntType, &ver);
		CFNumberRef nup   = CFNumberCreate(NULL, kCFNumberIntType, &upage);
		CFNumberRef nu    = CFNumberCreate(NULL, kCFNumberIntType, &usage);
		const void *keys[] = {
			CFSTR("ReportDescriptor"), CFSTR("VendorID"), CFSTR("ProductID"),
			CFSTR("Product"), CFSTR("Manufacturer"), CFSTR("Transport"),
			CFSTR("PrimaryUsagePage"), CFSTR("PrimaryUsage"), CFSTR("VersionNumber")
		};
		const void *vals[] = {
			rdesc, nvid, npid, CFSTR("TongsengTouch"), CFSTR("tongseng"),
			CFSTR("Virtual"), nup, nu, nver
		};
		CFDictionaryRef props = CFDictionaryCreate(NULL, keys, vals, 9,
			&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		device = IOHIDUserDeviceCreateWithProperties(NULL, props, 0);
		CFRelease(props); CFRelease(rdesc);
		CFRelease(nvid); CFRelease(npid); CFRelease(nver); CFRelease(nup); CFRelease(nu);
		if (device) {
			IOHIDUserDeviceSetDispatchQueue(device,
				dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
			IOHIDUserDeviceActivate(device);
			reportPending.reset(new std::atomic<bool>(false));
			std::cout << "virtual multitouch device created" << std::endl;
		}
	}

	~DigitizerInjector()
	{
		if (device) {
			IOHIDUserDeviceCancel(device);
			CFRelease(device);
		}
	}

	bool isValid() const { return device != NULL; }

	struct AsyncReportCtx {
		uint8_t *report;
		std::shared_ptr<std::atomic<bool> > pending;
	};

	static IOReturn reportDone(void *refcon, IOReturn result)
	{
		AsyncReportCtx *ctx = (AsyncReportCtx*)refcon;
		if (result != kIOReturnSuccess)
			fprintf(stderr, "HID report failed: 0x%x\n", result);
		free(ctx->report);
		ctx->pending->store(false);
		delete ctx;
		return kIOReturnSuccess;
	}

	void apply(const std::vector<InjectedTouch>& touches)
	{
		// drop frames while a report is still in flight, so a congested
		// kernel queue can never stall the receiver thread
		bool expected = false;
		if (!reportPending->compare_exchange_strong(expected, true)) return;

		const CFIndex len = 1 + MAX_CONTACTS*6 + 1;
		uint8_t *report = (uint8_t*)calloc(len, 1);
		uint8_t *r = report;
		*r++ = 1;	// report id
		int n = (int)std::min(touches.size(), (size_t)MAX_CONTACTS);
		for (int i=0;i<MAX_CONTACTS;i++) {
			if (i<n) {
				uint16_t x = (uint16_t)(touches[i].x * DIGITIZER_MAX);
				uint16_t y = (uint16_t)(touches[i].y * DIGITIZER_MAX);
				*r++ = 1;								// tip switch + padding
				*r++ = (uint8_t)touches[i].id;			// contact id
				*r++ = x & 0xFF; *r++ = x >> 8;
				*r++ = y & 0xFF; *r++ = y >> 8;
			} else r += 6;
		}
		*r++ = (uint8_t)n;	// contact count

		AsyncReportCtx *ctx = new AsyncReportCtx;
		ctx->report = report;
		ctx->pending = reportPending;
		IOReturn ret = IOHIDUserDeviceHandleReportAsync(device, report, len, reportDone, ctx);
		if (ret != kIOReturnSuccess) reportDone(ctx, ret);
	}

private:
	IOHIDUserDeviceRef device;
	std::shared_ptr<std::atomic<bool> > reportPending;
};

// ---------------------------------------------------------------------------
// CGEvent fallback backend (mouse + gesture emulation)
// ---------------------------------------------------------------------------

class CGEventInjector : public InjectorBackend {
public:
	CGEventInjector() : mouseDown(false), magnifying(false), prevCount(0),
		prevDist(0), lastPoint(CGPointZero)
	{
		// synthetic events are silently dropped without Accessibility permission
		if (!AXIsProcessTrusted()) {
			const void *keys[] = { kAXTrustedCheckOptionPrompt };
			const void *vals[] = { kCFBooleanTrue };
			CFDictionaryRef opts = CFDictionaryCreate(NULL, keys, vals, 1,
				&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			AXIsProcessTrustedWithOptions(opts);
			CFRelease(opts);
			std::cout << "warning: input injection requires Accessibility permission" << std::endl;
		}
	}

	~CGEventInjector() { releaseAll(); }

	void releaseAll()
	{
		if (magnifying) { postMagnify(0, kCGGesturePhaseEnded, lastPoint); magnifying = false; }
		if (mouseDown)  { postMouse(kCGEventLeftMouseUp, lastPoint); mouseDown = false; }
		prevCount = 0;
	}

	void apply(const std::vector<InjectedTouch>& touches)
	{
		int n = (int)touches.size();

		if (n == 0) {
			releaseAll();
		} else if (n == 1) {
			CGPoint p = toScreen(touches[0]);
			if (magnifying) { postMagnify(0, kCGGesturePhaseEnded, lastPoint); magnifying = false; }
			if (mouseDown) {
				postMouse(kCGEventLeftMouseDragged, p);
			} else {
				postMouse(kCGEventMouseMoved, p);
				if (prevCount == 0) {	// fresh touch -> click, not after gesture lift
					postMouse(kCGEventLeftMouseDown, p);
					mouseDown = true;
				}
			}
			lastPoint = p;
		} else if (n == 2) {
			if (mouseDown) { postMouse(kCGEventLeftMouseUp, lastPoint); mouseDown = false; }
			CGPoint a = toScreen(touches[0]);
			CGPoint b = toScreen(touches[1]);
			CGPoint c = CGPointMake((a.x+b.x)/2, (a.y+b.y)/2);
			float d = hypotf(b.x-a.x, b.y-a.y);
			if (!magnifying) {
				magnifying = true;
				postMagnify(0, kCGGesturePhaseBegan, c);
			} else {
				if (prevDist > 0) postMagnify((d - prevDist)/prevDist, kCGGesturePhaseChanged, c);
				float dx = c.x - prevCentroid.x, dy = c.y - prevCentroid.y;
				if (fabsf(dx) > 0.5f || fabsf(dy) > 0.5f) postScroll(-dy, -dx);
			}
			prevDist = d; prevCentroid = c; lastPoint = c;
		} else {
			releaseAll();
			prevCount = n;	// keep tracking only
			return;
		}
		prevCount = n;
	}

private:
	bool mouseDown, magnifying;
	int prevCount;
	float prevDist;
	CGPoint prevCentroid, lastPoint;

	static CGPoint toScreen(const InjectedTouch& t)
	{
		CGRect b = CGDisplayBounds(CGMainDisplayID());
		return CGPointMake(b.origin.x + t.x * b.size.width,
						   b.origin.y + t.y * b.size.height);
	}

	static void postMouse(CGEventType type, CGPoint p)
	{
		CGEventRef e = CGEventCreateMouseEvent(NULL, type, p, kCGMouseButtonLeft);
		if (type==kCGEventLeftMouseDown || type==kCGEventLeftMouseUp || type==kCGEventLeftMouseDragged)
			CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
		CGEventPost(kCGHIDEventTap, e);
		CFRelease(e);
	}

	static void postScroll(float dy, float dx)
	{
		CGEventRef e = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitPixel, 2, (int32_t)dy, (int32_t)dx);
		CGEventPost(kCGHIDEventTap, e);
		CFRelease(e);
	}

	// Undocumented encoding WindowServer decodes as NSEventTypeGesture/magnify:
	// event type 29, field 110 = kIOHIDEventTypeZoom (8), field 113 = magnification
	// delta, field 132 = CGGesturePhase. May silently stop working on OS updates.
	static void postMagnify(double mag, CGGesturePhase phase, CGPoint p)
	{
		CGEventRef e = CGEventCreate(NULL);
		CGEventSetType(e, (CGEventType)29);
		CGEventSetIntegerValueField(e, (CGEventField)110, 8);
		CGEventSetIntegerValueField(e, (CGEventField)132, (int64_t)phase);
		CGEventSetDoubleValueField(e, (CGEventField)113, mag);
		CGEventSetLocation(e, p);
		CGEventPost(kCGHIDEventTap, e);
		CFRelease(e);
	}
};

// ---------------------------------------------------------------------------
// TouchInjector : TuioListener
// ---------------------------------------------------------------------------

TouchInjector::TouchInjector() : backend(NULL), verbose(false)
{
	DigitizerInjector *digitizer = new DigitizerInjector();
	if (digitizer->isValid()) {
		backend = digitizer;
	} else {
		delete digitizer;
		backend = new CGEventInjector();
		std::cout << "virtual device unavailable, using CGEvent fallback" << std::endl;
	}
}

TouchInjector::~TouchInjector()
{
	delete backend;
}

bool TouchInjector::usingVirtualDevice() const
{
	return dynamic_cast<DigitizerInjector*>(backend) != NULL;
}

int TouchInjector::slotFor(int session_id)
{
	std::map<int,int>::iterator it = slots.find(session_id);
	if (it != slots.end()) return it->second;
	for (int i=0;i<MAX_CONTACTS;i++) {
		bool taken = false;
		for (it=slots.begin();it!=slots.end();it++) if (it->second==i) { taken = true; break; }
		if (!taken) { slots[session_id] = i; return i; }
	}
	return -1;
}

void TouchInjector::addTuioCursor(TUIO::TuioCursor *tcur)
{
	int slot = slotFor(tcur->getSessionID());
	if (slot < 0) return;
	if (verbose) std::cout << "cursor " << tcur->getSessionID() << " added as contact "
		<< slot << " (" << tcur->getX() << "," << tcur->getY() << ")" << std::endl;
	InjectedTouch t = { slot, tcur->getX(), tcur->getY() };
	active.push_back(t);
}

void TouchInjector::updateTuioCursor(TUIO::TuioCursor *tcur)
{
	std::map<int,int>::iterator it = slots.find(tcur->getSessionID());
	if (it == slots.end()) return;
	for (size_t i=0;i<active.size();i++)
		if (active[i].id == it->second) {
			active[i].x = tcur->getX();
			active[i].y = tcur->getY();
			return;
		}
	if (verbose) std::cout << "cursor " << tcur->getSessionID()
		<< " update for inactive contact " << it->second << std::endl;
}

void TouchInjector::removeTuioCursor(TUIO::TuioCursor *tcur)
{
	std::map<int,int>::iterator it = slots.find(tcur->getSessionID());
	if (it == slots.end()) return;
	if (verbose) std::cout << "cursor " << tcur->getSessionID() << " removed from contact "
		<< it->second << std::endl;
	for (size_t i=0;i<active.size();i++)
		if (active[i].id == it->second) { active.erase(active.begin()+i); break; }
	slots.erase(it);
}

void TouchInjector::refresh(TUIO::TuioTime ftime)
{
	if (verbose) {
		std::cout << "frame: " << active.size() << " touch(es)";
		for (size_t i=0;i<active.size();i++)
			std::cout << " #" << active[i].id << "(" << active[i].x << "," << active[i].y << ")";
		std::cout << std::endl;
	}
	backend->apply(active);
}
