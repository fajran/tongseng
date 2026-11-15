#import <Cocoa/Cocoa.h>

@interface TongsengController : NSObject <NSMenuDelegate> {
    IBOutlet NSTextField *_hostname;
    IBOutlet NSTextField *_port;
	IBOutlet NSTextField *_info;
	IBOutlet NSPopUpButton *_device;
	IBOutlet NSPopUpButton *_protocol;
	IBOutlet NSButton *_button;
	IBOutlet NSPopUpButton *_clientProtocol;
	IBOutlet NSPopUpButton *_clientInjection;
	IBOutlet NSPopUpButton *_clientScreen;
	IBOutlet NSTextField *_clientPort;
	IBOutlet NSTextField *_clientInfo;
	IBOutlet NSButton *_clientButton;
	NSStatusItem *_statusItem;
	NSMenu *_statusMenu;
	bool _running;
	bool _clientRunning;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app;
- (void)applicationWillTerminate:(NSApplication *)app;

- (IBAction)select:(id)sender;
- (IBAction)stop:(id)sender;
- (IBAction)start:(id)sender;
- (IBAction)startstop:(id)sender;
- (IBAction)clientstart:(id)sender;
- (IBAction)clientstop:(id)sender;
- (IBAction)clientstartstop:(id)sender;

- (void)setupStatusItem;
- (IBAction)toggleStatusWindow:(id)sender;
- (IBAction)showAbout:(id)sender;
@end
