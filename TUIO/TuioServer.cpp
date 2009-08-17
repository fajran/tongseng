/*
	TUIO Server Component - part of the reacTIVision project
	http://reactivision.sourceforge.net/

	Copyright (C) 2005-2009 Martin Kaltenbrunner <martin@tuio.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "TuioServer.h"
using namespace TUIO;
using namespace osc;

TuioServer::TuioServer(OscSender *oscsnd)
	:full_update			(false)	
	,periodic_update		(false)	
	,objectProfileEnabled	(true)
	,cursorProfileEnabled	(true)
	,blobProfileEnabled		(true)
	,source_name			(NULL)
{

	this->sender = oscsnd;
	int size = sender->getBufferSize();
	oscBuffer = new char[size];
	oscPacket = new osc::OutboundPacketStream(oscBuffer,size);
	
	objectUpdateTime = TuioTime(currentFrameTime);
	cursorUpdateTime = TuioTime(currentFrameTime);
	blobUpdateTime = TuioTime(currentFrameTime);

	sendEmptyCursorBundle();
	sendEmptyObjectBundle();
	sendEmptyBlobBundle();
	
	invert_x = false;
	invert_y = false;
	invert_a = false;
}

TuioServer::~TuioServer() {
	sendEmptyCursorBundle();
	sendEmptyObjectBundle();
	sendEmptyBlobBundle();

	delete oscPacket;
	delete []oscBuffer;
	if(source_name) delete source_name;
}

void TuioServer::setSourceName(const char *src) {
	
	if (!source_name) source_name = new char[256];

	if (sender->isLocal()) {
		sprintf(source_name,src);
	} else { 
		char hostname[32];
		char *source_addr = NULL;
		struct hostent *hp;
		struct in_addr *addr;
		
		gethostname(hostname, 32);
		hp = gethostbyname(hostname);		
		for (int i = 0; hp->h_addr_list[i] != 0; ++i) {
			addr = (struct in_addr *)(hp->h_addr_list[i]);
			//std::cout << inet_ntoa(*addr) << std::endl;
			source_addr = inet_ntoa(*addr);
		}

		sprintf(source_name,"%s@%s",src,source_addr);
	} 
	
	//std::cout << "source: " << source_name << std::endl;
}

void TuioServer::commitFrame() {
	TuioManager::commitFrame();
	//std::cout << "TuioServer" << std::endl;
		
	if(updateObject) {
		startObjectBundle();
		for (std::list<TuioObject*>::iterator  tuioObject = objectList.begin(); tuioObject!=objectList.end(); tuioObject++) {
			
			// start a new packet if we exceed the packet capacity
			if ((oscPacket->Capacity()-oscPacket->Size())<OBJ_MESSAGE_SIZE) {
				sendObjectBundle(currentFrame);
				startObjectBundle();
			}
			TuioObject *tobj = (*tuioObject);
			if  ((full_update) || (tobj->getTuioTime()==currentFrameTime)) addObjectMessage(tobj);
		}
		objectUpdateTime = TuioTime(currentFrameTime);
		sendObjectBundle(currentFrame);
	} else if (objectProfileEnabled && periodic_update) {
		
		TuioTime timeCheck = currentFrameTime - objectUpdateTime;
		if(timeCheck.getSeconds()>=update_interval) {
			objectUpdateTime = TuioTime(currentFrameTime);
			startObjectBundle();
			if  (full_update) {
				for (std::list<TuioObject*>::iterator  tuioObject = objectList.begin(); tuioObject!=objectList.end(); tuioObject++) {
					// start a new packet if we exceed the packet capacity
					if ((oscPacket->Capacity()-oscPacket->Size())<OBJ_MESSAGE_SIZE) {
						sendObjectBundle(currentFrame);
						startObjectBundle();
					}
					addObjectMessage(*tuioObject);
				}
			}
			sendObjectBundle(currentFrame);
		}
	}
	updateObject = false;
	
	if(updateCursor) {
		startCursorBundle();
		for (std::list<TuioCursor*>::iterator tuioCursor = cursorList.begin(); tuioCursor!=cursorList.end(); tuioCursor++) {
			
			// start a new packet if we exceed the packet capacity
			if ((oscPacket->Capacity()-oscPacket->Size())<CUR_MESSAGE_SIZE) {
				sendCursorBundle(currentFrame);
				startCursorBundle();
			}
			TuioCursor *tcur = (*tuioCursor);
			if ((full_update) || (tcur->getTuioTime()==currentFrameTime)) addCursorMessage(tcur);				
		}
		cursorUpdateTime = TuioTime(currentFrameTime);
		sendCursorBundle(currentFrame);
	} else if (cursorProfileEnabled && periodic_update) {
		TuioTime timeCheck = currentFrameTime - cursorUpdateTime;
		if(timeCheck.getSeconds()>=update_interval) {
			cursorUpdateTime = TuioTime(currentFrameTime);
			startCursorBundle();
			if (full_update) {
				for (std::list<TuioCursor*>::iterator tuioCursor = cursorList.begin(); tuioCursor!=cursorList.end(); tuioCursor++) {
					// start a new packet if we exceed the packet capacity
					if ((oscPacket->Capacity()-oscPacket->Size())<CUR_MESSAGE_SIZE) {
						sendCursorBundle(currentFrame);
						startCursorBundle();
					}
					addCursorMessage(*tuioCursor);
				}
			}
			sendCursorBundle(currentFrame);
		}
	}
	updateCursor = false;
	
	if(updateBlob) {
		startBlobBundle();
		for (std::list<TuioBlob*>::iterator tuioBlob =blobList.begin(); tuioBlob!=blobList.end(); tuioBlob++) {
			
			// start a new packet if we exceed the packet capacity
			if ((oscPacket->Capacity()-oscPacket->Size())<BLB_MESSAGE_SIZE) {
				sendBlobBundle(currentFrame);
				startBlobBundle();
			}
			TuioBlob *tblb = (*tuioBlob);
			if ((full_update) || (tblb->getTuioTime()==currentFrameTime)) addBlobMessage(tblb);				
		}
		blobUpdateTime = TuioTime(currentFrameTime);
		sendBlobBundle(currentFrame);
	} else if (blobProfileEnabled && periodic_update) {
		TuioTime timeCheck = currentFrameTime - blobUpdateTime;
		if(timeCheck.getSeconds()>=update_interval) {
			blobUpdateTime = TuioTime(currentFrameTime);
			startBlobBundle();
			if (full_update) {
				for (std::list<TuioBlob*>::iterator tuioBlob =blobList.begin(); tuioBlob!=blobList.end(); tuioBlob++) {
				
					// start a new packet if we exceed the packet capacity
					if ((oscPacket->Capacity()-oscPacket->Size())<BLB_MESSAGE_SIZE) {
						sendBlobBundle(currentFrame);
						startBlobBundle();
					}
					addBlobMessage(*tuioBlob);
				}
			}
			sendBlobBundle(currentFrame);
		}
	}
	updateBlob = false;
}

void TuioServer::sendEmptyCursorBundle() {
	oscPacket->Clear();	
	(*oscPacket) << osc::BeginBundleImmediate;
	if (source_name) (*oscPacket) << osc::BeginMessage( "/tuio/2Dcur") << "source" << source_name << osc::EndMessage;
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dcur") << "alive" << osc::EndMessage;	
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dcur") << "fseq" << -1 << osc::EndMessage;
	(*oscPacket) << osc::EndBundle;
	sender->sendOSC( oscPacket );
}



void TuioServer::startCursorBundle() {	
	oscPacket->Clear();	
	(*oscPacket) << osc::BeginBundleImmediate;
	if (source_name) (*oscPacket) << osc::BeginMessage( "/tuio/2Dcur") << "source" << source_name << osc::EndMessage;
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dcur") << "alive";
	for (std::list<TuioCursor*>::iterator tuioCursor = cursorList.begin(); tuioCursor!=cursorList.end(); tuioCursor++) {
		(*oscPacket) << (int32)((*tuioCursor)->getSessionID());	
	}
	(*oscPacket) << osc::EndMessage;	
}

void TuioServer::addCursorMessage(TuioCursor *tcur) {

	float xpos = tcur->getX();
	float xvel = tcur->getXSpeed();
	if (invert_x) {
		xpos = 1 - xpos;
		xvel = -1 * xvel;
	}
	float ypos = tcur->getY();
	float yvel = tcur->getYSpeed();
	if (invert_y) {
		ypos = 1 - ypos;
		yvel = -1 * yvel;
	}

	(*oscPacket) << osc::BeginMessage( "/tuio/2Dcur") << "set";
	(*oscPacket) << (int32)(tcur->getSessionID()) << xpos << ypos;
	(*oscPacket) << xvel << yvel << tcur->getMotionAccel();	
	(*oscPacket) << osc::EndMessage;
}

void TuioServer::sendCursorBundle(long fseq) {
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dcur") << "fseq" << (int32)fseq << osc::EndMessage;
	(*oscPacket) << osc::EndBundle;
	sender->sendOSC( oscPacket );
}

void TuioServer::sendEmptyObjectBundle() {
	oscPacket->Clear();	
	(*oscPacket) << osc::BeginBundleImmediate;
	if (source_name) (*oscPacket) << osc::BeginMessage( "/tuio/2Dobj") << "source" << source_name << osc::EndMessage;
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dobj") << "alive" << osc::EndMessage;	
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dobj") << "fseq" << -1 << osc::EndMessage;
	(*oscPacket) << osc::EndBundle;
	sender->sendOSC( oscPacket );
}

void TuioServer::startObjectBundle() {
	oscPacket->Clear();	
	(*oscPacket) << osc::BeginBundleImmediate;
	if (source_name) (*oscPacket) << osc::BeginMessage( "/tuio/2Dobj") << "source" << source_name << osc::EndMessage;
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dobj") << "alive";
	for (std::list<TuioObject*>::iterator tuioObject = objectList.begin(); tuioObject!=objectList.end(); tuioObject++) {
		(*oscPacket) << (int32)((*tuioObject)->getSessionID());	
	}
	(*oscPacket) << osc::EndMessage;
}

void TuioServer::addObjectMessage(TuioObject *tobj) {
	
	float xpos = tobj->getX();
	float xvel = tobj->getXSpeed();
	if (invert_x) {
		xpos = 1 - xpos;
		xvel = -1 * xvel;
	}
	float ypos = tobj->getY();
	float yvel = tobj->getYSpeed();
	if (invert_y) {
		ypos = 1 - ypos;
		yvel = -1 * yvel;
	}
	float angle = tobj->getAngle();
	float rvel = tobj->getRotationSpeed();
	if (invert_a) {
		angle = 1 - angle;
		rvel = -1 * rvel;
	}
	
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dobj") << "set";
	(*oscPacket) << (int32)(tobj->getSessionID()) << tobj->getSymbolID() << xpos << ypos << angle;
	(*oscPacket) << xvel << yvel << rvel << tobj->getMotionAccel() << tobj->getRotationAccel();	
	(*oscPacket) << osc::EndMessage;
}

void TuioServer::sendObjectBundle(long fseq) {
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dobj") << "fseq" << (int32)fseq << osc::EndMessage;
	(*oscPacket) << osc::EndBundle;
	sender->sendOSC( oscPacket );
}


void TuioServer::sendEmptyBlobBundle() {
	oscPacket->Clear();	
	(*oscPacket) << osc::BeginBundleImmediate;
	if (source_name) (*oscPacket) << osc::BeginMessage( "/tuio/2Dblb") << "source" << source_name << osc::EndMessage;
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dblb") << "alive" << osc::EndMessage;	
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dblb") << "fseq" << -1 << osc::EndMessage;
	(*oscPacket) << osc::EndBundle;
	sender->sendOSC( oscPacket );
}

void TuioServer::startBlobBundle() {	
	oscPacket->Clear();	
	(*oscPacket) << osc::BeginBundleImmediate;
	if (source_name) (*oscPacket) << osc::BeginMessage( "/tuio/2Dblb") << "source" << source_name << osc::EndMessage;
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dblb") << "alive";
	for (std::list<TuioBlob*>::iterator tuioBlob = blobList.begin(); tuioBlob!=blobList.end(); tuioBlob++) {
		(*oscPacket) << (int32)((*tuioBlob)->getSessionID());	
	}
	(*oscPacket) << osc::EndMessage;	
}

void TuioServer::addBlobMessage(TuioBlob *tblb) {
	
	float xpos = tblb->getX();
	float xvel = tblb->getXSpeed();
	if (invert_x) {
		xpos = 1 - xpos;
		xvel = -1 * xvel;
	}
	float ypos = tblb->getY();
	float yvel = tblb->getYSpeed();
	if (invert_y) {
		ypos = 1 - ypos;
		yvel = -1 * yvel;
	}
	float angle = tblb->getAngle();
	float rvel = tblb->getRotationSpeed();
	if (invert_a) {
		angle = 1 - angle;
		rvel = -1 * rvel;
	}
	
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dblb") << "set";
	(*oscPacket) << (int32)(tblb->getSessionID()) << xpos << ypos << angle << tblb->getWidth() << tblb->getHeight() << tblb->getArea();
	(*oscPacket) << xvel << yvel  << rvel << tblb->getMotionAccel()  << tblb->getRotationAccel();	
	(*oscPacket) << osc::EndMessage;
}

void TuioServer::sendBlobBundle(long fseq) {
	(*oscPacket) << osc::BeginMessage( "/tuio/2Dblb") << "fseq" << (int32)fseq << osc::EndMessage;
	(*oscPacket) << osc::EndBundle;
	sender->sendOSC( oscPacket );
}




