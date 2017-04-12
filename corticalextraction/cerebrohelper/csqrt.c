/* Copyright 2000-2002 Roger P. Woods, M.D. */
/* Modified 7/19/02 */

/*
 *
 * This routine computes c=sqrt(a) where a and c are complex
 *									
 */

#include "AIR.h"

void AIR_csqrt(const double ar, const double ai, double *cr, double *ci)

{
	double w;
	
	if(ar==0.0 && ai==0.0) w=0.0;
	else if(fabs(ar)>=fabs(ai)){
		w=ai/ar;
		w=w*w;
		w+=1.0;
		w=sqrt(w);
		w+=1.0;
		w/=2.0;
		w=sqrt(w);
		w*=sqrt(fabs(ar));
	}
	else{
		w=ar/ai;
		w=w*w;
		w+=1.0;
		w=sqrt(w);
		w+=fabs(ar/ai);
		w/=2.0;
		w=sqrt(w);
		w*=sqrt(fabs(ai));
	}
	if(w==0.0){
		*cr=0.0;
		*ci=0.0;
		return;
	}
	if(ar>=0.0){
		*cr=w;
		*ci=ai/(2.0*w);
		return;
	}
	if(ai>=0.0){
		*cr=ai/(2.0*w);
		*ci=w;
		return;
	}
	*cr=fabs(ai)/(2.0*w);
	*ci=-w;
	return;
}
