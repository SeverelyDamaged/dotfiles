diff --git a/config.def.h b/config.def.h
index a2ac963..86fcc84 100644
--- a/config.def.h
+++ b/config.def.h
@@ -3,8 +3,13 @@
 /* appearance */
 static const unsigned int borderpx  = 1;        /* border pixel of windows */
 static const unsigned int snap      = 32;       /* snap pixel */
-static const int showbar            = 1;        /* 0 means no bar */
-static const int topbar             = 1;        /* 0 means bottom bar */
+static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
+static const unsigned int systrayonleft = 0;   	/* 0: systray in the right corner, >0: systray on left of status text */
+static const unsigned int systrayspacing = 2;   /* systray spacing */
+static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
+static const int showsystray        = 1;     /* 0 means no systray */
+static const int showbar            = 1;     /* 0 means no bar */
+static const int topbar             = 1;     /* 0 means bottom bar */
 static const char *fonts[]          = { "monospace:size=10" };
 static const char dmenufont[]       = "monospace:size=10";
 static const char col_gray1[]       = "#222222";
@@ -101,8 +106,8 @@ static Key keys[] = {
 /* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
 static Button buttons[] = {
 	/* click                event mask      button          function        argument */
-	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
-	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
+	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
+	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
 	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
 	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
 	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
diff --git a/dwm.c b/dwm.c
index a96f33c..8153bfe 100644
--- a/dwm.c
+++ b/dwm.c
@@ -57,12 +57,27 @@
 #define TAGMASK                 ((1 << LENGTH(tags)) - 1)
 #define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
 
+#define SYSTEM_TRAY_REQUEST_DOCK    0
+/* XEMBED messages */
+#define XEMBED_EMBEDDED_NOTIFY      0
+#define XEMBED_WINDOW_ACTIVATE      1
+#define XEMBED_FOCUS_IN             4
+#define XEMBED_MODALITY_ON         10
+#define XEMBED_MAPPED              (1 << 0)
+#define XEMBED_WINDOW_ACTIVATE      1
+#define XEMBED_WINDOW_DEACTIVATE    2
+#define VERSION_MAJOR               0
+#define VERSION_MINOR               0
+#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR
+
 /* enums */
 enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
 enum { SchemeNorm, SchemeSel }; /* color schemes */
 enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
+       NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
        NetWMFullscreen, NetActiveWindow, NetWMWindowType,
        NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
+enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
 enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
 enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
        ClkClientWin, ClkRootWin, ClkLast }; /* clicks */
@@ -141,6 +156,12 @@ typedef struct {
 	int monitor;
 } Rule;
 
+typedef struct Systray   Systray;
+struct Systray {
+	Window win;
+	Client *icons;
+};
+
 /* function declarations */
 static void applyrules(Client *c);
 static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
@@ -163,6 +184,7 @@ static void detachstack(Client *c);
 static Monitor *dirtomon(int dir);
 static void drawbar(Monitor *m);
 static void drawbars(void);
+static int drawstatusbar(Monitor *m, int bh, char* text);
 static void enternotify(XEvent *e);
 static void expose(XEvent *e);
 static void focus(Client *c);
@@ -172,6 +194,7 @@ static void focusstack(const Arg *arg);
 static Atom getatomprop(Client *c, Atom prop);
 static int getrootptr(int *x, int *y);
 static long getstate(Window w);
+static unsigned int getsystraywidth();
 static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
 static void grabbuttons(Client *c, int focused);
 static void grabkeys(void);
@@ -189,13 +212,16 @@ static void pop(Client *);
 static void propertynotify(XEvent *e);
 static void quit(const Arg *arg);
 static Monitor *recttomon(int x, int y, int w, int h);
+static void removesystrayicon(Client *i);
 static void resize(Client *c, int x, int y, int w, int h, int interact);
+static void resizebarwin(Monitor *m);
 static void resizeclient(Client *c, int x, int y, int w, int h);
 static void resizemouse(const Arg *arg);
+static void resizerequest(XEvent *e);
 static void restack(Monitor *m);
 static void run(void);
 static void scan(void);
