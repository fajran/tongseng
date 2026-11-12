#ifndef TONGSENG_H_
#define TONGSENG_H_

#ifdef __cplusplus
extern "C" {
#endif
	
#define TUIO_UDP 0
#define TUIO_TCP 1
#define TUIO_WEB 2

void tongseng_set_protocol(int);
void tongseng_set_hostname_and_port(const char*, int);
void tongseng_set_device(int);
void tongseng_set_verbose(int);
void tongseng_start(void);
void tongseng_stop(void);
void tongseng_list_devices(void);

// TUIO client: listen for incoming TUIO messages and inject them into macOS
// returns 0 on success, -1 if the receiver could not be started
int tongseng_client_start(int protocol, int port);
void tongseng_client_stop(void);
int tongseng_client_virtual_device(void);

#ifdef __cplusplus
}
#endif

#endif
