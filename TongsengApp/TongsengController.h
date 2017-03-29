#import <Cocoa/Cocoa.h>

@interface TongsengController : NSObject {
    IBOutlet NSTextField *_hostname;
    IBOutlet NSTextField *_port;
	IBOutlet NSTextField *_info;
	IBOutlet NSPopUpButton *_device;
	IBOutlet NSPopUpButton *_protocol;
	IBOutlet NSButton *_button;
	bool _running;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app;
- (void)applicationWillTerminate:(NSApplication *)app;

- (IBAction)select:(id)sender;
- (IBAction)stop:(id)sender;
- (IBAction)start:(id)sender;
- (IBAction)startstop:(id)sender;
@end
