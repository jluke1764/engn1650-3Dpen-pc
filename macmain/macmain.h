#ifndef MACMAIN_H
#define MACMAIN_H

#include <stddef.h>
#define INT_PTR ptrdiff_t

	//Program flow
extern const char *prognam;
extern int xres, yres;
extern void win_settitle (const char *st);
extern int initapp (void);
extern void uninitapp (void);
extern int breath (void);
extern void quitloop (void);
//extern int cmdline2arg (char *cmdline, char **argv); //Helper function
//extern long (CALLBACK *peekwindowproc)(HWND,UINT,WPARAM,LPARAM); //Allow menus&other custom stuff

	//Screen
extern int startdirectdraw (INT_PTR *f, INT_PTR *p, INT_PTR *x, INT_PTR *y);
extern void stopdirectdraw ();
extern void nextpage (void);

	//Timer
extern double klock (void);

	//Keyboard
extern char keystatus[256];
extern int keyread (void);

	//Mouse
extern int mousx, mousy, bstatus, dmousx, dmousy, dmousz, ActiveApp;

	//File dialog
extern int msgbox (const char *tit, const char *txt, const char *butxt);
extern const char *file_select (int flags, const char *txt, const char *def, const char *wildst);
extern int getclip (char **buf);
extern void putclip (char *buf, int leng);

	//Graphics helper
typedef struct { INT_PTR f, p, x, y; } tiletype;
extern void print6x8     (tiletype *dd, int ox, int y, int fcol, int bcol, const char *fmt, ...);
extern void drawpix      (tiletype *dd, int x, int y, int col);
extern void drawhlin     (tiletype *dd, int x0, int x1, int y, int col);
extern void drawline     (tiletype *dd, float x0, float y0, float x1, float y1, int col);
extern void drawcirc     (tiletype *dd, int xc, int yc, int r, int col);
extern void drawrectfill (tiletype *dd, int x0, int y0, int x1, int y1, int col);
extern void drawcircfill (tiletype *dd, int xc, int yc, int r, int col);

#endif
