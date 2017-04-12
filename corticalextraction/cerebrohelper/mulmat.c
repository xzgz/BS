/* Copyright 1997-2001 Roger P. Woods, M.D. */
/* Modified 4/20/02 */

/* void mulmat()
 *
 * Multiplies matrices: c=a*b
 *  	where a is m by n
 *	and
 *	where b is n by r
 *
 * 	so that c is m by r
 */
 
 /* IMPORTANT NOTE
  *
  * This function assumes that the matrices have been allocated by AIR_matrix2
  * and that the dimensions of the matrices listed here correspond to the 
  * dimensions used at allocation time
  *
  * Specific assumptions are made about the structure of the array and if these are
  * not valid, matrix multiplication will give inaccurate results
  */

#include "AIR.h"

void AIR_mulmat(double **a, const unsigned int m, const unsigned int n, double **b, const unsigned int r, double **c)

{
	unsigned int y;
	double *out;
	double **btemp;
	
	for(y=0,out=*c,btemp=b;y<r;y++,btemp++){
		
		unsigned int x;
		
		for(x=0;x<m;x++,out++){
			*out=0.0;
			{
				unsigned int i;
				double *ina;
				double *inb;
				
				for(i=0,ina=(*a)+x,inb=*btemp;i<n;i++,ina+=m,inb++){
					/* c[y][x]=a[i][x]*b[y][i]; */
					*out+=*ina**inb;
				}
			}
		}
	}
}
