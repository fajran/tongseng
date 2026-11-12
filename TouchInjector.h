// Copyright (C) 2026 Martin Kaltenbrunner <martin@tuio.org>
//
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

#ifndef TOUCHINJECTOR_H_
#define TOUCHINJECTOR_H_

#include <TuioListener.h>
#include <vector>
#include <map>

struct InjectedTouch {
	int id;		// stable contact index
	float x;	// normalized 0..1
	float y;	// normalized 0..1
};

// Receives a per-frame snapshot of active touches
class InjectorBackend {
public:
	virtual ~InjectorBackend() {}
	virtual void apply(const std::vector<InjectedTouch>& touches) = 0;
};

// Receives TUIO cursor events and injects them into macOS as input events.
// Uses a virtual HID digitizer when it can be created (system-level multitouch,
// requires com.apple.developer.hid.virtual.device), otherwise falls back to
// CGEvent mouse and gesture emulation.
class TouchInjector : public TUIO::TuioListener {

public:
	TouchInjector();
	~TouchInjector();

	void addTuioObject(TUIO::TuioObject *tobj) {}
	void updateTuioObject(TUIO::TuioObject *tobj) {}
	void removeTuioObject(TUIO::TuioObject *tobj) {}
	void addTuioCursor(TUIO::TuioCursor *tcur);
	void updateTuioCursor(TUIO::TuioCursor *tcur);
	void removeTuioCursor(TUIO::TuioCursor *tcur);
	void addTuioBlob(TUIO::TuioBlob *tblb) {}
	void updateTuioBlob(TUIO::TuioBlob *tblb) {}
	void removeTuioBlob(TUIO::TuioBlob *tblb) {}
	void refresh(TUIO::TuioTime ftime);

	// true when a virtual HID digitizer is driving the system, false when
	// the CGEvent mouse/gesture fallback is active
	bool usingVirtualDevice() const;

private:
	std::map<int,int> slots;	// session_id -> contact index
	std::vector<InjectedTouch> active;
	InjectorBackend *backend;

	int slotFor(int session_id);
};

#endif