-static int sendevent(Client *c, Atom proto);
+static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
 static void sendmon(Client *c, Monitor *m);
 static void setclientstate(Client *c, long state);
 static void setfocus(Client *c);
@@ -207,6 +233,7 @@ static void seturgent(Client *c, int urg);
 static void showhide(Client *c);
 static void sigchld(int unused);
 static void spawn(const Arg *arg);
+static Monitor *systraytomon(Monitor *m);
 static void tag(const Arg *arg);
 static void tagmon(const Arg *arg);
 static void tile(Monitor *);
@@ -224,20 +251,25 @@ static int updategeom(void);
 static void updatenumlockmask(void);
 static void updatesizehints(Client *c);
 static void updatestatus(void);
+static void updatesystray(void);
+static void updatesystrayicongeom(Client *i, int w, int h);
+static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
 static void updatetitle(Client *c);
 static void updatewindowtype(Client *c);
 static void updatewmhints(Client *c);
 static void view(const Arg *arg);
 static Client *wintoclient(Window w);
 static Monitor *wintomon(Window w);
+static Client *wintosystrayicon(Window w);
 static int xerror(Display *dpy, XErrorEvent *ee);
 static int xerrordummy(Display *dpy, XErrorEvent *ee);
 static int xerrorstart(Display *dpy, XErrorEvent *ee);
 static void zoom(const Arg *arg);
 
 /* variables */
+static Systray *systray = NULL;
 static const char broken[] = "broken";
-static char stext[256];
+static char stext[1024];
 static int screen;
 static int sw, sh;           /* X display screen geometry width, height */
 static int bh, blw = 0;      /* bar geometry */
@@ -258,9 +290,10 @@ static void (*handler[LASTEvent]) (XEvent *) = {
 	[MapRequest] = maprequest,
 	[MotionNotify] = motionnotify,
 	[PropertyNotify] = propertynotify,
+    [ResizeRequest] = resizerequest,
 	[UnmapNotify] = unmapnotify
 };
-static Atom wmatom[WMLast], netatom[NetLast];
+static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
 static int running = 1;
 static Cur *cursor[CurLast];
 static Clr **scheme;
@@ -440,7 +473,7 @@ buttonpress(XEvent *e)
 			arg.ui = 1 << i;
 		} else if (ev->x < x + blw)
 			click = ClkLtSymbol;
-		else if (ev->x > selmon->ww - (int)TEXTW(stext))
+		else if (ev->x > selmon->ww - (int)TEXTW(stext) - getsystraywidth())
 			click = ClkStatusText;
 		else
 			click = ClkWinTitle;
@@ -483,9 +516,16 @@ cleanup(void)
 	XUngrabKey(dpy, AnyKey, AnyModifier, root);
 	while (mons)
 		cleanupmon(mons);
-	for (i = 0; i < CurLast; i++)
+
+	if (showsystray) {
+		XUnmapWindow(dpy, systray->win);
+		XDestroyWindow(dpy, systray->win);
+		free(systray);
+	}
+
+    for (i = 0; i < CurLast; i++)
 		drw_cur_free(drw, cursor[i]);
-	for (i = 0; i < LENGTH(colors); i++)
+	for (i = 0; i < LENGTH(colors) + 1; i++)
 		free(scheme[i]);
 	XDestroyWindow(dpy, wmcheckwin);
 	drw_free(drw);
