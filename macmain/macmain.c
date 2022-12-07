#if 0
macmain: macmain.c; gcc -x objective-c macmain.c -O2 -o macmain -framework Cocoa -DSTANDALONE=1
ifdef zero
#endif

	//11/25/2022: Fantastic minimalistic SDL-type example found for MacOS!
	//https://hero.handmade.network/forums/code-discussion/t/1409-main_game_loop_on_os_x
	//https://stackoverflow.com/questions/38768645/custom-main-application-loop-in-cocoa/38768954?noredirect=1#comment64910793_38768954
	//https://stackoverflow.com/questions/7560979/cgcontextdrawimage-is-extremely-slow-after-large-uiimage-drawn-into-it

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#include <stdint.h>

typedef uint8_t uint8;

#if !defined(max)
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min)
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

//-----------------------------------
#define INT_PTR ptrdiff_t
typedef struct { INT_PTR f, p, x, y; } tiletype;

char keystatus[256] = {0};
int xres = 640, yres = 480, mousx = 0, mousy = 0, bstatus = 0, dmousx = 0, dmousy = 0, dmousz = 0, ActiveApp = 1;
const char *prognam = "KMacMain app";

int showwindmode = 1;
int viewpage, rendpage, numpages; //Hack to make it compilable

#define KEYBUFSIZ 256
int keybuf[KEYBUFSIZ], keybufr = 0, keybufw = 0;
//-----------------------------------
static int quitit = 1;
static int *pic, *pic2, *mypic;
//-----------------------------------

@class View;
@class AppDelegate;
@class WindowDelegate;
static AppDelegate *appDelegate;
static NSWindow *window;
static View *view;
static WindowDelegate *windowDelegate;

@interface AppDelegate: NSObject <NSApplicationDelegate> {}
@end

@implementation AppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
	{ quitit = 1; return NSTerminateCancel; } //Prevent Cocoa from killing app immediately
@end

@interface WindowDelegate : NSObject <NSWindowDelegate> {}
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(id)sender { quitit = 1; return YES; }

- (void)windowDidBecomeKey: (NSNotification *)notification { NSLog(@"Window: become key"); }
- (void)windowDidBecomeMain:(NSNotification *)notification { NSLog(@"Window: become main"); }
- (void)windowDidResignKey: (NSNotification *)notification { NSLog(@"Window: resign key"); }
- (void)windowDidResignMain:(NSNotification *)notification { NSLog(@"Window: resign main"); }

- (void)windowWillClose:(NSNotification *)notification { if (!quitit) { quitit = 1; [NSApp terminate:self]; } }
//- (void)windowWillClose:(NSNotification *)notification { Window *window = notification.object; if (window.isMainWindow) { [NSApp terminate:nil]; } } //Terminate app when main window closed

@end

#define FUKNEW 0

@interface View : NSView <NSWindowDelegate>
{
@public
	CGContextRef backbuf;
	CGColorSpaceRef colorSpace;
#if (FUKNEW != 0)
	CGContextRef backbuf2;
	CGRect layerRect;
	CGLayerRef layer;
#endif
}
- (instancetype)initWithFrame:(NSRect)frameRect;
- (void)drawRect:(NSRect)dirtyRect;
@end

@implementation View
- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if (self)
	{
#if 0
		colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB); //slower
#elif 1
		colorSpace = CGColorSpaceCreateDeviceRGB(); //slower
#else
		CMProfileRef prof; //slightly faster than ^
		CMGetSystemProfile(&prof);
		colorSpace = CGColorSpaceCreateWithPlatformColorSpace(prof);
		CMCloseProfile(prof);
#endif

		mypic = (int *)malloc(xres*yres*4+4096); mypic = (int *)((((INT_PTR)mypic)+4095)&-4096);

#if 0
		pic = (int *)mmap(0,xres*yres*4,PROT_WRITE|PROT_READ,MAP_ANON|MAP_PRIVATE,-1,0);
	 //pic = (int *)malloc(xres*yres*4+4096); pic = (int *)((((INT_PTR)pic)+4095)&-4096); //same as ^
		backbuf = CGBitmapContextCreate((void *)pic,xres,yres,8,xres*4,colorSpace,kCGImageAlphaNoneSkipFirst|kCGImageByteOrder32Little);
#else
	 //backbuf = CGBitmapContextCreate(0,xres,yres,8,xres*4,colorSpace,kCGImageAlphaNoneSkipFirst|kCGImageByteOrder32Little);
		backbuf = CGBitmapContextCreate(0,xres,yres,8,xres*4,colorSpace,kCGImageAlphaPremultipliedFirst|kCGImageByteOrder32Little); //blit faster than AlphaNoneSkipFirst, but req all pixels: |= 0xff000000; :/
		pic = (int *)CGBitmapContextGetData(backbuf);
#endif

		//CGColorSpaceRelease(colorSpace);
