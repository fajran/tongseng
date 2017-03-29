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
void tongseng_start();
void tongseng_stop();
void tongseng_list_devices();

#ifdef __cplusplus
}
#endif

#endif
