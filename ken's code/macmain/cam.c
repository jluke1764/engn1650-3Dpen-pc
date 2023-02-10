#if 0
OPTS=-pipe -O3
cam: cam.o macmain.o; gcc cam.o macmain.o -o cam $(OPTS) -framework Cocoa -framework AVFoundation -framework CoreVideo -framework CoreMedia
cam.o:     cam.c;     gcc -x objective-c -c cam.c     -o cam.o     $(OPTS)
macmain.o: macmain.c; gcc -x objective-c -c macmain.c -o macmain.o $(OPTS)
ifdef zero
#endif

#include <stdlib.h>
#include <stdio.h>
#include "macmain.h"

static unsigned char *cambuf = 0;
static int cambufmal = 0, xsiz = 0, ysiz = 0, bpl = 0;

#if !defined(max)
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min)
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

	//https://developer.apple.com/library/archive/documentation/AudioVideo/Conceptual/AVFoundationPG/Articles/04_MediaCapture.html#//apple_ref/doc/uid/TP40010188-CH5-SW2
	//https://webrtc.googlesource.com/src/webrtc/+/f54860e9ef0b68e182a01edc994626d21961bc4b/modules/video_capture/objc/rtc_video_capture_objc.mm
	//https://ffmpeg.org/doxygen/3.1/avfoundation_8m_source.html
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
@interface AVFFrameReceiver : NSObject {}
- (id) initWithContext:(int)hi;
- (void) captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)videoFrame fromConnection:(AVCaptureConnection *)connection;
@end
@implementation AVFFrameReceiver
- (id)initWithContext:(int)hi { return self; }
- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	CVImageBufferRef videoFrame = CMSampleBufferGetImageBuffer(sampleBuffer);
	if (CVPixelBufferLockBaseAddress(videoFrame,0) != kCVReturnSuccess) return;

	unsigned char *fptr = CVPixelBufferGetBaseAddress(videoFrame);
	bpl  = CVPixelBufferGetBytesPerRow(videoFrame);
	xsiz = CVPixelBufferGetWidth(videoFrame);
	ysiz = CVPixelBufferGetHeight(videoFrame);
	//if (CVPixelBufferGetPixelFormatType(videoFrame) == kCVPixelFormatType_32BGRA) printf("32BGRA\n");
	//if (CVPixelBufferGetPixelFormatType(videoFrame) == kCVPixelFormatType_OneComponent8) printf("8Mono\n");

	int siz = bpl*ysiz;
	if (siz > cambufmal)
	{
		if (cambuf) free(cambuf);
		cambuf = (unsigned char *)malloc(siz); cambufmal = siz;
	}

	memcpy(cambuf,fptr,bpl*ysiz); //Slow! Would be much faster to process image here directly - i.e. no memcpy

	CVPixelBufferUnlockBaseAddress(videoFrame,0);
}
@end

static int webcam_init (int camuse, int isbgra)
{
	int cam = 0;

	AVCaptureSession *session = [[AVCaptureSession alloc] init];
	session.sessionPreset = AVCaptureSessionPresetMedium;
	if ([session canSetSessionPreset:AVCaptureSessionPresetHigh]) { session.sessionPreset = AVCaptureSessionPresetHigh; }

	//AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

	NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	for (AVCaptureDevice *device in devices)
	{
		printf("cam %d: %s\n",cam,[[device localizedName] UTF8String]);
		//if ([device position] == AVCaptureDevicePositionBack) { printf("back\n"); } else { printf("front\n"); }
		if (cam == camuse)
		{
			NSError *error;
			AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error]; if (!input) { return(-2); }
			if ([session canAddInput:input]) { [session addInput:input]; } else { return(-3); }
			//[session addInput:input];

			AVCaptureVideoDataOutput *output = [[AVCaptureVideoDataOutput alloc] init];
			[session addOutput:output];
			if (isbgra) output.videoSettings = @{ (NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA) };
					 else output.videoSettings = @{ (NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_OneComponent8) }; //8-bit grayscale

			id avf_delegate = [[AVFFrameReceiver alloc] initWithContext:cam];

			dispatch_queue_t queue = dispatch_queue_create("myqueue",NULL);
			//[output setSampleBufferDelegate:self queue:queue];
			[output setSampleBufferDelegate:avf_delegate queue:queue];
			dispatch_release(queue);

			[output setAlwaysDiscardsLateVideoFrames:YES]; //discard if data output queue blocked (as we process still image)

			if ([session canAddOutput:output]) [session addOutput:output];

			[session startRunning]; //<-WebCam LED turns on here
			return(1);
		}
		cam++;
	}
	return(-1);
}

int main (int argc, char **argv)
{
	tiletype dd;
	double tim = 0.0, otim, dtim, avgdtim = 0.0;
	int i, x, y, camuse = 0, isrgba = 1;

	prognam = "Ken's MacOSX webcam viewer";

	if (argc >= 2) { camuse = atoi(argv[1]); }
	if (argc >= 3) { isrgba = atoi(argv[2]); }
	if (webcam_init(camuse,isrgba) < 0) { msgbox(prognam,"Error initing cam","Ok"); return(1); }

	xres = 1024; yres = 600; if (!initapp()) return(0);

	while (!breath())
	{
		otim = tim; tim = klock(); dtim = tim-otim;

		if (keystatus[0x1]) { keystatus[0x1] = 0; quitloop(); }

		if (startdirectdraw(&dd.f,&dd.p,&dd.x,&dd.y))
		{
			drawrectfill(&dd,0,0,dd.x,dd.y,0x000000);

			if (bpl > xsiz)
			{     //BGRA:
				for(y=min(dd.y,ysiz>>1)-1;y>=0;y--)
				{
					int *iptr = (int *)&cambuf[(y*2)*bpl];
					for(x=min(dd.x,xsiz>>1)-1;x>=0;x--)
					{
						i = iptr[x*2];
						if (bstatus&1) { i = ~i; } //example of insanely stupid image processing
						drawpix(&dd,x,y,i);
					}
				}
			}
			else if (bpl)
			{     //8-bit gray:
				for(y=min(dd.y,ysiz>>1)-1;y>=0;y--)
				{
					unsigned char *ucptr = (unsigned char *)&cambuf[(y*2)*bpl];
					for(x=min(dd.x,xsiz>>1)-1;x>=0;x--)
					{
						i = ((int)ucptr[x*2])*0x10101;
						if (bstatus&1) { i = ~i; } //example of insanely stupid image processing
						drawpix(&dd,x,y,i);
					}
				}
			}

			for(y=540;y<600;y++)
				for(x=960+(y&1);x<1024;x+=2)
				{
					drawpix(&dd,x,y,0x00ff00);
				}

			avgdtim += (dtim-avgdtim)*.05; print6x8(&dd,dd.x-64,0,0xffffff,0,"%.2f fps",1.0/avgdtim);
			print6x8(&dd,dd.x-64,8,0xffffff,0,"cam %d",camuse);
			print6x8(&dd,dd.x-64,16,0xffffff,0,"%dx%d",xsiz,ysiz);

			stopdirectdraw(); nextpage();
		}
	}
	uninitapp();
	return(0);
}

#if 0
endif
#endif
