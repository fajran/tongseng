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
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
	[self stop:app];
	if (_clientRunning) [self clientstop:app];
	return YES;
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
	[_info setStringValue:@"Tongseng is running"];
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
	const int protocol = (int)[_clientProtocol indexOfSelectedItem];
	const int port = atoi([[_clientPort stringValue] UTF8String]);

	if (tongseng_client_start(protocol, port) == 0) {
		[_clientProtocol setEnabled:false];
		[_clientPort setEnabled:false];
		[_clientInfo setStringValue:(tongseng_client_virtual_device() ?
			@"Tongseng is injecting multitouch" :
			(AXIsProcessTrusted() ? @"Tongseng is emulating input" :
				@"Needs Accessibility permission"))];
		[_clientButton setTitle:@"Stop"];
		_clientRunning = true;
	} else {
		[_clientInfo setStringValue:@"Tongseng failed to listen"];
	}
}

- (IBAction)clientstop:(id)sender {
	tongseng_client_stop();
	[_clientProtocol setEnabled:true];
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