@@ -513,9 +553,58 @@ cleanupmon(Monitor *mon)
 void
 clientmessage(XEvent *e)
 {
+	XWindowAttributes wa;
+	XSetWindowAttributes swa;
 	XClientMessageEvent *cme = &e->xclient;
 	Client *c = wintoclient(cme->window);
 
+	if (showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
+		/* add systray icons */
+		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
+			if (!(c = (Client *)calloc(1, sizeof(Client))))
+				die("fatal: could not malloc() %u bytes\n", sizeof(Client));
+			if (!(c->win = cme->data.l[2])) {
+				free(c);
+				return;
+			}
+			c->mon = selmon;
+			c->next = systray->icons;
+			systray->icons = c;
+			if (!XGetWindowAttributes(dpy, c->win, &wa)) {
+				/* use sane defaults */
+				wa.width = bh;
+				wa.height = bh;
+				wa.border_width = 0;
+			}
+			c->x = c->oldx = c->y = c->oldy = 0;
+			c->w = c->oldw = wa.width;
+			c->h = c->oldh = wa.height;
+			c->oldbw = wa.border_width;
+			c->bw = 0;
+			c->isfloating = True;
+			/* reuse tags field as mapped status */
+			c->tags = 1;
+			updatesizehints(c);
+			updatesystrayicongeom(c, wa.width, wa.height);
+			XAddToSaveSet(dpy, c->win);
+			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
+			XReparentWindow(dpy, c->win, systray->win, 0, 0);
+			/* use parents background color */
+			swa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
+			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			/* FIXME not sure if I have to send these events, too */
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			XSync(dpy, False);
+			resizebarwin(selmon);
+			updatesystray();
+			setclientstate(c, NormalState);
+		}
+		return;
+	}
+
 	if (!c)
 		return;
 	if (cme->message_type == netatom[NetWMState]) {
@@ -568,7 +657,7 @@ configurenotify(XEvent *e)
 				for (c = m->clients; c; c = c->next)
 					if (c->isfullscreen)
 						resizeclient(c, m->mx, m->my, m->mw, m->mh);
-				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
+				resizebarwin(m);
 			}
 			focus(NULL);
 			arrange(NULL);
@@ -653,6 +742,11 @@ destroynotify(XEvent *e)
 
 	if ((c = wintoclient(ev->window)))
 		unmanage(c, 1);
+	else if ((c = wintosystrayicon(ev->window))) {
+		removesystrayicon(c);
+		resizebarwin(selmon);
+		updatesystray();
+	}
 }
 
 void
@@ -693,10 +787,119 @@ dirtomon(int dir)
 	return m;
 }
 
