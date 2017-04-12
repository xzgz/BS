/* Copyright 1995-2004 Roger P. Woods, M.D. */
/* Modified 12/16/04 */


/*
 * This routine computes the first and second derivatives
 *  of the normalized standard deviation with respect to all
 *  spatial transformation parameters.
 *
 * Returns the normalized standard deviation			
 */

#include "AIR.h"

#define PARAM_INT 8
#define PARAM_INT_H PARAM_INT*(PARAM_INT+1)/2

#define BLOCK0 0	/* x domain */
#define BLOCK1 3	/* y domain */
#define BLOCK2 6	/* perspective domain */
#define BLOCK3 8	/* end of perspective domain */

double AIR_qvderivsN8(const unsigned int spatial_parameters, double **es, double ***des, double ****ees, const unsigned int samplefactor, AIR_Pixels ***pixel2, const struct AIR_Key_info *stats2, AIR_Pixels ***pixel5, const struct AIR_Key_info *stats5, const AIR_Pixels threshold5, double *dcff, double **ecff, unsigned int *count, double *mean, double *square, double **dmean, double **dsquare, /*@unused@*/ double ***emean, double ***esquare, const unsigned int partitions, AIR_Pixels max_actual_value, /*@unused@*/ double scale, /*@unused@*/ const AIR_Boolean forward, unsigned int *error)

