#import "TongsengController.h"
#import "multitouch.h"
#import "tongseng.h"
#import <stdlib.h>
#import <ApplicationServices/ApplicationServices.h>

@implementation TongsengController

- (id) init {
	self = [super init];
	_running = false;
	_clientRunning = false;
	return self;
}

- (void)awakeFromNib
{
	
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NSDisabledDictationMenuItem"];
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NSDisabledCharacterPaletteMenuItem"];
	
	CFArrayRef devList = MTDeviceCreateList();
	CFIndex dev_count = (CFIndex)CFArrayGetCount(devList);
	
	if(dev_count > 1) {
		for (int i=1;i<=dev_count;i++)
		[_device addItemWithTitle:@"External"];
	} else if(dev_count == 0) {
		[_device removeAllItems];
		[_device addItemWithTitle:@"None"];
		[_button setEnabled:false];
		[_hostname setEnabled:false];
		[_port setEnabled:false];
		[_device setEnabled:false];
		[_info setStringValue:@"Tongseng is disabled"];
	}
	
	// native touch injection requires macOS 27 and the hid.virtual.device
	// entitlement; older systems only support CGEvent emulation
	NSOperatingSystemVersion ver = [[NSProcessInfo processInfo] operatingSystemVersion];
	if (ver.majorVersion < 27) {
		[_clientInjection removeAllItems];
		[_clientInjection addItemWithTitle:@"Emulation"];
		[_clientInjection setEnabled:false];
	}
	[self rebuildClientScreens];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	// the tray menu covers all app-level actions; drop the app menubar
	// before creating the status item — replacing it afterwards tears the
	// status item scene down on macOS 27
	NSApp.mainMenu = [[[NSMenu alloc] init] autorelease];
	[self setupStatusItem];
	[self installKeyMonitor];

	// tray app: present the main window on launch
	[self showStatusWindow];
}

// menu bar status item: About, window toggle and quit — the app's only UI
// surface once it runs as a tray app (LSUIElement, no Dock icon)
- (void)setupStatusItem
{
	// MRC target: statusItemWithLength returns an autoreleased object,
	// keep an owning reference or the item is uninstalled on pool drain
	_statusItem = [[[NSStatusBar systemStatusBar]
		statusItemWithLength:NSVariableStatusItemLength] retain];

	NSString *iconPath = [[NSBundle mainBundle]
		pathForResource:@"TUIO" ofType:@"icns"];
	NSImage *icon = [NSImage imageNamed:@"TUIO"];
	if (!icon && iconPath) {
		icon = [[NSImage alloc] initWithContentsOfFile:iconPath];
	}
	NSLog(@"Tongseng status item: iconPath=%@ reps=%lu size=%@",
		iconPath, (unsigned long)(icon ? [icon.representations count] : 0),
		NSStringFromSize(icon ? icon.size : NSZeroSize));
	if (icon && icon.representations.count > 0) {
		[icon setSize:NSMakeSize(18, 18)];
		_statusItem.button.image = icon;
	} else {
		_statusItem.button.title = @"TUIO";
	}
	_statusItem.visible = YES;

	_statusMenu = [[NSMenu alloc] init];
	_statusMenu.delegate = self;
	NSMenuItem *aboutItem = [_statusMenu addItemWithTitle:@"About Tongseng"
		action:@selector(showAbout:) keyEquivalent:@""];
	aboutItem.target = self;
	[_statusMenu addItem:[NSMenuItem separatorItem]];
	NSMenuItem *winItem = [_statusMenu addItemWithTitle:@"Show Tongseng"
		action:@selector(toggleStatusWindow:) keyEquivalent:@"w"];
	winItem.target = self;
	[_statusMenu addItem:[NSMenuItem separatorItem]];
	NSMenuItem *quitItem = [_statusMenu addItemWithTitle:@"Quit Tongseng"
		action:@selector(terminate:) keyEquivalent:@"q"];
	quitItem.target = NSApp;

	_statusItem.menu = _statusMenu;
}

