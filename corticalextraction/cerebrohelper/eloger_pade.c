/* Copyright 2003 Roger P. Woods, M.D. */
/* Modified 2/15/03 */

/*
 * Computes the logarithm of the real matrix a to the DBL_EPSILON tolerance using the
 * transformation free scaling and squaring algorithm described by Cheng, et al.
 * Approximating the logarithm of a matrix to specified accuracy. SIAM J. Matrix. Anal. Appl.
 * 2001;22(4):1112-1125.
 *
 * On input:
 *
 * 	N is the dimension of A
 * 	A is the input matrix
 * 	WRK must be allocated using wrk=AIR_matrix3(>=N,>=N,>=4);
 *
 * Returns:
 *	0 if no error occurred, otherwise returns error code
 *
 * On return:
 * 	
 * 	A contains the logarithm of the value of A on input unless an error occurred
 */

#include "AIR.h"
#include <float.h>
#define MAXIT 50

AIR_Error AIR_eloger_pade(const unsigned int n, double **a, double ***wrk, AIR_Boolean is_affine)
{
	{
		double delta=DBL_EPSILON/4.0;
		double scale=0.5;
		//const unsigned int nn=n*n;
		unsigned int mk=0;
				
		// Initialization of M
		{
			double **wrk3j=wrk[3];
			
			unsigned int j;
			for(j=0;j<n;j++){
			
				double *wrk3ji=*wrk3j++;
				
				unsigned int i;
				for(i=0;i<n;i++){
				
					*wrk3ji++=0.0;
				}
			}
		}
		// Step 1 of Algorithm 7.1 from page 1122
		{
			unsigned int k;
			for(k=1;k<=MAXIT+1;k++){
				if(k==MAXIT+1){
					// Too many square roots
					return AIR_ELOGER_CONVERGE_ERROR;
				}
				// Set a=Y-I
				{
					unsigned int j;
					
					for(j=0;j<n;j++){
						a[j][j]-=1.0;
					}
				}
				{
					unsigned int itk=0;
					{
						double norm_i_minus_y=0.0;
						{
							unsigned int j;
							for(j=0;j<n;j++){
								double sum=0.0;
								unsigned int i;
								for(i=0;i<n;i++){
									sum+=fabs(a[j][i]);
								}
								if(sum>norm_i_minus_y) norm_i_minus_y=sum;
							}
						}
						// Set a=Y
						{
							unsigned int j;
							
							for(j=0;j<n;j++){
								a[j][j]+=1.0;
							}
						}
			
						// Test whether criteria of step 1 of Algorithm 7.1 have been met
						if(norm_i_minus_y<.99){
						
							// Find mk based on equation (7.1), page 1121
							{
								double epsilon=delta*pow(2.0,1.0-k);
								epsilon*=(1.0-pow(2.0,0.0-k));
								if(DBL_EPSILON>epsilon) epsilon=DBL_EPSILON;
								
								mk=AIR_padeorder(norm_i_minus_y,epsilon);
								if(mk==0) return AIR_ELOGER_CONVERGE_ERROR;
							}
							{
								// Determine whether to iterate further for computational efficiency
								double epsilon=delta*pow(2.0,0.0-k);
								epsilon*=(1.0-pow(2.0,-1.0-k));
								if(DBL_EPSILON>epsilon) epsilon=DBL_EPSILON;
								{
									unsigned int mkp1=AIR_padeorder(norm_i_minus_y/2.0,epsilon);
									if(mkp1==0) break;	// Can't evaluate further iterations
										
									if(mk<=2*itk+mkp1){
										break;	// Convergence criteria of equation (7.6), p. 1122
									}
								}
							}
						}
					}		 
					scale*=2.0;
					{
						// Iterate square roots until equation (5.7) is satisfied
						double epsilon=delta/pow(4.0,-1.0+k);
						
						if(!AIR_sqrtdb(n,a,wrk,epsilon,&itk)){
							return AIR_ELOGER_CONVERGE_ERROR;
						}
					}
				}
				// Cumulate scale*(M-I) in wrk3
				{
					unsigned int j;
					
					for(j=0;j<n;j++){
						AIR_daxpy(n,scale,wrk[0][j],wrk[3][j]);
					}
				}
				{
					unsigned int j;
					
					for(j=0;j<n;j++){
						wrk[3][j][j]-=scale;
					}
				}
			}
		}
		// Step 2 of Algorithm 7.1 from page 1122
		{
			AIR_Error errcode=AIR_logpade(n,mk,a,wrk);
			if(errcode!=0) return errcode;
		}
	
		// Step 3 of Algorithm 7.1, using values cumulated in wrk3 in Step 1
		scale*=2.0;
		{
			double **aj=a;
			double **wrk3j=wrk[3];
			
			unsigned int j;
			for(j=0;j<n;j++){
			
				double *aji=*aj++;
				double *wrk3ji=*wrk3j++;
				
				unsigned int i;
				for(i=0;i<n;i++){
				
					*aji*=scale;
					*aji++-=*wrk3ji++;
				}
			}
		}
		if(is_affine) a[n-1][n-1]=0.0;
	}
	return 0;
}
