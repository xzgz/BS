/* Copyright 1995-2004 Roger P. Woods, M.D. */
/* Modified 12/18/04 */


/* AIR_Pixels interp_lin_2D()
 *
 * returns the interpolated value
 */

#include "AIR.h"

AIR_Pixels AIR_interp_lin_2D(/*@unused@*/ const struct AIR_Key_info *stats, AIR_Pixels ***volume, const double x_p, const double y_p, const unsigned int k, const double scale, /*@unused@*/ const unsigned int *window, /*@unused@*/ const double pi)

{
	/* Find interpolated value */
	unsigned int x_down=x_p;
	unsigned int y_down=y_p;
	
	double a=x_p-x_down;
	double b=y_p-y_down;
	
	double d=1.0-a;
	double e=1.0-b;

	/* Interpolate */
	{
		AIR_Pixels *pi2, **pj2, ***pk2;
		double total;
		
		pk2=volume+k;
		pj2=*pk2+y_down;
		pi2=*pj2+x_down;
		total=*pi2*d*e;

		if (a!=0.0){
			pi2++;
			total+=*pi2*a*e;
		}

		if (b!=0.0){
			pj2++;
			pi2=*pj2+x_down;
			total+=*pi2*d*b;

			if (a!=0.0){
				pi2++;
				total+=*pi2*a*b;
			}
		}

		total*=scale;
		total+=.5;
		if(total>(double)AIR_CONFIG_MAX_POSS_VALUE){
			return AIR_CONFIG_MAX_POSS_VALUE;
		}
		else return (AIR_Pixels)total;
	}
}