#pragma mark - NSMenuDelegate
- (void)menuNeedsUpdate:(NSMenu *)menu
{
    // Update "Tongseng Window" menu item title
    for (NSMenuItem *item in menu.itemArray) {
        if (item.action == @selector(toggleStatusWindow:)) {
            NSWindow *w = [self statusWindow];
            BOOL visible = w && [w isVisible];
            item.title = visible ? @"Hide Tongseng" : @"Show Tongseng";
        }
    }
}

- (NSWindow *)statusWindow
{
	for (NSWindow *w in [NSApp windows]) {
		if (![w isKindOfClass:[NSPanel class]] && [w canBecomeKeyWindow]) return w;
	}
	return nil;
}

- (void)activateApp
{
	if ([NSApp respondsToSelector:@selector(activate)]) {
		[NSApp activate];
	} else {
		[NSApp activateIgnoringOtherApps:YES];
	}
}

- (void)showStatusWindow
{
	NSWindow *w = [self statusWindow];
	if (!w) return;
	// activate first: ordering the window while the app is inactive
	// leaves it behind other apps' windows
	[self activateApp];
	[w makeKeyAndOrderFront:nil];
}

- (IBAction)toggleStatusWindow:(id)sender
{
	NSWindow *w = [self statusWindow];
	if (!w) return;
	if ([w isVisible]) {
		[w orderOut:nil];
	} else {
		[self showStatusWindow];
	}
}

// orderFrontStandardAboutPanel alone leaves the panel behind for an
// accessory app — order it front explicitly and activate
- (IBAction)showAbout:(id)sender
{
	[NSApp orderFrontStandardAboutPanel:sender];
	// the standard panel can still order behind other apps for an
	// accessory app — pull it to the front explicitly
	for (NSWindow *w in [[NSApp windows] reverseObjectEnumerator]) {
		if ([w isKindOfClass:[NSPanel class]] && [w isVisible]
			&& [[w title] hasPrefix:@"About"]) {
			[w makeKeyAndOrderFront:nil];
			break;
		}
	}
	[self activateApp];
}

// key equivalents on a status-item menu never fire on their own — the
// menu is not the main menu. A local monitor gives ⌘W / ⌘Q real
// function whenever the app is active (its window is key).
- (void)installKeyMonitor
{
	[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
		handler:^NSEvent *(NSEvent *event) {
			if (!(event.modifierFlags & NSEventModifierFlagCommand)) return event;
			switch (event.keyCode) {
				case 13: [self toggleStatusWindow:nil]; break; // W
				case 12: [NSApp terminate:nil]; break;         // Q
				default: return event;
			}
			return nil;
		}];
}

// - (BOOL)validateMenuItem:(NSMenuItem *)item
// {
// 	if (item.action == @selector(toggleStatusWindow:)) {
// 		NSWindow *w = [self statusWindow];
// 		BOOL visible = w && [w isVisible];
// 		[item setTitle:visible ? @"Hide Tongseng Window" : @"Tongseng Window"];
// 	}
// 	return YES;
// }

// refresh the destination screen list; defaults to the internal display and
// keeps a previously chosen display selected while it stays connected
- (void)rebuildClientScreens
{
	NSInteger prev = [_clientScreen selectedItem].tag;
	[_clientScreen removeAllItems];
	CGDirectDisplayID ids[16];
	uint32_t count = 0;
	CGGetActiveDisplayList(16, ids, &count);
	NSInteger sel = 0;
	int ext = 0;
	for (uint32_t i=0;i<count;i++) {
		BOOL builtin = CGDisplayIsBuiltin(ids[i]) != 0;
		[_clientScreen addItemWithTitle:
			(builtin ? @"Internal" : [NSString stringWithFormat:@"External %d", ++ext])];
		_clientScreen.lastItem.tag = (NSInteger)ids[i];
		if (builtin || (CGDirectDisplayID)prev == ids[i]) sel = (NSInteger)i;
	}
	if (count == 0) {
		[_clientScreen addItemWithTitle:@"Internal"];
		_clientScreen.lastItem.tag = 0;
	}
	[_clientScreen selectItemAtIndex:sel];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
	// tray app stays resident; sender/receiver keep running until Quit,
	// which is handled by applicationWillTerminate:
	return NO;
}

