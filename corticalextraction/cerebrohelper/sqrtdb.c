/* Copyright 2003 Roger P. Woods, M.D. */
/* Modified 2/15/03 */

/*
 * This implements algorithm 5.1 of Cheng, et al, Approximating the logarithm of a matrix to
 * specfied accuracy, SIAM J. Matrix Anal. Appl. 22(4):1112-1125.
 *
 * It computes the square root of a real matrix to a specified accuracy using Denman-Beavers
 * iterations
 *
 * On input:
 * 
 * 	N is the dimension of the matrix A
 * 	A is the input matrix
 * 	WRK must be allocated using wrk=AIR_matrix3(>=N,>=N,>=3);
 * 	EPSILON is the required accuracy
 *
 * Returns:
 *	TRUE if the specified accuracy was achieved
 *	FALSE otherwise
 *
 * On return:
 *
 * 	A contains the square root of the input A, computed to the required accuracy
 *	*ITER is the number of iterations performed
 */

#include "AIR.h"
#define MAXIMUM_ITERATIONS 50
#define MAXIMUM_MATRIX_DIMENSION 4

AIR_Boolean AIR_sqrtdb(const unsigned int n, double **a, double ***wrk, double epsilon, unsigned int *itk)
{	
	// Copy a into wrk0 and wrk2
	{
		double **wrk0j=wrk[0];
		double **wrk2j=wrk[2];
		double **aj=a;
		
		unsigned int j;
		for(j=0;j<n;j++){
			
			double *wrk0ji=*wrk0j++;
			double *wrk2ji=*wrk2j++;
			double *aji=*aj++;
			
			unsigned int i;
			for(i=0;i<n;i++){
			
				*wrk0ji++=*aji;
				*wrk2ji++=*aji++;
			}
		}
	}
	{
		unsigned int iters=0;
		for(;;){
		
			unsigned int ipvt[MAXIMUM_MATRIX_DIMENSION];
		
			iters++;
			
			// Compute gamma_k
			if(AIR_dgefa(wrk[0],n,ipvt)!=n){
				*itk=iters;
				return FALSE;
			}
			{
				double gamma=AIR_ddet(wrk[0],n,ipvt);
				gamma=pow(fabs(gamma),-1.0/(2.0*n));
				
				// compute wrk0=inverse of M
				{
					double work[MAXIMUM_MATRIX_DIMENSION];
					
					AIR_dgedi(wrk[0],n,ipvt,work);
				}
				
				// compute a=Y
				{
					double **wrk1j=wrk[1];
					double **aj=a;
					
					unsigned int j;
					for(j=0;j<n;j++){
					
						double *wrk1ji=*wrk1j++;
						double *aji=*aj++;
						
						unsigned int i;
						for(i=0;i<n;i++){
						
							*wrk1ji++=*aji++;
						}
					}
				}
				AIR_dgemm('n','n',n ,n, n, 1.0/(2.0*gamma), wrk[1], wrk[0], (gamma/2.0), a);
				
				// Compute wrk0=M-I for convergence test
				{
					double value=4.0*gamma*gamma;
					{
						double **wrk0j=wrk[0];
						
						unsigned int j;
						for(j=0;j<n;j++){
						
							double *wrk0ji=*wrk0j++;
							
							unsigned int i;
							for(i=0;i<n;i++){
							
								*wrk0ji++/=value;
							}
						}
					}
				}
				{
					double value=gamma*gamma/4.0;
					{
						unsigned int j;
						for(j=0;j<n;j++){
							AIR_daxpy(n,value,wrk[2][j],wrk[0][j]);
							wrk[0][j][j]-=.5;
						}
					}
				}
			}
			{
				double norm_w=0.0;
				// Compute the 1-norm of wrk0=M-I
				{
					double *wrk0=wrk[0][0];
					
					unsigned int j;
					for(j=0;j<n;j++){
						double sum=0.0;
						unsigned int i;
						for(i=0;i<n;i++){
							sum+=fabs(*wrk0++);
						}
						if(sum>norm_w) norm_w=sum;
					}
				}
	
				// Set wrk0=M
				{
					unsigned int j;
					for(j=0;j<n;j++){
						wrk[0][j][j]+=1.0;
					}
				}

				// Check convergence criteria of equation 5.7, p. 1119
				if(fabs(norm_w+log(1.0-norm_w))<=epsilon){
					*itk=iters;
					return TRUE;
				}
				
				// Check for maximum iterations
				if(iters>=MAXIMUM_ITERATIONS){
					*itk=iters;
					return FALSE;
				}
	
				// Copy wrk[0] to wrk[2]
				{
					double **wrk2j=wrk[2];
					double **wrk0j=wrk[0];
					
					unsigned int j;
					for(j=0;j<n;j++){
						
						double *wrk2ji=*wrk2j++;
						double *wrk0ji=*wrk0j++;
						
						unsigned int i;
						for(i=0;i<n;i++){
						
							*wrk2ji++=*wrk0ji++;
						}
					}
				}
			}
		}
	}
}
