// Copyright (C) 2009  Fajran Iman Rusadi <fajran@gmail.com>

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// Based on multitouch code from http://www.steike.com/code/multitouch/

#include "tongseng.h"
#include "mt.h"
#include <iostream>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <TuioServer.h>
#include <TuioCursor.h>
#include <TuioTime.h>

#include <set>
#include <map>

#define D(s) /* std::cout << s << std::endl; */

#define EXISTS(c, v) ((c).find(v) != (c).end())
#define FOREACH(c, i) for (i=(c).begin(); i!=(c).end(); i++)

static MTDeviceRef dev;
static TUIO::TuioServer* server;
static bool running = false;
static bool verbose = false;
static std::string host("localhost");
static int port = 3333;

// Sensitivity
#define MIN_DISTANCE 0.00001f

// Events dispatch interval
#define SAMPLING_INTERVAL 20000
static long lastFrameTimeSec = -1;
static long lastFrameTimeUsec = -1;

// Active fingers
static std::set<int> currentFingers;

// Position of active fingers
static std::map<int, float> lastX;
static std::map<int, float> lastY;

// Finger to TUIO cursors mapping
static std::map<int, TUIO::TuioCursor*> cursors;

// Check if a position changes is considered as a movement
static inline bool isMoving(int id, float x, float y)
{
	float dx = x - lastX[id];
	float dy = y - lastY[id];
	float distance2 = dx*dx + dy*dy;
	D("distance: id=" << id << ", distance=" << distance2);
	return distance2 > MIN_DISTANCE;
}

// New touch
static void touch_add(int id, float x, float y)
{
	D("touch_add: id=" << id << ", x=" << x << ", y=" << y);
	lastX[id] = x;
	lastY[id] = y;

	TUIO::TuioCursor* cursor = server->addTuioCursor(x, y);
	cursors[id] = cursor;
}

// Update touch
static void touch_update(int id, float x, float y)
{
	if (isMoving(id, x, y)) {
		D("touch_update: id=" << id << ", x=" << x << ", y=" << y);
		lastX[id] = x;
		lastY[id] = y;

		TUIO::TuioCursor* cursor = cursors[id];
		server->updateTuioCursor(cursor, x, y);
	}
}

// Remove touch
static void touch_remove(int id)
{
	lastX.erase(id);
	lastY.erase(id);

	D("touch_remove: id=" << id);

	TUIO::TuioCursor* cursor = cursors[id];
	server->removeTuioCursor(cursor);
	cursors.erase(id);
}

// Event dispatch guard
static bool sampling_interval_passed()
{
	TUIO::TuioTime currentTime = TUIO::TuioTime::getSessionTime();
	long dsec = currentTime.getSeconds() - lastFrameTimeSec;
	long dusec = currentTime.getMicroseconds() - lastFrameTimeUsec;

	bool res = false;

	if (lastFrameTimeSec == -1) {
		lastFrameTimeSec = currentTime.getSeconds();
		lastFrameTimeUsec = currentTime.getMicroseconds();
		res = true;
	}
	else if (dsec > 1) {
		res = true;
	}
	else {
		long delta =  dsec * 1000000 + dusec;
		if (delta > SAMPLING_INTERVAL) {
			res = true;
		}
	}

	return res;
}

void tuio_frame_begin()
{
	TUIO::TuioTime currentTime = TUIO::TuioTime::getSessionTime();
	server->initFrame(currentTime);
}

// Flush events
static void tuio_frame_end()
{
	server->stopUntouchedMovingCursors();
	server->commitFrame();
}

// Process incoming events
static int callback(int device, Finger *data, int nFingers, double timestamp, int frame)
{
	if (!running || !sampling_interval_passed()) {
		return 0;
	}

	tuio_frame_begin();

	std::set<int> fingers;

	// Process incoming events
	int i;
	for (i=0; i<nFingers; i++) {
		Finger *f = &data[i];
		int id = f->identifier;

		float x = f->normalized.pos.x;
		float y = 1.0f - f->normalized.pos.y; // reverse y axis
		
		if (EXISTS(currentFingers, id)) {
			// update
			touch_update(id, x, y);
		}
		else {
			// add
			touch_add(id, x, y);
		}
		fingers.insert(id);
	}

	// Remove old events
	std::set<int>::iterator iter;
	FOREACH(currentFingers, iter) {
		int id = *iter;
		if (!EXISTS(fingers, id)) {
			// remove
			touch_remove(id);
		}
	}

	// Update active fingers list
	currentFingers.clear();
	currentFingers.insert(fingers.begin(), fingers.end());

	tuio_frame_end();

	return 0;
}

static void tuio_start()
{
	server = new TUIO::TuioServer((char*)host.c_str(), port);
	server->setVerbose(verbose);
}

static void release_all_fingers()
{
	tuio_frame_begin();

	std::set<int>::iterator iter;
	FOREACH(currentFingers, iter) {
		touch_remove(*iter);
	}

	tuio_frame_end();
}

static void tuio_stop()
{
	release_all_fingers();
	delete server;
}

static void show_help()
{
	std::cout << "Usage: tongseng [options] [host] [port]" << std::endl;
	std::cout << "        -v verbose" << std::endl;
	std::cout << "        -h show help" << std::endl;
}

static void init(int argc, char** argv)
{
	int aflag = 0;
	int bflag = 0;
	char *cvalue = NULL;
	int index;
	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "v")) != -1) {
		switch (c) {
			case 'v':
				verbose = true;
				break;
			case 'h':
				show_help();
				exit(0);
			default:
				show_help();
				exit(1);
		}
	}

	for (index=optind, c=0; index < argc; index++, c++) {
		switch (c) {
			case 0:
				host = argv[index];
				break;
			case 1:
				port = atoi(argv[index]);
				break;
			default:
				break;
		}
	}
}

static void stop(int param)
{
	running = false;
}

void mt_start()
{
	dev = MTDeviceCreateDefault();
	MTRegisterContactFrameCallback(dev, callback);
	MTDeviceStart(dev);
}

// Stop
void mt_stop()
{
	MTUnregisterContactFrameCallback(dev, callback);
	MTDeviceStop(dev);
	MTDeviceRelease(dev);
}

int main(int argc, char** argv)
{
	init(argc, argv);
	std::cout << "Host: " << host << std::endl;
	std::cout << "Port: " << port << std::endl;
	std::cout << "Verbose: " << verbose << std::endl;
	std::cout << "Press Ctrl+C to end this program." << std::endl;

	signal(SIGINT, stop);
	signal(SIGHUP, stop);
	signal(SIGQUIT, stop);
	signal(SIGTERM, stop);

	mt_start();
	tuio_start();

	// Loop until the program is stopped.
	running = true;
	while (running) { 
		usleep(1000);
	};

	std::cout << "Cleaning up.." << std::endl;

	tuio_stop();
	mt_stop();

	std::cout << "Program stopped." << std::endl;

	return 0;
}

void tongseng_set_hostname_and_port(const char* _hostname, int _port)
{
	host = _hostname;
	port = _port;
}

void tongseng_set_verbose(int _verbose)
{
	verbose = _verbose != 0;
}

void tongseng_start()
{
	mt_start();
	tuio_start();
}

void tongseng_stop()
{
	tuio_stop();
	mt_stop();
}

