/* Copyright 1995-2004 Roger P. Woods, M.D. */
/* Modified 12/18/04 */


/* AIR_Pixels interp_nn_3D()
 *
 * returns the interpolated value
 */

#include "AIR.h"

AIR_Pixels AIR_interp_nn_3D(/*@unused@*/ const struct AIR_Key_info *stats, AIR_Pixels ***volume, const double x_p, const double y_p, const double z_p, const double scale, /*@unused@*/ const unsigned int *window, /*@unused@*/ const double pi)

{
	/* Find interpolated value */
	unsigned int x_up=(x_p+.5);
	unsigned int y_up=(y_p+.5);
	unsigned int z_up=(z_p+.5);

	{
		double total;

		total=(double)volume[z_up][y_up][x_up];

		total*=scale;
		total+=.5;
		if(total>(double)AIR_CONFIG_MAX_POSS_VALUE){
			return AIR_CONFIG_MAX_POSS_VALUE;
		}
		else return (AIR_Pixels)total;
	}
}