#if (FUKNEW != 0)
		backbuf2 = CGBitmapContextCreate(0,xres,yres,8,xres*4,colorSpace,kCGImageAlphaPremultipliedFirst|kCGImageByteOrder32Little);
		CGContextSetInterpolationQuality(backbuf2,kCGInterpolationNone);
		pic2 = (int *)CGBitmapContextGetData(backbuf2);

		layerRect = CGRectMake(0,0,xres,yres);
		layer = CGLayerCreateWithContext(backbuf,layerRect.size,0);
#endif
	}
	return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
	CGContextRef gctx = [[NSGraphicsContext currentContext] CGContext];
	CGContextSetInterpolationQuality(gctx,kCGInterpolationNone); //none, low, medium, high   none=~2x faster!

#if (FUKNEW == 0)
	for(int i=xres*yres-1;i>=0;i--) pic[i] = mypic[i]|0xff000000; //damn damn stupid slow

	CGImageRef backImage = CGBitmapContextCreateImage(backbuf);
	CGContextDrawImage(gctx,CGRectMake(0,0,xres,yres),backImage);
	CGImageRelease(backImage);

	//NSBitmapImageRep *img = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:pic pixelsWide:xres pixelsHigh:yres bitsPerSample:8
	//   samplesPerPixel:4 hasAlpha:0 isPlanar:0 colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:xres*4 bitsPerPixel:32];
	//NSImage *Image = [[NSImage alloc] init];
	//[Image addRepresentation:img];
	//[NSImageView setImage:Image];
	//[img release];

#elif 0
	//CGRect layerRect = CGRectMake(0,0,xres,yres);
	//CGLayerRef layer = CGLayerCreateWithContext(gctx,layerRect.size,0);

	CGContextRef lctx = CGLayerGetContext(layer);

	//for(int i=xres*yres-1;i>=0;i--) pic2[i] = mypic[i]|0xff000000; //damn damn stupid slow

	//CGImageRef bitmapImageRef = CGBitmapContextCreateImage(backbuf2);
	//CGContextDrawImage(backbuf /*lctx*/,layerRect,bitmapImageRef);

	//int *wptr = CGBitmapContextGetData(lctx); //backbuf); //FUKFUKFUKFUK
	for(int i=xres*yres-1;i>=0;i--) pic2[i] = mypic[i]|0xff000000; //damn damn stupid slow

	//CGImageRelease(bitmapImageRef);


	CGContextDrawLayerAtPoint(gctx,CGPointZero,layer);
	//CGLayerRelease(layer);
#else
	CGLayerRef layer = CGLayerCreateWithContext(backbuf,CGSizeMake(xres,yres),0);
	CGContextRef lctx = CGLayerGetContext(layer);

	int *wptr = CGBitmapContextGetData(lctx);
	for(int i=xres*yres-1;i>=0;i--) wptr[i] = mypic[i]|0xff000000; //damn damn stupid slow

	CGContextDrawLayerAtPoint(backbuf,CGPointMake(0,0),layer);
	CGLayerRelease(layer);

	CGImageRef backImage = CGBitmapContextCreateImage(backbuf);
	CGContextDrawImage(gctx,CGRectMake(0,0,xres,yres),backImage);
	CGImageRelease(backImage);
#endif
}

@end

	//msgbox("Title","Hi *","OK");
int msgbox (const char *tit, const char *txt, const char *butxt)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[NSApplication sharedApplication];
	int i = NSRunAlertPanel(@(tit),@(txt),@(butxt),0,0);
	[pool release];
	return(i); //<-FUK!
}

	//Issues:
	//* is return string on stack? i.e. safe to return?
	//* wildst: must format as "c" instead of "*.c": incompatible w/win&lin code :/
const char *file_select (int flags, const char *txt, const char *def, const char *wildst)
{
	NSOpenPanel *dlg = [NSOpenPanel openPanel];
	[dlg setTitle:@"Open File"];
	[dlg setCanChooseFiles:1];
	[dlg setCanChooseDirectories:0];
	[dlg setAllowsMultipleSelection:0];
	[dlg setAllowedFileTypes:[NSArray arrayWithObjects:@(wildst), nil]];
							  //^":[NSArray arrayWithObjects:@"txt", @"doc", nil]; //<- example: multiple filetypes
	if ([dlg runModal] != NSModalResponseOK) return 0;

	NSString *myst = [NSString stringWithFormat:@"%@", [[dlg URLs] objectAtIndex:0/*<-1,.. if multiple*/]];
	return [myst UTF8String]; //is this on stack? safe?
}

void putclip (char *st, int leng)
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	[pasteboard setString:[NSString stringWithUTF8String:st] forType:NSPasteboardTypeString];
}

	//NOTE:assumes caller will free(buf); if return != 0