+int
+drawstatusbar(Monitor *m, int bh, char* stext) {
+	int ret, i, w, x, len;
+	short isCode = 0;
+	char *text;
+	char *p;
+
+	len = strlen(stext) + 1 ;
+	if (!(text = (char*) malloc(sizeof(char)*len)))
+		die("malloc");
+	p = text;
+	memcpy(text, stext, len);
+
+	/* compute width of the status text */
+	w = 0;
+	i = -1;
+	while (text[++i]) {
+		if (text[i] == '^') {
+			if (!isCode) {
+				isCode = 1;
+				text[i] = '\0';
+				w += TEXTW(text) - lrpad;
+				text[i] = '^';
+				if (text[++i] == 'f')
+					w += atoi(text + ++i);
+			} else {
+				isCode = 0;
+				text = text + i + 1;
+				i = -1;
+			}
+		}
+	}
+	if (!isCode)
+		w += TEXTW(text) - lrpad;
+	else
+		isCode = 0;
+	text = p;
+
+	w += 2; /* 1px padding on both sides */
+	ret = m->ww - w;
+	x = m->ww - w - getsystraywidth();
+
+	drw_setscheme(drw, scheme[LENGTH(colors)]);
+	drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
+	drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
+	drw_rect(drw, x, 0, w, bh, 1, 1);
+	x++;
+
+	/* process status text */
+	i = -1;
+	while (text[++i]) {
+		if (text[i] == '^' && !isCode) {
+			isCode = 1;
+
+			text[i] = '\0';
+			w = TEXTW(text) - lrpad;
+			drw_text(drw, x, 0, w, bh, 0, text, 0);
+
+			x += w;
+
+			/* process code */
+			while (text[++i] != '^') {
+				if (text[i] == 'c') {
+					char buf[8];
+					memcpy(buf, (char*)text+i+1, 7);
+					buf[7] = '\0';
+					drw_clr_create(drw, &drw->scheme[ColFg], buf);
+					i += 7;
+				} else if (text[i] == 'b') {
+					char buf[8];
+					memcpy(buf, (char*)text+i+1, 7);
+					buf[7] = '\0';
+					drw_clr_create(drw, &drw->scheme[ColBg], buf);
+					i += 7;
+				} else if (text[i] == 'd') {
+					drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
+					drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
+				} else if (text[i] == 'r') {
+					int rx = atoi(text + ++i);
+					while (text[++i] != ',');
+					int ry = atoi(text + ++i);
+					while (text[++i] != ',');
+					int rw = atoi(text + ++i);
+					while (text[++i] != ',');
+					int rh = atoi(text + ++i);
+
+					drw_rect(drw, rx + x, ry, rw, rh, 1, 0);
+				} else if (text[i] == 'f') {
+					x += atoi(text + ++i);
+				}
+			}
+
+			text = text + i + 1;
+			i=-1;
+			isCode = 0;
+		}
+	}
+
+	if (!isCode) {
+		w = TEXTW(text) - lrpad;
+		drw_text(drw, x, 0, w, bh, 0, text, 0);
+	}
+
+	drw_setscheme(drw, scheme[SchemeNorm]);
+	free(p);
+
+	return ret;
+}
+
 void
 drawbar(Monitor *m)
 {
-	int x, w, tw = 0;
+	int x, w, tw = 0, stw = 0;
 	int boxs = drw->fonts->h / 9;
 	int boxw = drw->fonts->h / 6 + 2;
 	unsigned int i, occ = 0, urg = 0;
@@ -705,13 +908,15 @@ drawbar(Monitor *m)
 	if (!m->showbar)
 		return;
 
+	if(showsystray && m == systraytomon(m) && !systrayonleft)
+		stw = getsystraywidth();
+
 	/* draw status first so it can be overdrawn by tags later */
 	if (m == selmon) { /* status is only drawn on selected monitor */
-		drw_setscheme(drw, scheme[SchemeNorm]);
-		tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
-		drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
+		tw = m->ww - drawstatusbar(m, bh, stext);
 	}
 
+	resizebarwin(m);
 	for (c = m->clients; c; c = c->next) {
 		occ |= c->tags;
 		if (c->isurgent)
@@ -732,7 +937,7 @@ drawbar(Monitor *m)
 	drw_setscheme(drw, scheme[SchemeNorm]);
 	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);
 
-	if ((w = m->ww - tw - x) > bh) {
+	if ((w = m->ww - tw - stw - x) > bh) {
 		if (m->sel) {
 			drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);
 			drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
@@ -743,7 +948,7 @@ drawbar(Monitor *m)
 			drw_rect(drw, x, 0, w, bh, 1, 1);
 		}
 	}
-	drw_map(drw, m->barwin, 0, 0, m->ww, bh);
+	drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
 }
 
 void
@@ -780,8 +985,11 @@ expose(XEvent *e)
 	Monitor *m;
 	XExposeEvent *ev = &e->xexpose;
 
-	if (ev->count == 0 && (m = wintomon(ev->window)))
+	if (ev->count == 0 && (m = wintomon(ev->window))) {
 		drawbar(m);
+		if (m == selmon)
+			updatesystray();
+	}
 }
 
 void
@@ -867,9 +1075,17 @@ getatomprop(Client *c, Atom prop)
 	unsigned char *p = NULL;
 	Atom da, atom = None;
 
-	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
+	/* FIXME getatomprop should return the number of items and a pointer to
+	 * the stored data instead of this workaround */
+	Atom req = XA_ATOM;
+	if (prop == xatom[XembedInfo])
+		req = xatom[XembedInfo];
+
+	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
 		&da, &di, &dl, &dl, &p) == Success && p) {
 		atom = *(Atom *)p;
+		if (da == xatom[XembedInfo] && dl == 2)
+			atom = ((Atom *)p)[1];
 		XFree(p);
 	}
 	return atom;
@@ -903,6 +1119,16 @@ getstate(Window w)
 	return result;
 }
 
