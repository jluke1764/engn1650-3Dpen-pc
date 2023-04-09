#if 0
OPTS=-pipe -O3
cam: cam.o macmain.o; gcc cam.o macmain.o -o cam $(OPTS) -framework Cocoa -framework AVFoundation -framework CoreVideo -framework CoreMedia
cam.o:     cam.c;     gcc -x objective-c -c cam.c     -o cam.o     $(OPTS)
macmain.o: macmain.c; gcc -x objective-c -c macmain.c -o macmain.o $(OPTS)
ifdef zero
#endif

#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include "macmain.h"
#define PORT 8080

static unsigned char cambuf[1280*800*4];
static int camfifw = 0, xsiz = 0, ysiz = 0, bpl = 0;

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

struct Point3D {
	float x;
	float y;
	float z;
} typedef Point3D;

struct linefit3d {
    float s;
    float sx;
    float sy;
    float sz;
    float sxx;
    float syy;
    float szz;
    float sxy;
    float sxz;
    float syz;
} typedef linefit3d;

void addPoint(linefit3d* self, float x, float y, float z) {
	self->s += 1;
	self->sx += x;
	self->sy += y;
	self->sz += z;
	self->sxx += x*x;
	self->syy += y*y;
	self->szz += z*z;
	self->sxy += x*y;
	self->sxz += x*z;
	self->syz += y*z;
}
    
void getplane (linefit3d* self, Point3D* norm, Point3D* cent ){
        float r = 1/self->s;
        cent->x = self->sx*r;
		cent->y = self->sy*r;
		cent->z = self->sz*r;
        float sxx = self->sxx - self->sx * cent->x;
        float sxy = self->sxy - self->sx * cent->y;
        float syy = self->syy - self->sy * cent->y;
        float sxz = self->sxz - self->sx * cent->z;
        float szz = self->szz - self->sz * cent->z;
        float syz = self->syz - self->sy * cent->z;
        norm->x = sxy*syz - sxz*syy;
		norm->y = sxy*sxz - sxx*syz;
		norm->z = sxx*syy - sxy*sxy;
}

