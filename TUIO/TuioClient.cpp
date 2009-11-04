/*
 TUIO C++ Library - part of the reacTIVision project
 http://reactivision.sourceforge.net/
 
 Copyright (c) 2005-2009 Martin Kaltenbrunner <martin@tuio.org>
 
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

#include "TuioClient.h"

using namespace TUIO;
using namespace osc;

#ifndef WIN32
static void* ClientThreadFunc( void* obj )
#else
static DWORD WINAPI ClientThreadFunc( LPVOID obj )
#endif
{
	static_cast<TuioClient*>(obj)->socket->Run();
	return 0;
};

TuioClient::TuioClient(int port)
: socket      (NULL)
, currentFrame(-1)
, source_id   (0)
, thread      (NULL)
, locked      (false)
, connected   (false)

{
	try {
		socket = new UdpListeningReceiveSocket(IpEndpointName( IpEndpointName::ANY_ADDRESS, port ), this );
	} catch (std::exception &e) { 
		std::cerr << "could not bind to UDP port " << port << std::endl;
		socket = NULL;
	}
	
	if (socket!=NULL) {
		if (!socket->IsBound()) {
			delete socket;
			socket = NULL;
		} else std::cout << "listening to TUIO messages on UDP port " << port << std::endl;
	}
	
	source_name = NULL;
	source_addr = NULL;
}

TuioClient::~TuioClient() {	
	delete socket;
}

void TuioClient::ProcessBundle( const ReceivedBundle& b, const IpEndpointName& remoteEndpoint) {
	
	try {
		for( ReceivedBundle::const_iterator i = b.ElementsBegin(); i != b.ElementsEnd(); ++i ){
			if( i->IsBundle() )
				ProcessBundle( ReceivedBundle(*i), remoteEndpoint);
			else
				ProcessMessage( ReceivedMessage(*i), remoteEndpoint);
		}
	} catch (MalformedBundleException& e) {
		std::cerr << "malformed OSC bundle" << std::endl << e.what() << std::endl;
	}
	
}

void TuioClient::ProcessMessage( const ReceivedMessage& msg, const IpEndpointName& remoteEndpoint) {
	try {
		ReceivedMessageArgumentStream args = msg.ArgumentStream();
		ReceivedMessage::const_iterator arg = msg.ArgumentsBegin();
		
		if( strcmp( msg.AddressPattern(), "/tuio/2Dobj" ) == 0 ){
			
			const char* cmd;
			args >> cmd;
			
			if (strcmp(cmd,"source")==0) {
				const char* src;
				args >> src;
				
				source_name = strtok((char*)src, "@");
				char *addr = strtok(NULL, "@");
				
				if (addr!=NULL) source_addr = addr;
				else source_addr = (char*)"127.0.0.1";
				
				int src_id = 0;
				std::list<std::string>::iterator iter;
				// look if we know this source already
				for (iter=sourceList.begin(); iter != sourceList.end(); iter++) {
					if ((*iter)==std::string(src)){
						source_id = src_id;
						break;
					} else src_id++;
				}
				
				// add a new source
				if (iter==sourceList.end()) {
					sourceList.push_back(std::string(src));
					source_id = sourceList.size()-1;
				}
				
			} else if (strcmp(cmd,"set")==0) {	
												
				int32 s_id, c_id;
				float xpos, ypos, angle, xspeed, yspeed, rspeed, maccel, raccel;
				args >> s_id >> c_id >> xpos >> ypos >> angle >> xspeed >> yspeed >> rspeed >> maccel >> raccel;
				
				lockObjectList();
				std::list<TuioObject*>::iterator tobj;
				for (tobj=objectList.begin(); tobj!= objectList.end(); tobj++)
					if((*tobj)->getSessionID()==(long)s_id) break;
				
				if (tobj == objectList.end()) {
					
					TuioObject *addObject = new TuioObject((long)s_id,(int)c_id,xpos,ypos,angle);
					frameObjects.push_back(addObject);

				} else if ( ((*tobj)->getX()!=xpos) || ((*tobj)->getY()!=ypos) || ((*tobj)->getAngle()!=angle) || ((*tobj)->getXSpeed()!=xspeed) || ((*tobj)->getYSpeed()!=yspeed) || ((*tobj)->getRotationSpeed()!=rspeed) || ((*tobj)->getMotionAccel()!=maccel) || ((*tobj)->getRotationAccel()!=raccel) ) {

					TuioObject *updateObject = new TuioObject((long)s_id,(*tobj)->getSymbolID(),xpos,ypos,angle);
					updateObject->update(xpos,ypos,angle,xspeed,yspeed,rspeed,maccel,raccel);
					frameObjects.push_back(updateObject);
					
				}
				unlockObjectList();

			} else if (strcmp(cmd,"alive")==0) {
				
				int32 s_id;
				aliveObjectList.clear();
				while(!args.Eos()) {
					args >> s_id;
					aliveObjectList.push_back((long)s_id);
				}
				
			} else if (strcmp(cmd,"fseq")==0) {
				
				int32 fseq;
				args >> fseq;
				bool lateFrame = false;
				if (fseq>0) {
					if (fseq>currentFrame) currentTime = TuioTime::getSessionTime();
					if ((fseq>=currentFrame) || ((currentFrame-fseq)>100)) currentFrame = fseq;
					else lateFrame = true;
				} else if ((TuioTime::getSessionTime().getTotalMilliseconds()-currentTime.getTotalMilliseconds())>100) {
					currentTime = TuioTime::getSessionTime();
				}
			
				if (!lateFrame) {
					
					//find the removed objects first
					lockObjectList();
					for (std::list<TuioObject*>::iterator tobj=objectList.begin(); tobj != objectList.end(); tobj++) {
						if ((*tobj)->getTuioSourceID()==source_id) {
							std::list<long>::iterator iter = find(aliveObjectList.begin(), aliveObjectList.end(), (*tobj)->getSessionID());
							if (iter == aliveObjectList.end()) {
								(*tobj)->remove(currentTime);
								frameObjects.push_back(*tobj);							
							}
						}
					}
					unlockObjectList();
					
					for (std::list<TuioObject*>::iterator iter=frameObjects.begin(); iter != frameObjects.end(); iter++) {
						TuioObject *tobj = (*iter);

						TuioObject *frameObject;
						switch (tobj->getTuioState()) {
							case TUIO_REMOVED:
								frameObject = tobj;
								frameObject->remove(currentTime);

								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->removeTuioObject(frameObject);

								lockObjectList();
								for (std::list<TuioObject*>::iterator delobj=objectList.begin(); delobj!=objectList.end(); delobj++) {
									if((*delobj)->getSessionID()==frameObject->getSessionID()) {
										objectList.erase(delobj);
										break;
									}
								}
								unlockObjectList();
								
								break;
							case TUIO_ADDED:
								frameObject = new TuioObject(currentTime,tobj->getSessionID(),tobj->getSymbolID(),tobj->getX(),tobj->getY(),tobj->getAngle());
								if (source_name) frameObject->setTuioSource(source_id,source_name,source_addr);
								lockObjectList();
								objectList.push_back(frameObject);
								unlockObjectList();
	
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->addTuioObject(frameObject);
								
								break;
							default:
								frameObject = getTuioObject(source_id,tobj->getSessionID());
								if ( (tobj->getX()!=frameObject->getX() && tobj->getXSpeed()==0) || (tobj->getY()!=frameObject->getY() && tobj->getYSpeed()==0) )
									frameObject->update(currentTime,tobj->getX(),tobj->getY(),tobj->getAngle());
								else
									frameObject->update(currentTime,tobj->getX(),tobj->getY(),tobj->getAngle(),tobj->getXSpeed(),tobj->getYSpeed(),tobj->getRotationSpeed(),tobj->getMotionAccel(),tobj->getRotationAccel());
								
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->updateTuioObject(frameObject);
								
						}
						delete tobj;
					}
					
					for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
						(*listener)->refresh(currentTime);
										
				} else {
					for (std::list<TuioObject*>::iterator iter=frameObjects.begin(); iter != frameObjects.end(); iter++) {
						TuioObject *tobj = (*iter);
						delete tobj;
					}
				}
				
				frameObjects.clear();
			}
		} else if( strcmp( msg.AddressPattern(), "/tuio/2Dcur" ) == 0 ) {
			const char* cmd;
			args >> cmd;
			
			if (strcmp(cmd,"source")==0) {
				const char* src;
				args >> src;
				
				source_name = strtok((char*)src, "@");
				char *addr = strtok(NULL, "@");
				
				if (addr!=NULL) source_addr = addr;
				else source_addr = (char*)"127.0.0.1";
				
				int src_id = 0;
				std::list<std::string>::iterator iter;
				// look if we know this source already
				for (iter=sourceList.begin(); iter != sourceList.end(); iter++) {
					if ((*iter)==std::string(src)){
						source_id = src_id;
						break;
					} else src_id++;
				}
				
				// add a new source
				if (iter==sourceList.end()) {
					sourceList.push_back(std::string(src));
					source_id = sourceList.size()-1;
					maxCursorID[source_id]=-1;
				}
				
			} else if (strcmp(cmd,"set")==0) {	

				int32 s_id;
				float xpos, ypos, xspeed, yspeed, maccel;				
				args >> s_id >> xpos >> ypos >> xspeed >> yspeed >> maccel;
				
				lockCursorList();
				std::list<TuioCursor*>::iterator tcur;
				for (tcur=cursorList.begin(); tcur!= cursorList.end(); tcur++)
					if (((*tcur)->getSessionID()==(long)s_id) && ((*tcur)->getTuioSourceID()==source_id)) break;
				
				if (tcur==cursorList.end()) {
									
					TuioCursor *addCursor = new TuioCursor((long)s_id,-1,xpos,ypos);
					frameCursors.push_back(addCursor);

				} else if ( ((*tcur)->getX()!=xpos) || ((*tcur)->getY()!=ypos) || ((*tcur)->getXSpeed()!=xspeed) || ((*tcur)->getYSpeed()!=yspeed) || ((*tcur)->getMotionAccel()!=maccel) ) {

					TuioCursor *updateCursor = new TuioCursor((long)s_id,(*tcur)->getCursorID(),xpos,ypos);
					updateCursor->update(xpos,ypos,xspeed,yspeed,maccel);
					frameCursors.push_back(updateCursor);

				}
				unlockCursorList();
				
			} else if (strcmp(cmd,"alive")==0) {
				
				int32 s_id;
				aliveCursorList.clear();
				while(!args.Eos()) {
					args >> s_id;
					aliveCursorList.push_back((long)s_id);
				}
				
			} else if( strcmp( cmd, "fseq" ) == 0 ){
				int32 fseq;
				args >> fseq;
				bool lateFrame = false;
				if (fseq>0) {
					if (fseq>currentFrame) currentTime = TuioTime::getSessionTime();
					if ((fseq>=currentFrame) || ((currentFrame-fseq)>100)) currentFrame = fseq;
					else lateFrame = true;
				}  else if ((TuioTime::getSessionTime().getTotalMilliseconds()-currentTime.getTotalMilliseconds())>100) {
					currentTime = TuioTime::getSessionTime();
				}
			
				if (!lateFrame) {
					
					// find the removed cursors first
					lockCursorList();
					for (std::list<TuioCursor*>::iterator tcur=cursorList.begin(); tcur != cursorList.end(); tcur++) {
						if ((*tcur)->getTuioSourceID()==source_id) {
							std::list<long>::iterator iter = find(aliveCursorList.begin(), aliveCursorList.end(), (*tcur)->getSessionID());
							
							if (iter == aliveCursorList.end()) {
									(*tcur)->remove(currentTime);
									frameCursors.push_back(*tcur);
							}
						}
					}
					unlockCursorList();
					
					for (std::list<TuioCursor*>::iterator iter=frameCursors.begin(); iter != frameCursors.end(); iter++) {
						TuioCursor *tcur = (*iter);
						
						int c_id = 0;
						int free_size = 0;
						TuioCursor *frameCursor;
						switch (tcur->getTuioState()) {
							case TUIO_REMOVED:
								frameCursor = tcur;
								frameCursor->remove(currentTime);
	
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->removeTuioCursor(frameCursor);

								lockCursorList();
								for (std::list<TuioCursor*>::iterator delcur=cursorList.begin(); delcur!=cursorList.end(); delcur++) {
									if(((*delcur)->getTuioSourceID()==source_id) && ((*delcur)->getSessionID()==frameCursor->getSessionID())) {
										cursorList.erase(delcur);
										break;
									}
								}

								if (frameCursor->getCursorID()==maxCursorID[source_id]) {
									maxCursorID[source_id] = -1;
									delete frameCursor;
									
									if (cursorList.size()>0) {
										std::list<TuioCursor*>::iterator clist;
										for (clist=cursorList.begin(); clist != cursorList.end(); clist++) {
											if ((*clist)->getTuioSourceID()==source_id) {
												c_id = (*clist)->getCursorID();
												if (c_id>maxCursorID[source_id]) maxCursorID[source_id]=c_id;
											}
										}
			
										freeCursorBuffer.clear();
										for (std::list<TuioCursor*>::iterator flist=freeCursorList.begin(); flist != freeCursorList.end(); flist++) {
											TuioCursor *freeCursor = (*flist);
											if (freeCursor->getTuioSourceID()==source_id) {
												if (freeCursor->getCursorID()>maxCursorID[source_id]) delete freeCursor;
												else freeCursorBuffer.push_back(freeCursor);
											} else freeCursorBuffer.push_back(freeCursor);
										}	
										freeCursorList = freeCursorBuffer;

									} else {
										freeCursorBuffer.clear();
										for (std::list<TuioCursor*>::iterator flist=freeCursorList.begin(); flist != freeCursorList.end(); flist++) {
											TuioCursor *freeCursor = (*flist);
											if (freeCursor->getTuioSourceID()==source_id) delete freeCursor;
											else freeCursorBuffer.push_back(freeCursor);
										}	
										freeCursorList = freeCursorBuffer;
										
									}
								} else if (frameCursor->getCursorID()<maxCursorID[source_id]) {
									freeCursorList.push_back(frameCursor);
								} 
								unlockCursorList();
								
								break;
							case TUIO_ADDED:
								
								for(std::list<TuioCursor*>::iterator iter = cursorList.begin();iter!= cursorList.end(); iter++)
									if ((*iter)->getTuioSourceID()==source_id) c_id++;
								
								for(std::list<TuioCursor*>::iterator iter = freeCursorList.begin();iter!= freeCursorList.end(); iter++)
									if ((*iter)->getTuioSourceID()==source_id) free_size++;
								
								if ((free_size<=maxCursorID[source_id]) && (free_size>0)) {
									std::list<TuioCursor*>::iterator closestCursor = freeCursorList.end();
									
									for(std::list<TuioCursor*>::iterator iter = freeCursorList.begin();iter!= freeCursorList.end(); iter++) {
										if (((*iter)->getTuioSourceID()==source_id) && ((*iter)->getDistance(tcur)<(*closestCursor)->getDistance(tcur))) closestCursor = iter;
									}
									
									if (closestCursor!=freeCursorList.end()) {
										TuioCursor *freeCursor = (*closestCursor);
										c_id = freeCursor->getCursorID();
										freeCursorList.erase(closestCursor);
										delete freeCursor;
									}
								} else maxCursorID[source_id] = c_id;									
								
								frameCursor = new TuioCursor(currentTime,tcur->getSessionID(),c_id,tcur->getX(),tcur->getY());
								if (source_name) frameCursor->setTuioSource(source_id,source_name,source_addr);
								lockCursorList();
								cursorList.push_back(frameCursor);
								unlockCursorList();
								
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->addTuioCursor(frameCursor);
								delete tcur;
								break;
							default:
								
								frameCursor = getTuioCursor(source_id,tcur->getSessionID());
								if ( (tcur->getX()!=frameCursor->getX() && tcur->getXSpeed()==0) || (tcur->getY()!=frameCursor->getY() && tcur->getYSpeed()==0) )
									frameCursor->update(currentTime,tcur->getX(),tcur->getY());
								else
									frameCursor->update(currentTime,tcur->getX(),tcur->getY(),tcur->getXSpeed(),tcur->getYSpeed(),tcur->getMotionAccel());
						
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->updateTuioCursor(frameCursor);
								delete tcur;
						}	
					}
					
					for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
						(*listener)->refresh(currentTime);
					
					
				} else {
					for (std::list<TuioCursor*>::iterator iter=frameCursors.begin(); iter != frameCursors.end(); iter++) {
						TuioCursor *tcur = (*iter);
						delete tcur;
					}
				}
				
				frameCursors.clear();
			}
		} else if( strcmp( msg.AddressPattern(), "/tuio/2Dblb" ) == 0 ){
			const char* cmd;
			args >> cmd;
			
			if (strcmp(cmd,"source")==0) {	
				const char* src;
				args >> src;
				
				source_name = strtok((char*)src, "@");
				char *addr = strtok(NULL, "@");
				
				if (addr!=NULL) source_addr = addr;
				else source_addr = (char*)"127.0.0.1";
				
				int src_id = 0;
				std::list<std::string>::iterator iter;
				// look if we know this source already
				for (iter=sourceList.begin(); iter != sourceList.end(); iter++) {
					if ((*iter)==std::string(src)){
						source_id = src_id;
						break;
					} else src_id++;
				}
				
				// add a new source
				if (iter==sourceList.end()) {
					sourceList.push_back(std::string(src));
					source_id = sourceList.size()-1;
					maxBlobID[source_id]=-1;
				}
				
			} else if (strcmp(cmd,"set")==0) {	
				
				int32 s_id;
				float xpos, ypos, angle, width, height, area, xspeed, yspeed, rspeed, maccel, raccel;				
				args >> s_id >> xpos >> ypos >> angle >> width >> height >> area >> xspeed >> yspeed >> rspeed >> maccel >> raccel;
				
				lockBlobList();
				std::list<TuioBlob*>::iterator tblb;
				for (tblb=blobList.begin(); tblb!= blobList.end(); tblb++)
					if((*tblb)->getSessionID()==(long)s_id) break;
				
				if (tblb==blobList.end()) {
					
					TuioBlob *addBlob = new TuioBlob((long)s_id,-1,xpos,ypos,angle,width,height,area);
					frameBlobs.push_back(addBlob);
					
				} else if ( ((*tblb)->getX()!=xpos) || ((*tblb)->getY()!=ypos) || ((*tblb)->getXSpeed()!=xspeed) || ((*tblb)->getYSpeed()!=yspeed) || ((*tblb)->getMotionAccel()!=maccel) ) {
					
					TuioBlob *updateBlob = new TuioBlob((long)s_id,(*tblb)->getBlobID(),xpos,ypos,angle,width,height,area);
					updateBlob->update(xpos,ypos,angle,width,height,area,xspeed,yspeed,rspeed,maccel,raccel);
					frameBlobs.push_back(updateBlob);
				}
				unlockBlobList();
				
			} else if (strcmp(cmd,"alive")==0) {
				
				int32 s_id;
				aliveBlobList.clear();
				while(!args.Eos()) {
					args >> s_id;
					aliveBlobList.push_back((long)s_id);
				}
				
			} else if( strcmp( cmd, "fseq" ) == 0 ){
				
				int32 fseq;
				args >> fseq;
				bool lateFrame = false;
				if (fseq>0) {
					if (fseq>currentFrame) currentTime = TuioTime::getSessionTime();
					if ((fseq>=currentFrame) || ((currentFrame-fseq)>100)) currentFrame = fseq;
					else lateFrame = true;
				}  else if ((TuioTime::getSessionTime().getTotalMilliseconds()-currentTime.getTotalMilliseconds())>100) {
					currentTime = TuioTime::getSessionTime();
				}
				
				if (!lateFrame) {
					
					// find the removed blobs first
					lockBlobList();
					for (std::list<TuioBlob*>::iterator tblb=blobList.begin(); tblb != blobList.end(); tblb++) {
						if ((*tblb)->getTuioSourceID()==source_id) {
							std::list<long>::iterator iter = find(aliveBlobList.begin(), aliveBlobList.end(), (*tblb)->getSessionID());
							
							if (iter == aliveBlobList.end()) {
								(*tblb)->remove(currentTime);
								frameBlobs.push_back(*tblb);
							}
						}
					}
					unlockBlobList();
					
					for (std::list<TuioBlob*>::iterator iter=frameBlobs.begin(); iter != frameBlobs.end(); iter++) {
						TuioBlob *tblb = (*iter);
						
						int b_id = 0;
						int free_size = 0;
						TuioBlob *frameBlob;
						switch (tblb->getTuioState()) {
							case TUIO_REMOVED:
								frameBlob = tblb;
								frameBlob->remove(currentTime);
								
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->removeTuioBlob(frameBlob);
								
								lockBlobList();
								for (std::list<TuioBlob*>::iterator delblb=blobList.begin(); delblb!=blobList.end(); delblb++) {
									if(((*delblb)->getTuioSourceID()==source_id) && ((*delblb)->getSessionID()==frameBlob->getSessionID())) {
										blobList.erase(delblb);
										break;
									}
								}
								
								if (frameBlob->getBlobID()==maxBlobID[source_id]) {
									maxBlobID[source_id] = -1;
									delete frameBlob;
									
									if (blobList.size()>0) {
										std::list<TuioBlob*>::iterator clist;
										for (clist=blobList.begin(); clist != blobList.end(); clist++) {
											if ((*clist)->getTuioSourceID()==source_id) {
												b_id = (*clist)->getBlobID();
												if (b_id>maxBlobID[source_id]) maxBlobID[source_id]=b_id;
											}
										}
										
										freeBlobBuffer.clear();
										for (std::list<TuioBlob*>::iterator flist=freeBlobList.begin(); flist != freeBlobList.end(); flist++) {
											TuioBlob *freeBlob = (*flist);
											if (freeBlob->getTuioSourceID()==source_id) {
												if (freeBlob->getBlobID()>maxBlobID[source_id]) delete freeBlob;
												else freeBlobBuffer.push_back(freeBlob);
											} else freeBlobBuffer.push_back(freeBlob);
										}	
										freeBlobList = freeBlobBuffer;
										
									} else {
										freeBlobBuffer.clear();
										for (std::list<TuioBlob*>::iterator flist=freeBlobList.begin(); flist != freeBlobList.end(); flist++) {
											TuioBlob *freeBlob = (*flist);
											if (freeBlob->getTuioSourceID()==source_id) delete freeBlob;
											else freeBlobBuffer.push_back(freeBlob);
										}	
										freeBlobList = freeBlobBuffer;
										
									}
								} else if (frameBlob->getBlobID()<maxBlobID[source_id]) {
									freeBlobList.push_back(frameBlob);
								} 
								unlockBlobList();
								
								break;
							case TUIO_ADDED:
								
								for(std::list<TuioBlob*>::iterator iter = blobList.begin();iter!= blobList.end(); iter++)
									if ((*iter)->getTuioSourceID()==source_id) b_id++;
								
								for(std::list<TuioBlob*>::iterator iter = freeBlobList.begin();iter!= freeBlobList.end(); iter++)
									if ((*iter)->getTuioSourceID()==source_id) free_size++;
								
								if ((free_size<=maxBlobID[source_id]) && (free_size>0)) {
									std::list<TuioBlob*>::iterator closestBlob = freeBlobList.end();
									
									for(std::list<TuioBlob*>::iterator iter = freeBlobList.begin();iter!= freeBlobList.end(); iter++) {
										if (((*iter)->getTuioSourceID()==source_id) && ((*iter)->getDistance(tblb)<(*closestBlob)->getDistance(tblb))) closestBlob = iter;
									}
									
									if (closestBlob!=freeBlobList.end()) {
										TuioBlob *freeBlob = (*closestBlob);
										b_id = freeBlob->getBlobID();
										freeBlobList.erase(closestBlob);
										delete freeBlob;
									}
								} else maxBlobID[source_id] = b_id;									
								
								frameBlob = new TuioBlob(currentTime,tblb->getSessionID(),b_id,tblb->getX(),tblb->getY(),tblb->getAngle(),tblb->getWidth(),tblb->getHeight(),tblb->getArea());
								if (source_name) frameBlob->setTuioSource(source_id,source_name,source_addr);
								lockBlobList();
								blobList.push_back(frameBlob);
								unlockBlobList();
								
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->addTuioBlob(frameBlob);
								delete tblb;
								break;
							default:
								
								frameBlob = getTuioBlob(source_id,tblb->getSessionID());
								if ( (tblb->getX()!=frameBlob->getX() && tblb->getXSpeed()==0) || (tblb->getY()!=frameBlob->getY() && tblb->getYSpeed()==0) || (tblb->getAngle()!=frameBlob->getAngle() && tblb->getRotationSpeed()==0) )
									frameBlob->update(currentTime,tblb->getX(),tblb->getY());
								else
									frameBlob->update(currentTime,tblb->getX(),tblb->getY(),tblb->getXSpeed(),tblb->getYSpeed(),tblb->getMotionAccel());
								
								for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
									(*listener)->updateTuioBlob(frameBlob);
								delete tblb;
						}	
					}
					
					for (std::list<TuioListener*>::iterator listener=listenerList.begin(); listener != listenerList.end(); listener++)
						(*listener)->refresh(currentTime);
					
					
				} else {
					for (std::list<TuioBlob*>::iterator iter=frameBlobs.begin(); iter != frameBlobs.end(); iter++) {
						TuioBlob *tblb = (*iter);
						delete tblb;
					}
				}
				
				frameBlobs.clear();
			}
		}
	} catch( Exception& e ){
		std::cerr << "error parsing TUIO message: "<< msg.AddressPattern() <<  " - " << e.what() << std::endl;
	}
}

void TuioClient::ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint ) {
	ReceivedPacket p( data, size );
	if(p.IsBundle()) ProcessBundle( ReceivedBundle(p), remoteEndpoint);
	else ProcessMessage( ReceivedMessage(p), remoteEndpoint);
}

TuioObject* TuioClient::getTuioObject(int src_id, long s_id) {
	lockObjectList();
	for (std::list<TuioObject*>::iterator iter=objectList.begin(); iter != objectList.end(); iter++) {
		if (((*iter)->getTuioSourceID()==src_id) && ((*iter)->getSessionID()==s_id)) {
			unlockCursorList();
			return (*iter);
		}
	}	
	unlockObjectList();
	return NULL;
}

TuioCursor* TuioClient::getTuioCursor(int src_id, long s_id) {
	lockCursorList();
	for (std::list<TuioCursor*>::iterator iter=cursorList.begin(); iter != cursorList.end(); iter++) {
		if (((*iter)->getTuioSourceID()==src_id) && ((*iter)->getSessionID()==s_id)) {
			unlockCursorList();
			return (*iter);
		}
	}	
	unlockCursorList();
	return NULL;
}

TuioBlob* TuioClient::getTuioBlob(int src_id, long s_id) {
	lockBlobList();
	for (std::list<TuioBlob*>::iterator iter=blobList.begin(); iter != blobList.end(); iter++) {
		if (((*iter)->getTuioSourceID()==src_id) && ((*iter)->getSessionID()==s_id)) {
			unlockBlobList();
			return (*iter);
		}
	}	
	unlockBlobList();
	return NULL;
}

void TuioClient::connect(bool lk) {
			
	if (socket==NULL) return;
	TuioTime::initSession();
	currentTime.reset();
	
	locked = lk;
	if (!locked) {
#ifndef WIN32
		pthread_create(&thread , NULL, ClientThreadFunc, this);
#else
		DWORD threadId;
		thread = CreateThread( 0, 0, ClientThreadFunc, this, 0, &threadId );
#endif
	} else socket->Run();
	
	connected = true;
	unlockCursorList();
	unlockObjectList();
	unlockBlobList();
}

void TuioClient::disconnect() {
	
	if (socket==NULL) return;
	socket->Break();
	
	if (!locked) {
#ifdef WIN32
		if( thread ) CloseHandle( thread );
#endif
		thread = 0;
		locked = false;
	}
	
	aliveObjectList.clear();
	aliveCursorList.clear();
	aliveBlobList.clear();

	for (std::list<TuioObject*>::iterator iter=objectList.begin(); iter != objectList.end(); iter++)
		delete (*iter);
	objectList.clear();

	for (std::list<TuioCursor*>::iterator iter=cursorList.begin(); iter != cursorList.end(); iter++)
		delete (*iter);
	cursorList.clear();

	for (std::list<TuioBlob*>::iterator iter=blobList.begin(); iter != blobList.end(); iter++)
		delete (*iter);
	blobList.clear();
	
	for (std::list<TuioCursor*>::iterator iter=freeCursorList.begin(); iter != freeCursorList.end(); iter++)
		delete(*iter);
	freeCursorList.clear();

	for (std::list<TuioBlob*>::iterator iter=freeBlobList.begin(); iter != freeBlobList.end(); iter++)
		delete(*iter);
	freeBlobList.clear();
	
	connected = false;
}


