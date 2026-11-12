/*  reacTIVision tangible interaction framework
	Copyright (c) 2005-2026 Martin Kaltenbrunner <martin@tuio.org>
	Based on an example by Nicolas Roussel <nicolas.roussel@inria.fr>
 
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

#ifndef ONEEUROFILTER
#define ONEEUROFILTER

#define _USE_MATH_DEFINES
#include <stdexcept>
#include <cmath>

typedef double TimeStamp; // in seconds
static const TimeStamp UndefinedTime = -1.0;

namespace TUIO {
	
	/**
	 * An adaptive low-pass filter as described by
	 * Casiez, Roussel and Vogel, "1€ filter: A simple speed-based
	 * low-pass filter for noisy input in interactive systems".
	 *
	 * The cutoff frequency adapts to the rate of change of the signal,
	 * which reduces jitter during slow movements while keeping
	 * responsiveness during fast movements.
	 */
	class OneEuroFilter {
		
		/**
		 * A simple first-order low-pass filter used internally by
		 * the OneEuroFilter. The filter coefficient alpha is computed
		 * from the desired cutoff frequency and the sampling rate.
		 */
		class LowPassFilter {
			
		public:
			
			/**
			 * Whether the filter has processed at least one sample.
			 * Until then, the first input value is passed through unchanged.
			 */
			bool initialized;
			
			/** The most recent raw input value. */
			double lastRawValue;
			
			/** The most recent filtered output value. */
			double lastResult;
	
			/**
			 * Creates a new LowPassFilter.
			 *
			 * @param initval  the initial filter value (default 0.0)
			 */
			LowPassFilter(double initval=0.0) {
				
				lastRawValue = lastResult = initval;
				initialized = false;
			}
			
			/**
			 * Filters a new sample.
			 *
			 * @param value  the new raw input value
			 * @param alpha  the filter coefficient between 0.0 and 1.0
			 * @return the filtered output value
			 */
			double filter(double value, double alpha);
		};
		
		double freq;
		double mincutoff;
		double beta;
		double dcutoff;
		LowPassFilter *x;
		LowPassFilter *dx;
		TimeStamp lasttime;
		
		double alpha(double cutoff);
		
	public:
		
		/**
		 * Creates a new OneEuroFilter.
		 *
		 * @param f   the sampling frequency in Hz
		 * @param mc  the minimum cutoff frequency in Hz (default 1.0)
		 * @param b   the speed coefficient (default 0.0)
		 * @param dc  the cutoff frequency for the derivative filter in Hz (default 1.0)
		 * @throws std::range_error if any parameter is not > 0
		 */
		OneEuroFilter(double f, double mc=1.0, double b=0.0, double dc=1.0) {
			
			if (f<=0) throw std::range_error("freq should be >0");
			else freq = f;
			if (mc<=0) throw std::range_error("mincutoff should be >0");
			else mincutoff = mc;
			if (b<=0) throw std::range_error("beta should be >0");
			else beta = b;
			if (dc<=0) throw std::range_error("dcutoff should be >0");
			else dcutoff = dc;
			
			x = new LowPassFilter(alpha(mincutoff));
			dx = new LowPassFilter(alpha(dcutoff));
			lasttime = UndefinedTime;
		}
		
		/** Copy constructor */
		OneEuroFilter(const OneEuroFilter &f) {
			freq = f.freq;
			mincutoff = f.mincutoff;
			beta = f.beta;
			dcutoff = f.dcutoff;
			x = new LowPassFilter(*(f.x));
			dx = new LowPassFilter(*(f.dx));
			lasttime = f.lasttime;
		}
		
		/** Assignment operator */
		OneEuroFilter& operator=(const OneEuroFilter &f) {
			if (this!=&f) {
				freq = f.freq;
				mincutoff = f.mincutoff;
				beta = f.beta;
				dcutoff = f.dcutoff;
				*x = *(f.x);
				*dx = *(f.dx);
				lasttime = f.lasttime;
			}
			return *this;
		}
		
		/** Destructor */
		~OneEuroFilter(void) {
			delete x;
			delete dx;
		}
		
		/**
		 * Filters a new sample.
		 *
		 * @param value  the new raw input value
		 * @param dt     the time since the previous sample in seconds,
		 *               or UndefinedTime to use the configured sampling frequency
		 * @return the filtered output value
		 */
		double filter(double value, TimeStamp dt=UndefinedTime);
		
	};
	
}

#endif