+unsigned int
+getsystraywidth()
+{
+	unsigned int w = 0;
+	Client *i;
+	if(showsystray)
+		for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
+	return w ? w + systrayspacing : 1;
+}
+
 int
 gettextprop(Window w, Atom atom, char *text, unsigned int size)
 {
@@ -1007,7 +1233,8 @@ killclient(const Arg *arg)
 {
 	if (!selmon->sel)
 		return;
-	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
+
+	if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
 		XGrabServer(dpy);
 		XSetErrorHandler(xerrordummy);
 		XSetCloseDownMode(dpy, DestroyAll);
@@ -1096,6 +1323,13 @@ maprequest(XEvent *e)
 	static XWindowAttributes wa;
 	XMapRequestEvent *ev = &e->xmaprequest;
 
+	Client *i;
+	if ((i = wintosystrayicon(ev->window))) {
+		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
+		resizebarwin(selmon);
+		updatesystray();
+	}
+
 	if (!XGetWindowAttributes(dpy, ev->window, &wa))
 		return;
 	if (wa.override_redirect)
@@ -1219,7 +1453,18 @@ propertynotify(XEvent *e)
 	Window trans;
 	XPropertyEvent *ev = &e->xproperty;
 
-	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
+	if ((c = wintosystrayicon(ev->window))) {
+		if (ev->atom == XA_WM_NORMAL_HINTS) {
+			updatesizehints(c);
+			updatesystrayicongeom(c, c->w, c->h);
+		}
+		else
+			updatesystrayiconstate(c, ev);
+		resizebarwin(selmon);
+		updatesystray();
+	}
+
+    if ((ev->window == root) && (ev->atom == XA_WM_NAME))
 		updatestatus();
 	else if (ev->state == PropertyDelete)
 		return; /* ignore */
@@ -1269,6 +1514,19 @@ recttomon(int x, int y, int w, int h)
 	return r;
 }
 
+void
+removesystrayicon(Client *i)
+{
+	Client **ii;
+
+	if (!showsystray || !i)
+		return;
+	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
+	if (ii)
+		*ii = i->next;
+	free(i);
+}
+
 void
 resize(Client *c, int x, int y, int w, int h, int interact)
 {
@@ -1276,6 +1534,14 @@ resize(Client *c, int x, int y, int w, int h, int interact)
 		resizeclient(c, x, y, w, h);
 }
 
+void
+resizebarwin(Monitor *m) {
+	unsigned int w = m->ww;
+	if (showsystray && m == systraytomon(m) && !systrayonleft)
+		w -= getsystraywidth();
+	XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
+}
+
 void
 resizeclient(Client *c, int x, int y, int w, int h)
 {
@@ -1348,6 +1614,19 @@ resizemouse(const Arg *arg)
 	}
 }
 
+void
+resizerequest(XEvent *e)
+{
+	XResizeRequestEvent *ev = &e->xresizerequest;
+	Client *i;
+
+	if ((i = wintosystrayicon(ev->window))) {
+		updatesystrayicongeom(i, ev->width, ev->height);
+		resizebarwin(selmon);
+		updatesystray();
+	}
+}
+
 void
 restack(Monitor *m)
 {
@@ -1437,26 +1716,37 @@ setclientstate(Client *c, long state)
 }
 
 int
-sendevent(Client *c, Atom proto)
+sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
 {
 	int n;
-	Atom *protocols;
+	Atom *protocols, mt;
 	int exists = 0;
 	XEvent ev;
 
-	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
-		while (!exists && n--)
-			exists = protocols[n] == proto;
-		XFree(protocols);
+	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
+		mt = wmatom[WMProtocols];
+		if (XGetWMProtocols(dpy, w, &protocols, &n)) {
+			while (!exists && n--)
+				exists = protocols[n] == proto;
+			XFree(protocols);
+		}
 	}
