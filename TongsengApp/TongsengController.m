#import "TongsengController.h"
#import "tongseng.h"
#import <stdlib.h>

@implementation TongsengController

- (id) init {
	self = [super init];
	_running = false;
	return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
	[self stop:app];
	return YES;
}

- (void)applicationWillTerminate:(NSApplication *)app;
{
	[self stop:app];
}

- (IBAction)start:(id)sender {
	const char *hostname = [[_hostname stringValue] UTF8String];
	const char *strPort = [[_port stringValue] UTF8String];
	const int port = atoi(strPort);
	
	tongseng_set_hostname_and_port(hostname, port);
	tongseng_start();
	
	[_hostname setEnabled:false];
	[_port setEnabled:false];
	[_info setStringValue:@"Tongseng is running"];
	[_button setTitle:@"Stop"];
	
	_running = true;
}

- (IBAction)stop:(id)sender {
	tongseng_stop();
	
	[_hostname setEnabled:true];
	[_port setEnabled:true];
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

@end