{

	double 		cf=0.0;

	double		dcf[PARAM_INT];
	double 		ecfitems[PARAM_INT_H];
	
	double 		dxy[BLOCK1];
	
	double		dx[BLOCK3-BLOCK2];
	double		dy[BLOCK3-BLOCK2];
	
	double		exyitems[(BLOCK3-BLOCK2)*BLOCK1];
	
	double		exitems[(BLOCK3-BLOCK2)*(BLOCK3-BLOCK2+1)/2];
	double		eyitems[(BLOCK3-BLOCK2)*(BLOCK3-BLOCK2+1)/2];

	unsigned int x_max1u=stats2->x_dim-1;
	unsigned int y_max1u=stats2->y_dim-1;

	double x_max1=x_max1u;
	double y_max1=y_max1u;

	unsigned int x_dim2=stats5->x_dim;
	unsigned int y_dim2=stats5->y_dim;
	unsigned int z_dim2=stats5->z_dim;

	double		e00,e01,e03,e10,e11,e13,e30,e31;

	if(partitions-1>(unsigned int)max_actual_value && partitions-1<(unsigned int)AIR_CONFIG_MAX_POSS_VALUE) max_actual_value=partitions-1;

	/*Initialize values*/
	{
		unsigned int *countjj=count;
		double *meanjj=mean;
		double *squarejj=square;
		
		double **dmeanjj=dmean;
		double **dsquarejj=dsquare;
		
		double ***emeanjj=emean;
		double ***esquarejj=esquare;
		
		unsigned int jj;
		
		for(jj=0; jj<partitions; jj++, countjj++, meanjj++, squarejj++, dmeanjj++, dsquarejj++, emeanjj++, esquarejj++){
		
			*countjj=0;
			*meanjj=*squarejj=0.0;
			{
				double *dmeanjjt=*dmeanjj;
				double *dsquarejjt=*dsquarejj;
				
				double *emeanjjts=**emeanjj;
				double *esquarejjts=**esquarejj;
				
				unsigned int t;
			
				for(t=0; t<PARAM_INT; t++, dmeanjjt++, dsquarejjt++){
				
					*dmeanjjt=0.0;
					*dsquarejjt=0.0;
					
					{
						unsigned int s;
						
						for(s=0; s<=t; s++, emeanjjts++, esquarejjts++){
						
							*emeanjjts=0.0;
							*esquarejjts=0.0;
						}
					}
				}
			}
		}		
	}

	e00=es[0][0];
	e01=es[0][1];
	e03=es[0][3];
	e10=es[1][0];
	e11=es[1][1];
	e13=es[1][3];
	e30=es[3][0];
	e31=es[3][1];

	/*Examine pixels of standard file at samplefactor interval*/
	/*Note that it is assumed here that pixel5[z_dim][y_dim][x_dim] refers to the*/
	/* same pixel as *(pixel5[0][0]+z_dim*y_dim*x_dim), i.e. that all the pixels */
	/* are represented in a contiguous block of memory--see the routine*/
	/* "create_volume.c" for an illustration of how this is assured*/

	/*ARRAY STRUCTURE ASSUMPTIONS MADE HERE*/
	{
		unsigned long int r;
		unsigned long int r_term=x_dim2*y_dim2*z_dim2;

		AIR_Pixels *rr;
		
		for (r=0, rr=**pixel5; r<r_term; r+=samplefactor, rr+=samplefactor){

			/*Verify that pixel5>threshold*/
			if(*rr<=threshold5) continue;
			{
				signed int n0,n1,n2,n3;
				double a,b,d,e;
				double pix3=(double)*rr;
	
				#include "findcoords8.cf"
				
				{
					double ratio, dratio[PARAM_INT];
					{
						double pix4, dxpix4, dypix4;
						
						/* The variable pix1 is the partition of the current standard pixel */
						/* Note that if partitions==1, all pixels are in the same partition */
	
						unsigned int pix1=(partitions-1)*(unsigned int)pix3/(unsigned int)max_actual_value;
						{
							/*Calculate the trilinear interpolated voxel value*/
	
							pix4=n0*d*e+n1*a*e+n2*d*b+n3*a*b;
	
	
							/*Some intermediate values needed to calculate derivatives efficiently*/
	
							dxpix4=(e*(n1-n0)+b*(n3-n2));
							dypix4=(d*(n2-n0)+a*(n3-n1));
						}
	
						/* Calculate values needed to compute standard deviation */
						ratio=pix4/pix3;
						mean[pix1]+=ratio;
						square[pix1]+=ratio*ratio;
						count[pix1]++;
	
	
						/*Calculate derivatives that are nonzero*/
						
						/* First derivatives */
						{
							double *dratiot2;
							double *dmeanpix1t2;
							double *dsquarepix1t2;
							
							{
								unsigned int t;
								double *dxyt=dxy;
								
								double *dratiot1=dratio;
								
								double *dmeanpix1t1=dmean[pix1];
								
								double *dsquarepix1t1=dsquare[pix1];
								
								dratiot2=dratio+BLOCK1;
								dmeanpix1t2=dmean[pix1]+BLOCK1;
								dsquarepix1t2=dsquare[pix1]+BLOCK1;
								
								for(t=BLOCK0; t<BLOCK1; t++, dxyt++, dratiot1++, dratiot2++, dmeanpix1t1++, dmeanpix1t2++, dsquarepix1t1++, dsquarepix1t2++){
	
									/* dratio[t]=dxpix4*dx[t]/pix3; */
									*dratiot1=dxpix4**dxyt/pix3;
									
									/* dmean[pix1][t]+=dratio[t]; */
									*dmeanpix1t1+=*dratiot1;
									
									/* dsquare[pix1][t]+=2.0*ratio*dratio[t]*/
									*dsquarepix1t1+=2.0*ratio**dratiot1;
									
									/* dratio[t+BLOCK1]=dypix4*dy[t+BLOCK1]/pix3; */
									*dratiot2=dypix4**dxyt/pix3;
									
									/* dmean[pix1][t+BLOCK1]+=dratio[t+BLOCK1]; */
									*dmeanpix1t2+=*dratiot2;
									
									/* dsquare[pix1][t]+=2.0*ratio*dratio[t]*/
									*dsquarepix1t2+=2.0*ratio**dratiot2;
								}
							}
							{
								unsigned int t;
								double *dxt=dx;
								double *dyt=dy;
								
								for(t=BLOCK2; t<BLOCK3; t++, dratiot2++, dmeanpix1t2++, dsquarepix1t2++, dxt++, dyt++){
								
									/* dratio[t]=(dxpix4*dx[t]+dypix4*dy[t])/pix3; */
									*dratiot2=(dxpix4**dxt+dypix4**dyt)/pix3;
									
									/* dmean[pix1][t]+=dratio[t]; */
									*dmeanpix1t2+=*dratiot2;
									
									/* dmean[pix1][t]+=2.0*ratio*dratio[t]; */
									*dsquarepix1t2+=2.0*ratio**dratiot2;
								}
							}
						}
						
						/* Second derivatives */
						{
							double *esquarepix1ts=*esquare[pix1];
							double *dratiot=dratio;
							unsigned int t;
								
							for(t=BLOCK0;t<BLOCK3;t++, dratiot++){
								
								unsigned int s;
								double *dratios=dratio;
	
								for(s=BLOCK0; s<=t; s++, esquarepix1ts++, dratios++){
								
									/* esquare[pix1][t][s]+=2.0*(dratio[s]*dratio[t]) */
									*esquarepix1ts+=2.0*(*dratios**dratiot);
								}
							}
						}
					}
				}
			}
		}
	}
	
	#include "qnorm_std_dev.cf"
	
	{
		double **ep[PARAM_INT];
		double *dp[PARAM_INT];
		
		dp[0]=des[0][0];
		dp[1]=des[1][0];
		dp[2]=des[3][0];
		dp[3]=des[0][1];
		dp[4]=des[1][1];
		dp[5]=des[3][1];
		dp[6]=des[0][3];
		dp[7]=des[1][3];
		
		ep[0]=ees[0][0];
		ep[1]=ees[1][0];
		ep[2]=ees[3][0];
		ep[3]=ees[0][1];
		ep[4]=ees[1][1];
		ep[5]=ees[3][1];
		ep[6]=ees[0][3];
		ep[7]=ees[1][3];
		
		#include "param_chain_rule.cf"

	}
	return cf;
}
