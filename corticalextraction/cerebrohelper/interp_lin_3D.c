/* Copyright 1995-2004 Roger P. Woods, M.D. */
/* Modified 12/18/04 */


/* AIR_Pixels interp_lin_3D()
 *
 * Computes the interpolated value
 *
 */

#include "AIR.h"

AIR_Pixels AIR_interp_lin_3D(/*@unused@*/ const struct AIR_Key_info *stats, AIR_Pixels ***volume, const double x_p, const double y_p, const double z_p, const double scale, /*@unused@*/ const unsigned int *window, /*@unused@*/ const double pi)

{
	/* Find interpolated value */	
	unsigned int x_down=x_p;
	unsigned int y_down=y_p;
	unsigned int z_down=z_p;
	
	double a=x_p-x_down;
	double b=y_p-y_down;
	double c=z_p-z_down;
	
	double d=1.0-a;
	double e=1.0-b;
	double f=1.0-c;

	/*Interpolate */
	{
		AIR_Pixels *pi2, **pj2, ***pk2;
		double total;

		pk2=volume+z_down;
		pj2=*pk2+y_down;
		pi2=*pj2+x_down;
		total=*pi2*d*e*f;

		if (a!=0.0){
			pi2++;
			total+=*pi2*a*e*f;
		}

		if (b!=0.0){
			pj2++;
			pi2=*pj2+x_down;
			total+=*pi2*d*b*f;

			if (a!=0.0){
				pi2++;
				total+=*pi2*a*b*f;
			}
		}

		if (c!=0.0){
			pk2++;
			pj2=*pk2+y_down;
			pi2=*pj2+x_down;
			total+=*pi2*d*e*c;

			if (a!=0.0){
				pi2++;
				total+=*pi2*a*e*c;
			}

			if (b!=0.0){
				pj2++;
				pi2=*pj2+x_down;
				total+=*pi2*d*b*c;

				if (a!=0.0){
					pi2++;
					total+=*pi2*a*b*c;
				}
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