+	else {
+		exists = True;
+		mt = proto;
+    }
+
 	if (exists) {
 		ev.type = ClientMessage;
-		ev.xclient.window = c->win;
-		ev.xclient.message_type = wmatom[WMProtocols];
+		ev.xclient.window = w;
+		ev.xclient.message_type = mt;
 		ev.xclient.format = 32;
-		ev.xclient.data.l[0] = proto;
-		ev.xclient.data.l[1] = CurrentTime;
-		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
+		ev.xclient.data.l[0] = d0;
+		ev.xclient.data.l[1] = d1;
+		ev.xclient.data.l[2] = d2;
+		ev.xclient.data.l[3] = d3;
+		ev.xclient.data.l[4] = d4;
+		XSendEvent(dpy, w, False, mask, &ev);
 	}
 	return exists;
 }
@@ -1470,7 +1760,7 @@ setfocus(Client *c)
 			XA_WINDOW, 32, PropModeReplace,
 			(unsigned char *) &(c->win), 1);
 	}
-	sendevent(c, wmatom[WMTakeFocus]);
+	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
 }
 
 void
@@ -1558,22 +1848,32 @@ setup(void)
 	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
 	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
 	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
-	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
-	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
+   netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
+	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
+	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
+	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
+	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
+    netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
 	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
 	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
 	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
 	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
 	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
 	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
-	/* init cursors */
+	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
+	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
+	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
+    /* init cursors */
 	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
 	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
 	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
 	/* init appearance */
-	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
+	scheme = ecalloc(LENGTH(colors) + 1, sizeof(Clr *));
+	scheme[LENGTH(colors)] = drw_scm_create(drw, colors[0], 3);
 	for (i = 0; i < LENGTH(colors); i++)
 		scheme[i] = drw_scm_create(drw, colors[i], 3);
+	/* init system tray */
+	updatesystray();
 	/* init bars */
 	updatebars();
 	updatestatus();
@@ -1707,7 +2007,18 @@ togglebar(const Arg *arg)
 {
 	selmon->showbar = !selmon->showbar;
 	updatebarpos(selmon);
-	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
+	resizebarwin(selmon);
+	if (showsystray) {
+		XWindowChanges wc;
+		if (!selmon->showbar)
+			wc.y = -bh;
+		else if (selmon->showbar) {
+			wc.y = 0;
+			if (!selmon->topbar)
+				wc.y = selmon->mh - bh;
+		}
+		XConfigureWindow(dpy, systray->win, CWY, &wc);
+	}
 	arrange(selmon);
 }
 
@@ -1802,11 +2113,18 @@ unmapnotify(XEvent *e)
 		else
 			unmanage(c, 0);
 	}
