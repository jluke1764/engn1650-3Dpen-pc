	//Simple Raw HID functions for Linux - for use with Teensy RawHID example
	//http://www.pjrc.com/teensy/rawhid.html
	//Copyright (c) 2009 PJRC.COM, LLC
	//Version 1.0: Initial Release

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>

#define BUFSIZE 64

	//list of all opened HID devices, so caller can refer to them by number
typedef struct hid_struct hid_t;
typedef struct buf_struct buf_t;
static hid_t *first_hid = 0, *last_hid = 0;
struct hid_struct { IOHIDDeviceRef ref; int open; uint8_t buf[BUFSIZE]; buf_t *first_buf, *last_buf; struct hid_struct *prev, *next; };
struct buf_struct { struct buf_struct *next; uint32_t len; uint8_t buf[BUFSIZE]; };

static void add_hid (hid_t *h)
{
	if (!first_hid || !last_hid) { first_hid = last_hid = h; h->next = h->prev = 0; return; }
	last_hid->next = h; h->prev = last_hid; h->next = 0; last_hid = h;
}

static hid_t *get_hid (int num)
{
	hid_t *p;
	for (p=first_hid;p && (num > 0);p=p->next,num--);
	return p;
}

static void hid_close (hid_t *hid)
{
	if (!hid || !hid->open || !hid->ref) return;
	IOHIDDeviceUnscheduleFromRunLoop(hid->ref,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
	IOHIDDeviceClose(hid->ref,kIOHIDOptionsTypeNone);
	hid->ref = 0;
}

static void free_all_hid (void)
{
	hid_t *p, *q;
	for(p=first_hid;p;p=p->next) { hid_close(p); }
	p = first_hid;
	while (p) { q = p; p = p->next; free(q); }
	first_hid = last_hid = 0;
}

static void detach_callback (void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
	hid_t *p;
	for(p=first_hid;p;p=p->next)
		{ if (p->ref == dev) { p->open = 0; CFRunLoopStop(CFRunLoopGetCurrent()); return; } }
}

static void input_callback (void *context, IOReturn ret, void *sender, IOHIDReportType type, uint32_t id, uint8_t *data, CFIndex len)
{
	buf_t *n;
	hid_t *hid;

	if (ret != kIOReturnSuccess || len < 1) return;
	hid = context;
	if (!hid || hid->ref != sender) return;
	n = (buf_t *)malloc(sizeof(buf_t));
	if (!n) return;
	if (len > BUFSIZE) len = BUFSIZE;
	memcpy(n->buf,data,len);
	n->len = len;
	n->next = 0;
	if (!hid->first_buf || !hid->last_buf) { hid->first_buf = hid->last_buf = n; }
												 else { hid->last_buf->next = n; hid->last_buf = n; }
	CFRunLoopStop(CFRunLoopGetCurrent());
}

static void attach_callback (void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
	struct hid_struct *h;

	if (IOHIDDeviceOpen(dev,kIOHIDOptionsTypeNone) != kIOReturnSuccess) return;
	h = (hid_t *)malloc(sizeof(hid_t));
	if (!h) return;
	memset(h,0,sizeof(hid_t));
	IOHIDDeviceScheduleWithRunLoop(dev,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
	IOHIDDeviceRegisterInputReportCallback(dev,h->buf,sizeof(h->buf),input_callback,h);
	h->ref = dev;
	h->open = 1;
	add_hid(h);
}

static void timeout_callback (CFRunLoopTimerRef timer, void *info)
	{ *(int *)info = 1; CFRunLoopStop(CFRunLoopGetCurrent()); }

//--------------------------------------------

int rawhid_send (void *buf, int len) //Ret: #by sent, or -1
{
	hid_t *hid = get_hid(0);
	if (!hid || !hid->open) return -1;
	IOReturn ret = IOHIDDeviceSetReport(hid->ref,kIOHIDReportTypeOutput,0,buf,len);
	return (ret == kIOReturnSuccess) ? len : -1;
}

int rawhid_recv (void *buf, int len) //Ret:#by recv, or -1
{
	hid_t *hid;
	buf_t *b;
	CFRunLoopTimerRef timer = 0;
	CFRunLoopTimerContext context;
	int ret = 0, timeout_occurred = 0;

	if (len < 1) return 0;
	hid = get_hid(0);
	if (!hid || !hid->open) return -1;
	if ((b = hid->first_buf))
	{
		if (len > b->len) len = b->len;
		memcpy(buf,b->buf,len);
		hid->first_buf = b->next;
		free(b);
		return len;
	}
	memset(&context,0,sizeof(context));
	context.info = &timeout_occurred;
	timer = CFRunLoopTimerCreate(0,CFAbsoluteTimeGetCurrent(),0,0,0,timeout_callback,&context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(),timer,kCFRunLoopDefaultMode);
	while (1)
	{
		CFRunLoopRun();
		if ((b = hid->first_buf))
		{
			if (len > b->len) len = b->len;
			memcpy(buf,b->buf,len);
			hid->first_buf = b->next;
			free(b);
			ret = len;
			break;
		}
		if (!hid->open) { printf("rawhid_recv:dev not open\n"); ret = -1; break; }
		if (timeout_occurred) break;
	}
	CFRunLoopTimerInvalidate(timer);
	CFRelease(timer);
	return ret;
}

void rawhid_close (void)
{
	hid_t *hid = get_hid(0);
	if (!hid || !hid->open) return;
	hid_close(hid);
	hid->open = 0;
}

	//vid: Vend ID; -1=any
	//pid: Prod ID; -1=any
	//uspag: top level usage page; -1=any
	//usnum: top level usage #; -1=any
	//Ret: actual # devs opened
int rawhid_open (int vid, int pid, int uspag, int usnum)
{
	static IOHIDManagerRef hid_manager = 0;
	CFMutableDictionaryRef dict;
	CFNumberRef num;
	IOReturn ret;
	hid_t *p;
	int cnt = 0;

	if (first_hid) free_all_hid();

		  //Start HID Manager (http://developer.apple.com/technotes/tn2007/tn2187.html)
	if (!hid_manager)
	{
		hid_manager = IOHIDManagerCreate(kCFAllocatorDefault,kIOHIDOptionsTypeNone);
		if (hid_manager == 0 || CFGetTypeID(hid_manager) != IOHIDManagerGetTypeID()) { if (hid_manager) CFRelease(hid_manager); return 0; }
	}
	if (vid > 0 || pid > 0 || uspag > 0 || usnum > 0)
	{
			//Tell HID Manager what type of devices we want
		dict = CFDictionaryCreateMutable(kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks); if (!dict) return 0;
		if (vid   > 0) { num = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&vid  ); CFDictionarySetValue(dict,CFSTR(kIOHIDVendorIDKey        ),num); CFRelease(num); }
		if (pid   > 0) { num = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&pid  ); CFDictionarySetValue(dict,CFSTR(kIOHIDProductIDKey       ),num); CFRelease(num); }
		if (uspag > 0) { num = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&uspag); CFDictionarySetValue(dict,CFSTR(kIOHIDPrimaryUsagePageKey),num); CFRelease(num); }
		if (usnum > 0) { num = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&usnum); CFDictionarySetValue(dict,CFSTR(kIOHIDPrimaryUsageKey    ),num); CFRelease(num); }
		IOHIDManagerSetDeviceMatching(hid_manager,dict); CFRelease(dict);
	}
	else { IOHIDManagerSetDeviceMatching(hid_manager,0); }

		//set up callbacks for device attach & detach
	IOHIDManagerScheduleWithRunLoop(hid_manager,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
	IOHIDManagerRegisterDeviceMatchingCallback(hid_manager,attach_callback,0);
	IOHIDManagerRegisterDeviceRemovalCallback(hid_manager,detach_callback,0);
	ret = IOHIDManagerOpen(hid_manager,kIOHIDOptionsTypeNone);
	if (ret != kIOReturnSuccess) { IOHIDManagerUnscheduleFromRunLoop(hid_manager,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode); CFRelease(hid_manager); return 0; }

	while (CFRunLoopRunInMode(kCFRunLoopDefaultMode,0,1) == kCFRunLoopRunHandledSource) {} //let it do callback for all devs
	for(p=first_hid;p;p=p->next) cnt++; //count # added by callback
	return cnt;
}
