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

#ifndef INCLUDED_TUIOBLOB_H
#define INCLUDED_TUIOBLOB_H

#include "TuioContainer.h"

namespace TUIO {
	
	/**
	 * The TuioBlob class encapsulates /tuio/2Dblb TUIO objects.
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.4
	 */ 
	class LIBDECL TuioBlob: public TuioContainer {
		
	protected:
		/**
		 * The individual blob ID number that is assigned to each TuioBlob.
		 */ 
		int blob_id;
		/**
		 * The rotation angle value.
		 */ 
		float angle;
		/**
		 * The width value.
		 */ 
		float width;
		/**
		 * The height value.
		 */ 
		float height;
		/**
		 * The area value.
		 */ 
		float area;
		/**
		 * The rotation speed value.
		 */ 
		float rotation_speed;
		/**
		 * The rotation acceleration value.
		 */ 
		float rotation_accel;
		
	public:
		using TuioContainer::update;
		
		/**
		 * This constructor takes a TuioTime argument and assigns it along with the provided 
		 * Session ID, X and Y coordinate, width, height and angle to the newly created TuioBlob.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	si	the Session ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 */
		TuioBlob (TuioTime ttime, long si, int bi, float xp, float yp, float a, float w, float h, float f):TuioContainer(ttime, si, xp, yp) {
			blob_id = bi;
			angle = a;
			width = w;
			height = h;
			area = f;
			rotation_speed = 0.0f;
			rotation_accel = 0.0f;
		};

		/**
		 * This constructor takes the provided Session ID, X and Y coordinate 
		 *  width, height and angle, and assigs these values to the newly created TuioBlob.
		 *
		 * @param	si	the Session ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 */	
		TuioBlob (long si, int bi, float xp, float yp, float a, float  w, float h, float f):TuioContainer(si, xp, yp) {
			blob_id = bi;
			angle = a;
			width = w;
			height = h; 
			area = f;
			rotation_speed = 0.0f;
			rotation_accel = 0.0f;
		};
		
		/**
		 * This constructor takes the atttibutes of the provided TuioBlob 
		 * and assigs these values to the newly created TuioBlob.
		 *
		 * @param	tblb	the TuioBlob to assign
		 */
		TuioBlob (TuioBlob *tblb):TuioContainer(tblb) {
			blob_id = tblb->getBlobID();
			angle = tblb->getAngle();
			width = tblb->getWidth();
			height = tblb->getHeight();
			area = tblb->getArea();
			rotation_speed = 0.0f;
			rotation_accel = 0.0f;
		};
		
		/**
		 * The destructor is doing nothing in particular. 
		 */
		~TuioBlob() {};

		/**
		 * Returns the Blob ID of this TuioBlob.
		 * @return	the Blob ID of this TuioBlob
		 */
		int getBlobID() const{
			return blob_id;
		};
		
		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate, angle, X and Y velocity, motion acceleration,
		 * rotation speed and rotation acceleration to the private TuioBlob attributes.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the rotation angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	rs	the rotation velocity to assign
		 * @param	ma	the motion acceleration to assign
		 * @param	ra	the rotation acceleration to assign
		 */
		void update (TuioTime ttime, float xp, float yp, float a, float w, float h, float f, float xs, float ys, float rs, float ma, float ra) {
			TuioContainer::update(ttime,xp,yp,xs,ys,ma);
			angle = a;
			width = w;
			height = h;
			area = f;
			rotation_speed = rs;
			rotation_accel = ra;
			if ((rotation_accel!=0) && (state==TUIO_STOPPED)) state = TUIO_ROTATING;
		};