- (void)applicationWillTerminate:(NSApplication *)app;
{
	[self stop:app];
	if (_clientRunning) [self clientstop:app];
}


- (IBAction)select:(id)sender {
	const int protocol = (int)[_protocol indexOfSelectedItem];
	
	switch (protocol) {

		case TUIO_TCP:
			[_hostname setStringValue:@"incoming"];
			[_hostname setEnabled:false];
			[_port setStringValue:@"3334"];
			break;
		case TUIO_WEB:
			[_hostname setStringValue:@"incoming"];
			[_hostname setEnabled:false];
			[_port setStringValue:@"8080"];
			break;
		case TUIO_UDP:
		default:
			[_hostname setStringValue:@"localhost"];
			[_port setStringValue:@"3333"];
			[_hostname setEnabled:true];
			break;
	}
}

- (IBAction)start:(id)sender {
	if (_clientRunning) [self clientstop:sender];
	const char *hostname = [[_hostname stringValue] UTF8String];
	const char *strPort = [[_port stringValue] UTF8String];
	const int port = atoi(strPort);
	const int device = (int)[_device indexOfSelectedItem];
	const int protocol = (int)[_protocol indexOfSelectedItem];

	tongseng_set_protocol(protocol);
	tongseng_set_hostname_and_port(hostname, port);
	tongseng_set_device(device);
	tongseng_start();
	
	[_protocol setEnabled:false];
	[_hostname setEnabled:false];
	[_port setEnabled:false];
	[_device setEnabled:false];
	[_info setStringValue:@"Tongseng is sending"];
	[_button setTitle:@"Stop"];
	
	_running = true;
}

- (IBAction)stop:(id)sender {
	tongseng_stop();
	const int protocol = (int)[_protocol indexOfSelectedItem];

	if (protocol==0)[_hostname setEnabled:true];
	[_protocol setEnabled:true];
	[_port setEnabled:true];
	[_device setEnabled:true];
	[_info setStringValue:@"Tongseng is stopped"];
	[_button setTitle:@"Start"];
	
	_running = false;
}
	
- (IBAction)startstop:(id)sender {
	if (!_running) {
		[self start:sender];
	}
	else {
		[self stop:sender];
	}
}

- (IBAction)clientstart:(id)sender {
	if (_running) [self stop:sender];
	const int protocol = (int)[_clientProtocol indexOfSelectedItem];
	const int port = atoi([[_clientPort stringValue] UTF8String]);

	tongseng_client_set_injection(([_clientInjection numberOfItems]>1 &&
		[_clientInjection indexOfSelectedItem]==0) ?
		TUIO_INJECT_TOUCH : TUIO_INJECT_CGEVENT);
	[self rebuildClientScreens];
	tongseng_client_set_display((unsigned int)[[_clientScreen selectedItem] tag]);

	if (tongseng_client_start(protocol, port) == 0) {
		[_clientProtocol setEnabled:false];
		[_clientInjection setEnabled:false];
		[_clientScreen setEnabled:false];
		[_clientPort setEnabled:false];
		[_clientInfo setStringValue:(tongseng_client_virtual_device() ?
			@"Tongseng is injecting" :
			(AXIsProcessTrusted() ? @"Tongseng is emulating" :
				@"Needs Accessibility"))];
		[_clientButton setTitle:@"Stop"];
		_clientRunning = true;
	} else {
		[_clientInfo setStringValue:@"Tongseng failed to listen"];
	}
}

- (IBAction)clientstop:(id)sender {
	tongseng_client_stop();
	[_clientProtocol setEnabled:true];
	if ([_clientInjection numberOfItems] > 1) [_clientInjection setEnabled:true];
	[_clientScreen setEnabled:true];
	[_clientPort setEnabled:true];
	[_clientInfo setStringValue:@"Tongseng is stopped"];
	[_clientButton setTitle:@"Start"];
	_clientRunning = false;
}

- (IBAction)clientstartstop:(id)sender {
	if (!_clientRunning) {
		[self clientstart:sender];
	}
	else {
		[self clientstop:sender];
	}
}

@end