int getclip (char **buf)
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSString *available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:NSPasteboardTypeString]];
	if (![available isEqualToString:NSPasteboardTypeString]) return 0;

	NSString *string = [pasteboard stringForType:NSPasteboardTypeString];
	if (string == nil) return 0;

	const char *string_c = (const char *)[string UTF8String];
	int stleng = (int)strlen(string_c);
	(*buf) = malloc(stleng+1);
	strcpy(*buf,string_c);
	return(stleng);
}

static double klock0 = 0.0;
double klock (void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC,&t);
	return((double)t.tv_sec + ((double)t.tv_nsec)*1e-9 - klock0);
}

void quitloop (void) { quitit = 1; }

int keyread (void)
{
	int i;
	if (keybufr == keybufw) return(0);
	i = keybuf[keybufr&(KEYBUFSIZ-1)]; keybufr = (keybufr+1)&(KEYBUFSIZ-1);
	return(i);
}

void uninitapp (void)
{
	//FUK:memory leaks!
}

static int gcapslock = 0;
int breath (void)
{
		//int mackeys[128] =
		//{
		//   KEY_A,   KEY_S,        KEY_D,    KEY_F,    KEY_H,     KEY_G,   KEY_Z,  KEY_X,       KEY_C,      KEY_V,         0,             KEY_B,      KEY_Q,     KEY_W,    KEY_E,        KEY_R,
		//   KEY_Y,   KEY_T,        KEY_1,    KEY_2,    KEY_3,     KEY_4,   KEY_6,  KEY_5,       KEY_EQUALS, KEY_9,         KEY_7,         KEY_MINUS,  KEY_8,     KEY_0,    KEY_RBRACKET, KEY_O,
		//   KEY_U,   KEY_LBRACKET, KEY_I,    KEY_P,    KEY_ENTER, KEY_L,   KEY_J,  KEY_QUOTE,   KEY_K,      KEY_SEMICOLON, KEY_BACKSLASH, KEY_COMMA,  KEY_SLASH, KEY_N,    KEY_M,        KEY_PERIOD,
		//   KEY_TAB, KEY_SPACE,    KEY_TICK, KEY_BACK, 0,         KEY_ESC, 0,      KEY_LMETA,   KEY_LSHIFT, KEY_CAPS,      KEY_LALT,      KEY_LCTRL,  0,         0,        0,            0,
		//   0,       KP_PERIOD,    0,        KP_STAR,  0,         KP_PLUS, 0,      KEY_NUMLOCK, 0,          0,             0,             KP_DIVIDE,  KPENTER,   0,        KPMINUS,      0,
		//   0,       0,            KP0,      KP1,      KP2,       KP3,     KP4,    KP5,         KP6,        KP7,           0,             KP8,        KP9,       0,        0,            0,
		//   KEY_F5,  KEY_F6,       KEY_F7,   KEY_F3,   KEY_F8,    KEY_F9,  0,      KEY_F11,     0,          KEY_PRINT,     0,             KEY_SCROLL, 0,         KEY_F10,  0,            KEY_F12,
		//   0,       KEY_PAUSE,    KEY_INS,  KEY_HOME, KEY_PGUP,  KEY_DEL, KEY_F4, KEY_END,     KEY_F2,     KEY_PGDN,      KEY_F1,        KEY_LEFT,   KEY_RIGHT, KEY_DOWN, KEY_UP,       0
		//};
	static const unsigned char macscan2asc[2][128] =
	{
		'a','s','d','f','h','g','z','x','c','v', 0 ,'b','q','w','e','r',
		'y','t','1','2','3','4','6','5','=','9','7','-','8','0',']','o',
		'u','[','i','p', 13,'l','j', 39,'k',';', 92,',','/','n','m','.',
		 9 ,' ','`', 8 , 0, 27 , 0 ,255,255,255,255,255, 0 , 0 , 0 , 0 ,
		 0 ,'.', 0 ,'*', 0 ,'+', 0 ,255, 0 , 0 , 0 ,'/', 13, 0 ,'-', 0 ,
		 0 , 0 ,'0','1','2','3','4','5','6','7', 0 ,'8','9', 0 , 0 , 0 ,
		255,255,255,255,255,255, 0 ,255, 0 ,255, 0 ,255, 0 ,255, 0 ,255,
		 0 ,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 0 ,

			//shift
		'A','S','D','F','H','G','Z','X','C','V', 0 ,'B','Q','W','E','R',
		'Y','T','!','@','#','$','^','%','+','(','&','_','*',')','}','O',
		'U','{','I','P',255,'L','J', 34,'K',':','|','<','?','N','M','>',
		255,' ','~', 8 , 0, 255, 0 ,255,255,255,255,255, 0 , 0 , 0 , 0 ,
		 0 ,255, 0 ,255, 0 ,255, 0 ,255, 0 , 0 , 0 ,255,255, 0 ,255, 0 ,
		 0 , 0 ,255,255,255,255,255,255,255,255, 0 ,255,255, 0 , 0 , 0 ,
		255,255,255,255,255,255, 0 ,255, 0 ,255, 0 ,255, 0 ,255, 0 ,255,
		 0 ,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 0 ,
	};
	static const unsigned char pcscan[128] =
	{
		0x1e,0x1f,0x20,0x21,0x23,0x22,0x2c,0x2d,0x2e,0x2f, 0  ,0x30,0x10,0x11,0x12,0x13,
		0x15,0x14,0x02,0x03,0x04,0x05,0x07,0x06,0x0d,0x0a,0x08,0x0c,0x09,0x0b,0x1b,0x18,
		0x16,0x1a,0x17,0x19,0x1c,0x26,0x24,0x28,0x25,0x27,0x2b,0x33,0x35,0x31,0x32,0x34,
		0x0f,0x39,0x29,0x0e, 0  ,0x01, 0  ,0xdb,0x2a,0x3a,0x38,0x1d, 0  , 0  , 0  , 0  ,
		 0  ,0x53, 0  ,0x37, 0  ,0x4e, 0  ,0x45, 0  , 0  , 0  ,0xb5,0x9c, 0  ,0x4a, 0  ,
		 0  , 0  ,0x52,0x4f,0x50,0x51,0x4b,0x4c,0x4d,0x47, 0  ,0x48,0x49, 0  , 0  , 0  ,
		0x3f,0x40,0x41,0x3d,0x42,0x43, 0  ,0x57, 0  ,0xb7, 0  ,0x46, 0  ,0x44, 0  ,0x58,
		 0  ,0xc5,0xd2,0xc7,0xc9,0xd3,0x3e,0xcf,0x3c,0xd1,0x3b,0xcb,0xcd,0xd0,0xc8, 0  ,
	};

	@autoreleasepool
	{
		NSEvent *ev;
		int i;
		do
		{
			ev = [NSApp nextEventMatchingMask: NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
			if (ev)
			{
				switch (ev.type)
				{
					case NSEventTypeKeyDown: case NSEventTypeKeyUp:
						if ((ev.modifierFlags&NSEventModifierFlagCommand) && (ev.keyCode == 12)) //Handle cmd+q
							{ [NSApp sendEvent:ev]; break; }

						if ((unsigned int)ev.keyCode >= 128u) break;
						keystatus[pcscan[ev.keyCode]] = (ev.type == NSEventTypeKeyDown);

						if (ev.type == NSEventTypeKeyDown)
						{
							i = ((ev.modifierFlags&NSEventModifierFlagShift) != 0);
							if ((ev.modifierFlags&NSEventModifierFlagCapsLock) && (macscan2asc[0][ev.keyCode] >= 'a') && (macscan2asc[0][ev.keyCode] <= 'z')) i ^= 1;

							i = macscan2asc[i][ev.keyCode];
							if (i != 0)
							{
								if (i == 255) i = 0;
								keybuf[keybufw&(KEYBUFSIZ-1)] = i | (((int)pcscan[ev.keyCode])<<8) |
									((((ev.modifierFlags&NSEventModifierFlagShift  ) != 0)*3) << 16) |
									((((ev.modifierFlags&NSEventModifierFlagControl) != 0)*3) << 18) |
									((((ev.modifierFlags&NSEventModifierFlagOption ) != 0)*3) << 20) |
									((((ev.modifierFlags&NSEventModifierFlagCommand) != 0)*3) << 20);

								keybufw = (keybufw+1)&(KEYBUFSIZ-1);
							}
						}
						//printf("%c (%02x)\n",macscan2asc[0][ev.keyCode],pcscan[ev.keyCode]);
						break;
					case NSEventTypeMouseMoved: case NSEventTypeLeftMouseDragged:
						mousx = ev.locationInWindow.x;
						mousy = view.bounds.size.height - ev.locationInWindow.y;
						break;
					case NSEventTypeScrollWheel: if (ev.hasPreciseScrollingDeltas) dmousz += ev.scrollingDeltaY;
																								 else dmousz += ev.scrollingDeltaY*10;
						break;
					case NSEventTypeLeftMouseDown:  bstatus |= 1; break;
					case NSEventTypeLeftMouseUp:    bstatus &=~1; break;
					case NSEventTypeRightMouseDown: bstatus |= 2; break;
					case NSEventTypeRightMouseUp:   bstatus &=~2; break;
					case NSEventTypeOtherMouseDown: bstatus |= 4; break;
					case NSEventTypeOtherMouseUp:   bstatus &=~4; break;
					default: [NSApp sendEvent:ev]; break; //Handle events like app focus/unfocus etc
				}
			}
		} while (ev);
	}
	return(quitit);
}

void win_settitle (const char *st) { [window setTitle:@(st)]; }

int initapp (void)
{
	[NSAutoreleasePool new];
	[NSApplication sharedApplication];
	appDelegate = [[AppDelegate alloc] init];
	[NSApp setDelegate:appDelegate];
	quitit = 0;
	[NSApp finishLaunching];

		//createWindow:
	NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

	NSRect screenRect = [[NSScreen mainScreen] frame];
	NSRect viewRect = NSMakeRect(0,0,xres,yres);
	NSRect windowRect = NSMakeRect(NSMidX(screenRect)-NSMidX(viewRect),
											 NSMidY(screenRect)-NSMidY(viewRect),
											 viewRect.size.width,
											 viewRect.size.height);

	window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];

	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	id menubar = [[NSMenu new] autorelease];
	id appMenuItem = [[NSMenuItem new] autorelease];
	[menubar addItem:appMenuItem];
	[NSApp setMainMenu:menubar];

		//Add quit item to menu; simple since terminate already implemented in NSApplication and NSApplication always in responder chain.
	id appMenu = [[NSMenu new] autorelease];
	id appName = @(prognam); //[[NSProcessInfo processInfo] processName];
	id quitTitle = [@"Quit " stringByAppendingString:appName];
	id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
	[appMenu addItem:quitMenuItem];
	[appMenuItem setSubmenu:appMenu];

	NSWindowController *windowController = [[NSWindowController alloc] initWithWindow:window];
	[windowController autorelease];

	view = [[[View alloc] initWithFrame:viewRect] autorelease];
	[window setContentView:view];

	windowDelegate = [[WindowDelegate alloc] init];
	[window setDelegate:windowDelegate];
	[window setAcceptsMouseMovedEvents:YES];
	[window setDelegate:view];
	[window setTitle:@(prognam)]; //:appName];
	[window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
	[window makeKeyAndOrderFront:nil];
	//[window cascadeTopLeftFromPoint:NSMakePoint(20,20)]; //set windows pos (top-left corner)

	[NSApp activateIgnoringOtherApps:YES]; //start in foreground

	klock0 = klock();

	return(1);
}

int startdirectdraw (INT_PTR *f, INT_PTR *p, INT_PTR *x, INT_PTR *y) { (*f) = (INT_PTR)mypic; (*p) = xres*4; (*x) = xres; (*y) = yres; return(1); }
void stopdirectdraw (void) {}
void nextpage (void) { [view setNeedsDisplay:1]; }

	//NOTE: font is stored vertically first!
static const unsigned int font6x8[] = //256 DOS chars, from: DOSAPP.FON (tab blank)
{
	0x00000000,0x3E000000,0x3E455145,0x6F6B3E00,0x1C003E6B,0x1C3E7C3E,0x7E3C1800,0x3000183C,0x30367F36,0x7E5C1800,0x0000185C,0x00001818,0xE7E7FFFF,0x0000FFFF,0x00000000,0xDBDBC3FF,
	0x3000FFC3,0x0E364A48,0x79290600,0x60000629,0x04023F70,0x0A7E6000,0x2A003F35,0x2A1C361C,0x3E7F0000,0x0800081C,0x007F3E1C,0x7F361400,0x00001436,0x005F005F,0x7F090600,0x22007F01,
	0x2259554D,0x60606000,0x14000060,0x14B6FFB6,0x7F060400,0x10000406,0x10307F30,0x3E080800,0x0800081C,0x08083E1C,0x40407800,0x08004040,0x083E083E,0x3F3C3000,0x0300303C,0x030F3F0F,
	0x00000000,0x00000000,0x00065F06,0x00030700,0x24000307,0x247E247E,0x6A2B2400,0x63000012,0x63640813,0x56493600,0x00005020,0x00000307,0x413E0000,0x00000000,0x00003E41,0x1C3E0800,
	0x0800083E,0x08083E08,0x60E00000,0x08000000,0x08080808,0x60600000,0x20000000,0x02040810,0x49513E00,0x00003E45,0x00407F42,0x49516200,0x22004649,0x36494949,0x12141800,0x2F00107F,
	0x31494949,0x494A3C00,0x01003049,0x03050971,0x49493600,0x06003649,0x1E294949,0x6C6C0000,0x00000000,0x00006CEC,0x22140800,0x24000041,0x24242424,0x22410000,0x02000814,0x06095901,
	0x5D413E00,0x7E001E55,0x7E111111,0x49497F00,0x3E003649,0x22414141,0x41417F00,0x7F003E41,0x41494949,0x09097F00,0x3E000109,0x7A494941,0x08087F00,0x00007F08,0x00417F41,0x40403000,
	0x7F003F40,0x41221408,0x40407F00,0x7F004040,0x7F020402,0x04027F00,0x3E007F08,0x3E414141,0x09097F00,0x3E000609,0x5E215141,0x09097F00,0x26006619,0x32494949,0x7F010100,0x3F000101,
	0x3F404040,0x40201F00,0x3F001F20,0x3F403C40,0x08146300,0x07006314,0x07087008,0x45497100,0x00000043,0x0041417F,0x08040200,0x00002010,0x007F4141,0x01020400,0x80800402,0x80808080,
	0x07030000,0x20000000,0x78545454,0x44447F00,0x38003844,0x28444444,0x44443800,0x38007F44,0x08545454,0x097E0800,0x18000009,0x7CA4A4A4,0x04047F00,0x00000078,0x00407D00,0x84804000,
	0x7F00007D,0x00442810,0x7F000000,0x7C000040,0x78041804,0x04047C00,0x38000078,0x38444444,0x4444FC00,0x38003844,0xFC444444,0x44784400,0x08000804,0x20545454,0x443E0400,0x3C000024,
	0x007C2040,0x40201C00,0x3C001C20,0x3C603060,0x10106C00,0x9C00006C,0x003C60A0,0x54546400,0x0800004C,0x0041413E,0x77000000,0x00000000,0x083E4141,0x02010200,0x3C000001,0x3C262326,
	0xE1A11E00,0x3D001221,0x007D2040,0x54543800,0x20000955,0x78555555,0x54552000,0x20007855,0x78545555,0x55572000,0x1C007857,0x1422E2A2,0x55553800,0x38000855,0x08555455,0x55553800,
	0x00000854,0x00417C01,0x79020000,0x00000042,0x00407C01,0x24297000,0x78007029,0x782F252F,0x54547C00,0x34004555,0x58547C54,0x7F097E00,0x38004949,0x00394545,0x44453800,0x39000039,
	0x00384445,0x21413C00,0x3D00007D,0x007C2041,0x60A19C00,0x3D00003D,0x003D4242,0x40413C00,0x1800003D,0x00246624,0x493E4800,0x29006249,0x292A7C2A,0x16097F00,0x40001078,0x02097E88,
	0x55542000,0x00007855,0x00417D00,0x45443800,0x3C000039,0x007D2140,0x0A097A00,0x7A000071,0x00792211,0x55550800,0x4E005E55,0x004E5151,0x4D483000,0x3C002040,0x04040404,0x04040404,
	0x17001C04,0x506A4C08,0x34081700,0x0000782A,0x00307D30,0x00140800,0x14001408,0x08140008,0x11441144,0x55AA1144,0x55AA55AA,0xEEBBEEBB,0x0000EEBB,0x0000FF00,0xFF080808,0x0A0A0000,
	0x0000FF0A,0xFF00FF08,0xF8080000,0x0000F808,0xFE0A0A0A,0xFB0A0000,0x0000FF00,0xFF00FF00,0xFA0A0000,0x0000FE02,0x0F080B0A,0x0F080000,0x00000F08,0x0F0A0A0A,0x08080000,0x0000F808,
	0x0F000000,0x08080808,0x08080F08,0xF8080808,0x00000808,0x0808FF00,0x08080808,0x08080808,0x0808FF08,0xFF000000,0xFF000A0A,0x0808FF00,0x0B080F00,0xFE000A0A,0x0A0AFA02,0x0B080B0A,
	0xFA0A0A0A,0x0A0AFA02,0xFB00FF00,0x0A0A0A0A,0x0A0A0A0A,0xFB00FB0A,0x0A0A0A0A,0x0A0A0B0A,0x0F080F08,0x0A0A0808,0x0A0AFA0A,0xF808F808,0x0F000808,0x08080F08,0x0F000000,0x00000A0A,
	0x0A0AFE00,0xF808F800,0xFF080808,0x0808FF00,0xFB0A0A0A,0x08080A0A,0x00000F08,0xF8000000,0xFFFF0808,0xFFFFFFFF,0xF0F0F0F0,0xFFFFF0F0,0x000000FF,0xFF000000,0x0F0FFFFF,0x0F0F0F0F,
	0x24241800,0xFE002418,0x00344A4A,0x01017F00,0x02000003,0x027E027E,0x49556300,0x18000063,0x041C2424,0x2020FC00,0x0800001C,0x00047804,0x77550800,0x3E000855,0x003E4949,0x02724C00,
	0x22004C72,0x00305955,0x18241800,0x18001824,0x18247E24,0x2A2A1C00,0x3C00002A,0x003C0202,0x2A2A2A00,0x0000002A,0x00242E24,0x4A4A5100,0x44000044,0x00514A4A,0xFC000000,0x20000402,
	0x00003F40,0x2A080800,0x24000808,0x00122412,0x09090600,0x00000006,0x00001818,0x00080000,0x30000000,0x02023E40,0x010E0100,0x0900000E,0x00000A0D,0x3C3C3C00,0x0000003C,0x00000000,
};
void print6x8 (tiletype *dd, int ox, int y, int fcol, int bcol, const char *fmt, ...)
{
	va_list arglist;
	char st[1024], *c, *v;
	int i, j, ie, x;
	int *lp, *lpx;

	if (!fmt) return;
	va_start(arglist,fmt);
	if (vsnprintf((char *)&st,sizeof(st)-1,fmt,arglist)) st[sizeof(st)-1] = 0;
	va_end(arglist);

	lp = (int *)(y*dd->p+dd->f);
	for(j=1;j<256;y++,lp=(int *)(((INT_PTR)lp)+dd->p),j+=j)
		if ((unsigned)y < (unsigned)dd->y)
			for(c=st,x=ox;*c;c++,x+=6)
			{
				v = ((int)(*c))*6 + ((char *)font6x8); lpx = &lp[x];
				for(i=max(-x,0),ie=min(dd->x-x,6);i<ie;i++) { if (v[i]&j) lpx[i] = fcol; else if (bcol >= 0) lpx[i] = bcol; }
				if ((*c) == 9) { if (bcol >= 0) { for(i=max(-x,6),ie=min(dd->x-x,18);i<ie;i++) lpx[i] = bcol; } x += 2*6; }
			}
}

void drawpix (tiletype *dd, int x, int y, int col)
{
	if (((unsigned)x < (unsigned)dd->x) && ((unsigned)y < (unsigned)dd->y))
		*(int *)(dd->p*y + (x<<2) + dd->f) = col;
}

void drawhlin (tiletype *dd, int x0, int x1, int y, int col)
{
	int x, *iptr;

	if ((unsigned)y >= (unsigned)dd->y) return;
	x0 = max(x0,0); x1 = min(x1,dd->x); if (x0 >= x1) return;
	iptr = (int *)(dd->p*y + dd->f);
	for(x=x0;x<x1;x++) iptr[x] = col;
}

void drawrectfill (tiletype *dd, int x0, int y0, int x1, int y1, int col)
{
	int x, y, *iptr;

	x0 = max(x0,0); x1 = min(x1,dd->x); if (x0 >= x1) return;
	y0 = max(y0,0); y1 = min(y1,dd->y); if (y0 >= y1) return;
	iptr = (int *)(dd->p*y0 + dd->f);
	x1 -= x0; y1 -= y0; iptr += x0;
	for(y=y1;y>0;y--,iptr=(int *)(((INT_PTR)iptr)+dd->p))
	{
		if (col) { for(x=x1-1;x>=0;x--) iptr[x] = col; } else
					{ memset(iptr,0,x1*4); }
	}
}

void drawline (tiletype *dd, float x0, float y0, float x1, float y1, int col)
{
	float f;
	int i, x, y, ipx[2], ipy[2];

	if (x0 <   0.f) { if (x1 <   0.f) return; y0 = (         0.f-x0)*(y1-y0)/(x1-x0)+y0; x0 =          0.f; } else
	if (x0 > dd->x) { if (x1 > dd->x) return; y0 = ((float)dd->x-x0)*(y1-y0)/(x1-x0)+y0; x0 = (float)dd->x; }
	if (y0 <   0.f) { if (y1 <   0.f) return; x0 = (         0.f-y0)*(x1-x0)/(y1-y0)+x0; y0 =          0.f; } else
	if (y0 > dd->y) { if (y1 > dd->y) return; x0 = ((float)dd->y-y0)*(x1-x0)/(y1-y0)+x0; y0 = (float)dd->y; }
	if (x1 <   0.f) {                         y1 = (         0.f-x1)*(y1-y0)/(x1-x0)+y1; x1 =          0.f; } else
	if (x1 > dd->x) {                         y1 = ((float)dd->x-x1)*(y1-y0)/(x1-x0)+y1; x1 = (float)dd->x; }
	if (y1 <   0.f) {                         x1 = (         0.f-y1)*(x1-x0)/(y1-y0)+x1; y1 =          0.f; } else
	if (y1 > dd->y) {                         x1 = ((float)dd->y-y1)*(x1-x0)/(y1-y0)+x1; y1 = (float)dd->y; }

	x1 -= x0; y1 -= y0;
	i = (int)max(fabs(x1),fabs(y1)); if (!(i&0x7fffffff)) return;/*FUK:32-bit!*/
	f = 65536.f/(float)i;
	ipx[0] = (int)(x0*65536.0); ipx[1] = (int)(x1*f);
	ipy[0] = (int)(y0*65536.0); ipy[1] = (int)(y1*f);
	for(;i>0;i--)
	{
		x = (ipx[0]>>16); y = (ipy[0]>>16);
		if (((unsigned)x < (unsigned)dd->x) && ((unsigned)y < (unsigned)dd->y))
			*(int *)(dd->p*y + (x<<2) + dd->f) = col;
		ipx[0] += ipx[1]; ipy[0] += ipy[1];
	}
}

void drawcirc (tiletype *dd, int xc, int yc, int r, int col)
{
	int x, y, d;
	for(x=0,y=r,d=1-r;1;x++)
	{
		drawpix(dd,xc+x,yc+y,col); drawpix(dd,xc+y,yc+x,col);
		drawpix(dd,xc+y,yc-x,col); drawpix(dd,xc+x,yc-y,col);
		drawpix(dd,xc-x,yc+y,col); drawpix(dd,xc-y,yc+x,col);
		drawpix(dd,xc-y,yc-x,col); drawpix(dd,xc-x,yc-y,col);
		if (x >= y) break;
		if (d < 0) { d += ( x   <<1)+3; } else
					  { d += ((x-y)<<1)+5; y--; }
	}
}

void drawcircfill (tiletype *dd, int xc, int yc, int r, int col)
{
	int i, y;
	for(y=yc-r;y<=yc+r;y++)
	{
		i = (int)sqrt((double)(r*r - (yc-y)*(yc-y)));
		drawhlin(dd,xc-i,xc+i,y,col);
	}
}

#if (STANDALONE != 0)

int main (int argc, char **argv)
{
	tiletype dd;
	double tim = 0.0, otim, dtim, avgdtim = 0.0;
	int i, x, y, *iptr, xo = 0, yo = 0, obstatus = 0, numframes = 0;
	char st[64] = {0};

	xres = 1024; yres = 768; prognam = "KMacMain sample app";
	if (!initapp()) return(0);

	while (!breath())
	{
		otim = tim; tim = klock(); dtim = tim-otim;

		if (keystatus[0x1]) { keystatus[0x1] = 0; quitloop(); }

		if (startdirectdraw(&dd.f,&dd.p,&dd.x,&dd.y))
		{
			drawrectfill(&dd,0,0,dd.x,dd.y,0x000000); //Clear screen black

			while (1)
			{
				i = keyread(); if (!i) break; //Check ASCII keys..
				i &= 255; if (!i) continue;
				if (i == 8) { if (st[0]) st[strlen(st)-1] = 0; } //Handle backspace
				else if (strlen(st) < sizeof(st)-1) sprintf(&st[strlen(st)],"%c",i);
			}

				//test clipboard: F1=copy, F2=paste
			if (keystatus[0x3b]) { keystatus[0x3b] = 0; putclip(st,strlen(st)); }
			if (keystatus[0x3c]) { keystatus[0x3c] = 0; char *cptr; if (getclip(&cptr)) { for(i=0;cptr[i] && (i < sizeof(st));i++) { st[i] = cptr[i]; } st[i] = 0; free(cptr); } }
			if (keystatus[0x3d]) { keystatus[0x3d] = 0; msgbox("Title","Hi *","OK"); }
			if (keystatus[0x3e]) { keystatus[0x3e] = 0; const char *cptr = file_select(0,"Pick a .C file","c","c"); if (cptr) msgbox("file_selects returns:",cptr,"OK"); }

			for(x=0,i=0;i<256;i++)
				if (keystatus[i]) { print6x8(&dd,x*32,50,0xffffff,0,"0x%02x",i); x++; }

				//Fill bot-right quadrant of screen w/pattern
			xo += (keystatus[0xcd]-keystatus[0xcb]); //Rig-Lef
			yo += (keystatus[0xd0]-keystatus[0xc8]); //Dow-Up
			for(y=(dd.y>>1);y<dd.y;y++)
			{
				iptr = (int *)(y*dd.p + dd.f);
				for(x=(dd.x>>1);x<dd.x;x++) iptr[x] = ((x+xo)^(y+yo))*0x10101;
			}

			drawcirc(&dd,dd.x>>1,dd.y>>1,dd.y>>1,0xff0000);
			drawline(&dd,mousx,mousy,dd.x>>1,dd.y>>1,0x0000ff);

			print6x8(&dd,0, 0,0xffffff,0,"mousx/y,bstatus: %i,%i,%i",mousx,mousy,bstatus);
			print6x8(&dd,0,10,0xffffff,0,"     dmousx/y/z: %i %i %i",dmousx,dmousy,dmousz);
			print6x8(&dd,0,20,0xffffff,0,"         x/yres: %i %i",xres,yres);
			print6x8(&dd,0,30,0xffffff,0,"        klock(): %f",klock());
			print6x8(&dd,0,40,0xffffff,0,"        keyread: |%s|",st);

			avgdtim += (dtim-avgdtim)*.05; print6x8(&dd,dd.x-64,0,0xffffff,0,"%.2f fps",1.0/avgdtim);
			if (!(numframes&63)) printf("%.2f fps\n",1.0/avgdtim);
			numframes++;

			stopdirectdraw(); nextpage();
		}

		obstatus = bstatus;
	}
	uninitapp();
	return 0;
}

#endif

#if 0
endif
#endif