		/**
		 * Assigns the provided X and Y coordinate, angle, X and Y velocity, motion acceleration
		 * rotation velocity and rotation acceleration to the private TuioContainer attributes.
		 * The TuioTime time stamp remains unchanged.
		 *
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	rs	the rotation velocity to assign
		 * @param	ma	the motion acceleration to assign
		 * @param	ra	the rotation acceleration to assign
		 */
		void update (float xp, float yp, float a, float w, float h, float f, float xs, float ys, float rs, float ma, float ra) {
			TuioContainer::update(xp,yp,xs,ys,ma);
			angle = a;
			width = w;
			height = h;
			area = f;
			rotation_speed = rs;
			rotation_accel = ra;
			if ((rotation_accel!=0) && (state==TUIO_STOPPED)) state = TUIO_ROTATING;
		};
		
		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate and angle to the private TuioBlob attributes.
		 * The speed and accleration values are calculated accordingly.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle coordinate to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 */
		void update (TuioTime ttime, float xp, float yp, float a, float w, float h, float f) {
			TuioPoint lastPoint = path.back();
			TuioContainer::update(ttime,xp,yp);
			
			TuioTime diffTime = currentTime - lastPoint.getTuioTime();
			float dt = diffTime.getTotalMilliseconds()/1000.0f;
			float last_angle = angle;
			float last_rotation_speed = rotation_speed;
			angle = a;
			
			double da = (angle-last_angle)/(2*M_PI);
			if (da > 0.75f) da-=1.0f;
			else if (da < -0.75f) da+=1.0f;

			width = w;
			height = h;
			area = f;
			
			rotation_speed = (float)da/dt;
			rotation_accel =  (rotation_speed - last_rotation_speed)/dt;
		
			if ((rotation_accel!=0) && (state==TUIO_STOPPED)) state = TUIO_ROTATING;
		};

		/**
		 * This method is used to calculate the speed and acceleration values of a
		 * TuioBlob with unchanged position and angle.
		 */
		void stop (TuioTime ttime) {
			update(ttime,xpos,ypos,width,height,angle);
		};
		
		/**
		 * Takes the atttibutes of the provided TuioBlob 
		 * and assigs these values to this TuioBlob.
		 * The TuioTime time stamp of this TuioContainer remains unchanged.
		 *
		 * @param	tblb	the TuioContainer to assign
		 */	
		void update (TuioBlob *tblb) {
			TuioContainer::update(tblb);
			angle = tblb->getAngle();
			width = tblb->getWidth();
			height = tblb->getHeight();
			area = tblb->getArea();
			rotation_speed = tblb->getRotationSpeed();
			rotation_accel = tblb->getRotationAccel();
			if ((rotation_accel!=0) && (state==TUIO_STOPPED)) state = TUIO_ROTATING;
		};
		
		/**
		 * Returns the width of this TuioBlob.
		 * @return	the width of this TuioBlob
		 */
		float getWidth() const{ 
			return width;
		};

		/**
		 * Returns the height of this TuioBlob.
		 * @return	the height of this TuioBlob
		 */
		float getHeight() const{ 
			return height;
		};

		/**
		 * Returns the width of this TuioBlob.
		 * @return	the width of this TuioBlob
		 */
		int getScreenWidth(int w) const{ 
			return (int)(w*width);
		};
		
		/**
		 * Returns the height of this TuioBlob.
		 * @return	the height of this TuioBlob
		 */
		int getScreenHeight(int h) const{ 
			return (int)(h*height);
		};
		
		/**
		 * Returns the area of this TuioBlob.
		 * @return	the area of this TuioBlob
		 */
		float getArea() const{ 
			return area;
		};
		
		/**
		 * Returns the rotation angle of this TuioBlob.
		 * @return	the rotation angle of this TuioBlob
		 */
		float getAngle() const{
			return angle;
		};
		
		/**
		 * Returns the rotation angle in degrees of this TuioBlob.
		 * @return	the rotation angle in degrees of this TuioBlob
		 */
		float getAngleDegrees() const{ 
			return (float)(angle/M_PI*180);
		};
		
		/**
		 * Returns the rotation speed of this TuioBlob.
		 * @return	the rotation speed of this TuioBlob
		 */
		float getRotationSpeed() const{ 
			return rotation_speed;
		};
		
		/**
		 * Returns the rotation acceleration of this TuioBlob.
		 * @return	the rotation acceleration of this TuioBlob
		 */
		float getRotationAccel() const{
			return rotation_accel;
		};

		/**
		 * Returns true of this TuioBlob is moving.
		 * @return	true of this TuioBlob is moving
		 */
		virtual bool isMoving() const{ 
			if ((state==TUIO_ACCELERATING) || (state==TUIO_DECELERATING) || (state==TUIO_ROTATING)) return true;
			else return false;
		};
	};
};
#endif
