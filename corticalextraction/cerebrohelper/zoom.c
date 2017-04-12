/* Copyright 1995-2002 Roger P. Woods, M.D. */
/* Modified: 7/20/02 */

/*
 * Routine for linearly interpolating a 3-dimensional data set into
 *  cubic voxels.
 *
 * Returns:
 *	pointer to interpolated volume if successful
 *	NULL pointer if unsuccessful or if volume==NULL
 *	error code in *errcode
 */

#include "AIR.h"
#include <float.h>

AIR_Pixels ***AIR_zoom(AIR_Pixels ***volume, const struct AIR_Key_info *stats, struct AIR_Key_info *new_stats, AIR_Error *errcode)

{
	double f[16]={
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	};

	new_stats->bits=stats->bits;
	{
		double pixel_size_s=stats->x_size;
		if(stats->y_size<pixel_size_s) pixel_size_s=stats->y_size;
		if(stats->z_size<pixel_size_s) pixel_size_s=stats->z_size;
	
		if(fabs(stats->x_size-pixel_size_s)>AIR_CONFIG_PIX_SIZE_ERR){
			new_stats->x_dim=(unsigned int)floor((stats->x_dim-1)*(stats->x_size/pixel_size_s)+1.0+DBL_MIN);
			new_stats->x_size=pixel_size_s;
			f[0]=pixel_size_s/stats->x_size;	/* e[0][0] */
		}
		else{
			new_stats->x_dim=stats->x_dim;
			new_stats->x_size=stats->x_size;
		}
	
		if(fabs(stats->y_size-pixel_size_s)>AIR_CONFIG_PIX_SIZE_ERR){
			new_stats->y_dim=(unsigned int)floor((stats->y_dim-1)*(stats->y_size/pixel_size_s)+1.0+DBL_MIN);
			new_stats->y_size=pixel_size_s;
			f[5]=pixel_size_s/stats->y_size;	/* e[1][1] */
		}
		else{
			new_stats->y_dim=stats->y_dim;
			new_stats->y_size=stats->y_size;
		}
	
		if(fabs(stats->z_size-pixel_size_s)>AIR_CONFIG_PIX_SIZE_ERR){
			new_stats->z_dim=(unsigned int)floor((stats->z_dim-1)*(stats->z_size/pixel_size_s)+1.0+DBL_MIN);
			new_stats->z_size=pixel_size_s;
			f[10]=pixel_size_s/stats->z_size;	/* e[2][2] */
		}
		else{
			new_stats->z_dim=stats->z_dim;
			new_stats->z_size=stats->z_size;
		}
	}
	if(!volume){
		*errcode=0;	/* This is not an error, just a quick way to get new_stats dimensioned */
		return NULL;
	}
	{
		double *e[4];
		
		e[0]=f;
		e[1]=f+4;
		e[2]=f+8;
		e[3]=f+12;
		
		return AIR_r_affine_lin(volume,stats,new_stats,e,1.0,errcode);
	}
}