+	else if ((c = wintosystrayicon(ev->window))) {
+		/* KLUDGE! sometimes icons occasionally unmap their windows, but do
+		 * _not_ destroy them. We map those windows back */
+		XMapRaised(dpy, c->win);
+		updatesystray();
+	}
 }
 
 void
 updatebars(void)
 {
+	unsigned int w;
 	Monitor *m;
 	XSetWindowAttributes wa = {
 		.override_redirect = True,
@@ -1817,10 +2135,15 @@ updatebars(void)
 	for (m = mons; m; m = m->next) {
 		if (m->barwin)
 			continue;
-		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
+		w = m->ww;
+		if (showsystray && m == systraytomon(m))
+			w -= getsystraywidth();
+		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, w, bh, 0, DefaultDepth(dpy, screen),
 				CopyFromParent, DefaultVisual(dpy, screen),
 				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
 		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
+		if (showsystray && m == systraytomon(m))
+			XMapRaised(dpy, systray->win);
 		XMapRaised(dpy, m->barwin);
 		XSetClassHint(dpy, m->barwin, &ch);
 	}
@@ -1996,6 +2319,125 @@ updatestatus(void)
 	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
 		strcpy(stext, "dwm-"VERSION);
 	drawbar(selmon);
+	updatesystray();
+}
+
+
+void
+updatesystrayicongeom(Client *i, int w, int h)
+{
+	if (i) {
+		i->h = bh;
+		if (w == h)
+			i->w = bh;
+		else if (h == bh)
+			i->w = w;
+		else
+			i->w = (int) ((float)bh * ((float)w / (float)h));
+		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
+		/* force icons into the systray dimensions if they don't want to */
+		if (i->h > bh) {
+			if (i->w == i->h)
+				i->w = bh;
+			else
+				i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
+			i->h = bh;
+		}
+	}
+}
+
+void
+updatesystrayiconstate(Client *i, XPropertyEvent *ev)
+{
+	long flags;
+	int code = 0;
+
+	if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
+			!(flags = getatomprop(i, xatom[XembedInfo])))
+		return;
+
+	if (flags & XEMBED_MAPPED && !i->tags) {
+		i->tags = 1;
+		code = XEMBED_WINDOW_ACTIVATE;
+		XMapRaised(dpy, i->win);
+		setclientstate(i, NormalState);
+	}
+	else if (!(flags & XEMBED_MAPPED) && i->tags) {
+		i->tags = 0;
+		code = XEMBED_WINDOW_DEACTIVATE;
+		XUnmapWindow(dpy, i->win);
+		setclientstate(i, WithdrawnState);
+	}
+	else
+		return;
+	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
+			systray->win, XEMBED_EMBEDDED_VERSION);
+}
+
+void
+updatesystray(void)
+{
+	XSetWindowAttributes wa;
+	XWindowChanges wc;
+	Client *i;
+	Monitor *m = systraytomon(NULL);
+	unsigned int x = m->mx + m->mw;
+	unsigned int sw = TEXTW(stext) - lrpad + systrayspacing;
+	unsigned int w = 1;
+
+	if (!showsystray)
+		return;
+	if (systrayonleft)
+		x -= sw + lrpad / 2;
+	if (!systray) {
+		/* init systray */
+		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
+			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
+		systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeSel][ColBg].pixel);
+		wa.event_mask        = ButtonPressMask | ExposureMask;
+		wa.override_redirect = True;
+		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
+		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
+		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
+				PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
+		XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
+		XMapRaised(dpy, systray->win);
+		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
+		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
+			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
+			XSync(dpy, False);
+		}
+		else {
+			fprintf(stderr, "dwm: unable to obtain system tray.\n");
+			free(systray);
+			systray = NULL;
+			return;
+		}
+	}
+	for (w = 0, i = systray->icons; i; i = i->next) {
+		/* make sure the background color stays the same */
+		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
+		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
+		XMapRaised(dpy, i->win);
+		w += systrayspacing;
+		i->x = w;
+		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
+		w += i->w;
+		if (i->mon != m)
+			i->mon = m;
+	}
+	w = w ? w + systrayspacing : 1;
+	x -= w;
+	XMoveResizeWindow(dpy, systray->win, x, m->by, w, bh);
+	wc.x = x; wc.y = m->by; wc.width = w; wc.height = bh;
+	wc.stack_mode = Above; wc.sibling = m->barwin;
+	XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
+	XMapWindow(dpy, systray->win);
+	XMapSubwindows(dpy, systray->win);
+	/* redraw background */
+	XSetForeground(dpy, drw->gc, scheme[SchemeNorm][ColBg].pixel);
+	XFillRectangle(dpy, systray->win, drw->gc, 0, 0, w, bh);
+	XSync(dpy, False);
 }
 
 void
@@ -2063,6 +2505,16 @@ wintoclient(Window w)
 	return NULL;
 }
 
+Client *
+wintosystrayicon(Window w) {
+	Client *i = NULL;
+
+	if (!showsystray || !w)
+		return i;
+	for (i = systray->icons; i && i->win != w; i = i->next) ;
+	return i;
+}
+
 Monitor *
 wintomon(Window w)
 {
@@ -2116,6 +2568,22 @@ xerrorstart(Display *dpy, XErrorEvent *ee)
 	return -1;
 }
 
+Monitor *
+systraytomon(Monitor *m) {
+	Monitor *t;
+	int i, n;
+	if(!systraypinning) {
+		if(!m)
+			return selmon;
+		return m == selmon ? m : NULL;
+	}
+	for(n = 1, t = mons; t && t->next; n++, t = t->next) ;
+	for(i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next) ;
+	if(systraypinningfailfirst && n < systraypinning)
+		return mons;
+	return t;
+}
+
 void
 zoom(const Arg *arg)
 {
