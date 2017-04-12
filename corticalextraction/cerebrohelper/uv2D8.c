/* Copyright 1995-2011 Roger P. Woods, M.D. */
/* Modified: 3/13/11 */

/*
 * void uv2D8()
 *
 * will calculate first and second derivatives with
 * respect to the paramaters for a full 8
 * parameter 2D model.
 *
 * Note that a full 4 x 4 matrix is assumed.		
 */


#include "AIR.h"

#define PARAMETERS 8

void AIR_uv2D8(const double *i, double **ef, double ***def, double ****eef, /*@unused@*/ const struct AIR_Key_info *reslice_info, /*@unused@*/ const struct AIR_Key_info *standard_info, /*@unused@*/ const AIR_Boolean zooming)

{
	/*Define forward transform*/
	
	const double *ii=i;

	ef[0][0]=*ii++;
	ef[1][0]=*ii++;
	ef[2][0]=0.0;
	ef[3][0]=*ii++;
	ef[0][1]=*ii++;
	ef[1][1]=*ii++;
	ef[2][1]=0.0;
	ef[3][1]=*ii++;
	ef[0][2]=0.0;
	ef[1][2]=0.0;
	ef[2][2]=1.0;
	ef[3][2]=0.0;
	ef[0][3]=*ii++;
	ef[1][3]=*ii;
	ef[2][3]=0.0;
	ef[3][3]=1.0;		/*9th parameter is redundant*/


	/*All second partial derivatives are zero for forward transform*/
	{
		unsigned int m;
		double *defmno=**def;
		double *eefmnop=***eef;
		
		for (m=0;m<4;m++){
		
			unsigned int n;
			
			for (n=0;n<4;n++){
			
				unsigned int o;
				
				for (o=0;o<PARAMETERS;o++, defmno++){
				
					*defmno=0.0;
					{
						unsigned int p;
						
						for (p=0;p<=o;p++, eefmnop++){
						
							*eefmnop=0.0;
						}
					}
				}
			}
		}
	}
	
	/*First partial derivatives are zero except where the parameter*/
	/* is the factor being differentiated in which case it is one*/
	def[0][0][0]=1.0;
	def[1][0][1]=1.0;
	def[3][0][2]=1.0;
	def[0][1][3]=1.0;
	def[1][1][4]=1.0;
	def[3][1][5]=1.0;
	def[0][3][6]=1.0;
	def[1][3][7]=1.0;

}

	