int main (int argc, char **argv)
{

	int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    //char* hello = "Hello from server";

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
	printf("SOCKET CONNECTED: %d\n", server_fd);
	/*
	if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
	*/
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

	if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

	if ((new_socket
         = accept(server_fd, (struct sockaddr*)&address,
                  (socklen_t*)&addrlen))
        < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
  
	tiletype dd;
	double tim = 0.0, otim, dtim, avgdtim = 0.0;
	static unsigned char pcambuf[1280*800];
	static int qcambuf[1280*800];
	int i, x, y, camuse = 0, isrgba = 0;

	prognam = "Ken's MacOSX webcam viewer";

	if (argc >= 2) { camuse = atoi(argv[1]); }
	if (argc >= 3) { isrgba = atoi(argv[2]); }
	if (webcam_init(camuse,isrgba) < 0) { msgbox(prognam,"Error initing cam","Ok"); return(1); }

	xres = 1280; yres = 720; if (!initapp()) return(0);

	Point3D estpos = {.x = 10, .y = 10, .z = 10};
	//Point3D ihaf = {.x = xsiz/2, .y = ysiz/2, .z = ysiz/2*1.96};
	linefit3d lf = {0};
	Point3D nnorm = {0};
	Point3D cent = {0};
	Point3D norm = {0};

	while (!breath())
	{
		otim = tim; tim = klock(); dtim = tim-otim;
		Point3D ihaf = {.x = xsiz/2, .y = ysiz/2, .z = ysiz/2*1.96};

		if (keystatus[0x1]) { keystatus[0x1] = 0; close(new_socket); quitloop(); }

		if (startdirectdraw(&dd.f,&dd.p,&dd.x,&dd.y))
		{
			drawrectfill(&dd,0,0,dd.x,dd.y,0x000000);
			
			bool wasWhite = 0;
			if (bpl)
			{     //8-bit gray:
				static int fifx[1280*800], fify[1280*800], fifw, fifr;
				static char got[1280*800];

				memcpy(pcambuf,cambuf,bpl*ysiz);
				memset(got,0,xsiz*ysiz);
				memset(&lf,0,sizeof(lf));

				int largest_sumx, largest_sumy, largest_fifw, init_x, init_y = 0;

				for(y=min(dd.y,ysiz)-1;y>=0;y--)
				{
					for(x=min(dd.x,xsiz)-1;x>=0;x--)
					{
						i = pcambuf[y*bpl + x];
						drawpix(&dd,x,y,i*0x10101);
					}
				}
				for(y=min(dd.y,ysiz)-1;y>=0;y--)
				{
					wasWhite = 0;
					for(x=min(dd.x,xsiz)-1;x>=0;x--)
					{
						i = pcambuf[y*bpl + x];
						if (i <= 200) continue;
						if (got[y*xsiz + x]) continue;
					    got[y*xsiz + x] = 1;
						fifx[0] = x; fify[0] = y; fifr = 0; fifw = 1;
						largest_sumx = 0; largest_sumy = 0; largest_fifw = 0; init_x =0; init_y = 0;
						int sumx = 0, sumy = 0;
						while (fifr < fifw)
						{
							int x2 = fifx[fifr]; int y2 = fify[fifr]; fifr++;
							sumx += x2; sumy += y2;
							for(i=0;i<4;i++)
							{
								static const int dirx[4] = {-1,+1,0,0};
								static const int diry[4] = {0,0,-1,+1};
								int nx = x2+dirx[i];
								int ny = y2+diry[i];
								if ((nx < 0) || (nx >= xsiz)) continue;
								if ((ny < 0) || (ny >= ysiz)) continue;
								
								if (pcambuf[ny*bpl + nx] <= 200) continue;
								
								if (got[ny*xsiz + nx]) continue;
								
								fifx[fifw] = nx;
								fify[fifw] = ny;
								fifw++; if (fifw >= 1280*800) goto dammit;
								
								drawpix(&dd,nx,ny,0xaa00aa);
								got[ny*xsiz + nx] = 1;	

								if (fifw > largest_fifw)
								{
									largest_sumx = sumx;
									largest_sumy = sumy;
									largest_fifw = fifw;
									init_x = nx;
									init_y = ny;
								}			
							}
						}
						
						dammit:;
						// find highest fifw
						//find center of it
					}
				}
				
				drawcirc(&dd,(float)largest_sumx/largest_fifw,(float)largest_sumy/largest_fifw,sqrt(largest_fifw),0xffffff);
				static const int dir2x[6] = {-3, -2, -1,  0, +1, +2};
				static const int dir2y[6] = {+1, +1, -1, +1, +1, +1};

				drawcirc(&dd,init_x,init_y,20,0x00ffff);

				
				for(int j = 0; j<6; j++)
				{
					int it_x = init_x;
					int it_y = init_y;
					while (it_x < xsiz && it_y < ysiz)
					{
						it_x = it_x + dir2x[j];
						it_y = it_y + dir2y[j];
						if ((it_x < 0) || (it_x >= xsiz)) break;
						if ((it_y < 0) || (it_y >= ysiz)) break;
						if (pcambuf[it_y*bpl + it_x] <= 200){
							drawcirc(&dd,it_x,it_y,5,0x00ff00);
							float vx = it_x-ihaf.x;
							float vy = it_y-ihaf.y;
							float vz = ihaf.z;
							float t = 1/sqrt(vx * vx + vy * vy + vz * vz);
							addPoint(&lf, vx*t, vy*t, vz*t);
							break;
						}
					}
				}

				getplane(&lf, &norm, &cent);
				
				double t = cent.x*norm.x + cent.y*norm.y + cent.z*norm.z;
				t = 1/sqrt(norm.x*norm.x + norm.y*norm.y + norm.z*norm.z - t*t);
				estpos.x = norm.x*t;
				estpos.y = norm.y*t;
				estpos.z = norm.z*t;
				

				t = ihaf.z/estpos.z;
				float sx = estpos.x*t + ihaf.x;
				float sy = estpos.y*t + ihaf.y;
				bool sign_x = estpos.x > 0;
				bool sign_y = estpos.y > 0;
				bool sign_z = estpos.z > 0;
				estpos.x = fabsf(estpos.x);
				estpos.y = fabsf(estpos.y);
				estpos.z = fabsf(estpos.z);

				//print6x8(&dd,dd.x-1000,8,0xffffff,0,"%3.2f %3.2f %3.2f \n", cent.x, cent.y, cent.z);
				//print6x8(&dd,dd.x-1000,16,0xffffff,0,"%3.2f %3.2f %3.2f \n", norm.x, norm.y, norm.z);
				print6x8(&dd,dd.x-1000,16,0xffffff,0,"%d %d %d", sign_x, sign_y, sign_z);
				print6x8(&dd,dd.x-1000,24,0xffffff,0,"%d%5.2f,%d%5.2f,%d%5.2f", sign_x, estpos.x, sign_y, estpos.y, sign_z, estpos.z);
				char estpos_str[32];
				sprintf(estpos_str, "%d,%5.2f,%d,%5.2f,%d,%5.2f ", sign_x, estpos.x, sign_y, estpos.y, sign_z, estpos.z);

				send(new_socket, estpos_str, strlen(estpos_str), 0);
				
				drawcirc(&dd, (int)sx, (int)sy, 20, 0xff0000);

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
