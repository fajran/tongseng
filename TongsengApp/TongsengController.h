#import <Cocoa/Cocoa.h>

@interface TongsengController : NSObject {
    IBOutlet NSTextField *_hostname;
    IBOutlet NSTextField *_port;
	IBOutlet NSTextField *_info;
	IBOutlet NSButton *_button;
	bool _running;
}
- (IBAction)startstop:(id)sender;
@end
