#if 0
OPTS=-pipe -O3
nav: nav.o hidmac.o; gcc nav.o hidmac.o -o nav $(OPTS) -framework Cocoa -framework IOKit
nav.o:    nav.c    ; gcc -c nav.c    -o nav.o    $(OPTS)
hidmac.o: hidmac.c ; gcc -c hidmac.c -o hidmac.o $(OPTS)
ifdef zero
#endif

	//Built to test SpaceNav as generic HID device. NOTE: only new SpaceNav supports toggling LED!

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <IOKit/hid/IOHIDLib.h>

extern int rawhid_open (int vid, int pid, int usage_page, int usage);
extern int rawhid_recv (void *buf, int len);
extern int rawhid_send (void *buf, int len);
extern void rawhid_close (void);

//-------------------------------------------------------
#include <sys/ioctl.h>
#include <termios.h>
static struct termios oterm;
static void close_keyboard () { tcsetattr(0,TCSANOW,&oterm); }
static int kbhit (void) //Linux (POSIX) implementation of _kbhit(). Morgan McGuire, morgan@cs.brown.edu
{
	static int inited = 0;
	if (!inited) //turn off line buffering
	{
		struct termios term; inited = 1;
		tcgetattr(0/*STDIN*/,&term); oterm = term; term.c_lflag &= (~ICANON)&(~ECHO)&(~ISIG);
		tcsetattr(0/*STDIN*/,TCSANOW,&term);
		setbuf(stdin,0); atexit(close_keyboard);
	}
	int bywait; ioctl(0/*STDIN*/,FIONREAD,&bywait); return(bywait);
}
static char getch (void) { char c; while (!kbhit()) usleep(1000); if (fread(&c,1,1,stdin) < 1) return(0); return(c); }
//-------------------------------------------------------
static double klock0 = 0.0;
static double klock (void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC,&t);
	return((double)t.tv_sec + ((double)t.tv_nsec)*1e-9 - klock0);
}
//-------------------------------------------------------

int main (int argc, char **argv)
{
	int i;
	char buf[64], cbuf[64];

	setbuf(stdout,0);

	if (rawhid_open(0x046d,0xc626,-1,-1) <= 0) { printf("rawhid device not found\n"); return -1; } //3Dconnexion SpaceNavigator

	while (1)
	{
		if (rawhid_recv((char *)buf,64) > 0)
		{
				  if (buf[0] == 1) { memcpy(&cbuf[ 0],&buf[1],6); }
			else if (buf[0] == 2) { memcpy(&cbuf[ 6],&buf[1],6); }
			else if (buf[0] == 3) { memcpy(&cbuf[12],&buf[1],2); }
			printf("\rNav says: "); for(i=0;i<13;i++) { printf("%02x ",cbuf[i]&255); }
		}

		if (kbhit())
		{
			i = getch(); if (i == 27) break;
			if ((i == '0') || (i == '1')) { buf[0] = 4; buf[1] = i; rawhid_send(buf,64); printf("nav_led=%c\n",i); }
		}
	}
	rawhid_close();
	printf("ESC\n");

	return 0;
}

#if 0
endif
#endif
