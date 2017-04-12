/* Copyright 1995-2003 Roger P. Woods, M.D. */
/* Modified: 2/15/03 */

/*
 * This defines a new average standard space that is the
 *  average of the reslice spaces of a set of .air files that
 *  share a common standard space.
 * 
 * Routine iterates until absolute values of all means are less than TINY, then
 *  iterates until failure to improve further.
 *
 * Storage4 needs to be allocated matrix3(>=N,>=N,>=8)
 *
 * Returns:
 *	0 if successful
 *	errcode if unsuccessful
 *
 * On successful return, the mean matrix can be accessed in storage[2]					
 */

#include "AIR.h"
#include <float.h>
#define TINY 1.0e-10
#define MAXITERS 100

AIR_Error AIR_comloger(const unsigned int n, double ***en, const unsigned int count, double ***storage4, AIR_Boolean is_affine)

{
	double **e=*storage4++;
	double **mean=*storage4++;
	double **old_mean=*storage4++;
	double **log_sum=*storage4++;

	/*Do the averaging*/
	{
		double **old_meanj=old_mean;
		double **meanj=mean;
		double **en0j=en[0];
		
		unsigned int j;

		for(j=0;j<n;j++){
		
			double *old_meanji=*old_meanj++;
			double *meanji=*meanj++;
			double *en0ji=*en0j++;

			unsigned int i;

			for(i=0;i<n;i++){
				/* Start with first file as the estimated average */
				//old_mean[j][i]=t[j][i]=en[0][j][i];
				*old_meanji++=*meanji++=*en0ji++;
			}
		}
	}
	{
		unsigned int its;
		double fitold=DBL_MAX;
		AIR_Boolean small2=FALSE;
		unsigned int which=0;
		double ***enwhich=en+1;

		for(its=0;its<MAXITERS;its++){

			unsigned int ipvt[4];
			
			/*Minimize log*/

			/* Define estimated average as standard space */

			if(AIR_dgefa(mean,n,ipvt)!=n) return AIR_SINGULAR_COMLOGER_ERROR;
			{
				double **log_sumj=log_sum;
				unsigned int j;

				for(j=0;j<n;j++){

					double *log_sumji=*log_sumj++;
					unsigned int i;

					for(i=0;i<n;i++){
					
						//log_sum[j][i]=0.0;
						*log_sumji++=0.0;
					}
				}
			}

			{
				double ***enk=en;
				unsigned int k;

				for(k=0;k<count;k++){
				
					{
						double **enkj=*enk++;
						double **ej=e;
						
						unsigned int j;
						
						for(j=0;j<n;j++){
						
							double *enkji=*enkj++;
							double *eji=*ej++;
							
							unsigned int i;
						
							for(i=0;i<n;i++){
								
								//e[j][i]=en[k][j][i];
								*eji++=*enkji++;
							}
							AIR_dgesl(mean,n,ipvt,e[j],FALSE);
						}
					}
					/*Find log of e*/
					{
						AIR_Error errcode=AIR_eloger_pade(n,e,storage4,is_affine);
						
						if(errcode!=0){
							which++;
							if(which<count){
								{
									double **enwhichj=*enwhich++;
									double **old_meanj=old_mean;
									double **meanj=mean;
									double **log_sumj=log_sum;
									
									unsigned int j;

									for(j=0;j<n;j++){

										double *enwhichji=*enwhichj++;
										double *old_meanji=*old_meanj++;
										double *meanji=*meanj++;
										double *log_sumji=*log_sumj++;
										
										unsigned int i;

										for(i=0;i<n;i++){
											/* Restart with next file as the estimated average */
											//old_mean[j][i]=t[j][i]=en[which][j][i];
											*old_meanji++=*meanji++=*enwhichji++;
											//log_sum[j][i]=0.0;
											*log_sumji++=0.0;
										}
									}
								}
								if(AIR_dgefa(mean,n,ipvt)!=n) return AIR_SINGULAR_COMLOGER_ERROR;
								enk=en;
								k=0;
								its=0;
								continue;
							}
							else return errcode;
						}
					}
					{
						double **log_sumj=log_sum;
						double **ej=e;
						
						unsigned int j;

						for(j=0;j<n;j++){

							double *log_sumji=*log_sumj++;
							double *eji=*ej++;
							
							unsigned int i;

							for(i=0;i<n;i++){
								//log_sum[j][i]+=e[j][i];
								*log_sumji+++=*eji++;
							}
						}
					}
				}
			}
			{
				double **log_sumj=log_sum;
				double **ej=e;
				
				unsigned int j;

				for(j=0;j<n;j++){

					double *log_sumji=*log_sumj++;
					double *eji=*ej++;
					
					unsigned int i;

					for(i=0;i<n;i++){
						//log_sum[j][i]/=count;
						*log_sumji/=count;
						//e[j][i]=log_sum[j][i];
						*eji++=*log_sumji++;
					}
				}
			}
			
			/*Find exp e*/
			{
				AIR_Error errcode=AIR_eexper_pade(n,e,storage4,is_affine);

				if(errcode!=0) return errcode;
			}

			//AIR_mulmat(old_mean,4,4,e,4,mean);	// WARNING: old_mean, e, and mean array structure assumptions are made
			
			AIR_dgemm('n', 'n', n, n, n, 1.0, old_mean, e, 0.0, mean);

			/*Reset for next iteration*/
			{
				double **old_meanj=old_mean;
				double **meanj=mean;
				
				unsigned int j;

				for(j=0;j<n;j++){
				
					double *old_meanji=*old_meanj++;
					double *meanji=*meanj++;

					unsigned int i;

					for(i=0;i<n;i++){
						//old_mean[j][i]=t[j][i];
						*old_meanji++=*meanji++;
					}
				}
			}

			if(!small2){
				small2=TRUE;
				{
					double **log_sumj=log_sum;
					
					unsigned int j;

					for(j=0;j<n;j++){

						double *log_sumji=*log_sumj++;
						
						unsigned int i;

						for(i=0;i<n;i++){
						
							//if(fabs(log_sum[j][i])>TINY) small2=FALSE;
							if(fabs(*log_sumji++)>TINY){
								small2=FALSE;
								break;
							}
						}
					}
				}
			}
			else{
				double fitnew=0.0;
				{
					double **log_sumj=log_sum;
					
					unsigned int j;

					for(j=0;j<n;j++){
					
						double *log_sumji=*log_sumj++;

						unsigned int i;

						for(i=0;i<n;i++){
							//double tempvalue=fabs(log_sum[j][i]);
							double tempvalue=fabs(*log_sumji++);
							fitnew+=tempvalue*tempvalue;
						}
					}
				}
				if(fitnew>=fitold) break;
				fitold=fitnew;
			}
		} /* End of loop */
		if(its==MAXITERS && !small2) return AIR_COMLOGER_CONVERGE_ERROR;
	}
	/*Define new standard file based on mean of reslice files
	 * Note that the mean is bi-invariant, which is why the mean derived above with inv(mean)*e
	 * can now be used to computed adjusted means as e*inv(mean)
	 */
	{
		unsigned int ipvt[4];

		if(AIR_dgefa(mean,n,ipvt)!=n) return AIR_SINGULAR_COMLOGER_ERROR;
		{
			unsigned int k;

			for(k=0;k<count;k++){

				unsigned int j;

				for(j=0;j<n;j++){
					/*Copy row j of en[k] into work*/

					double work[4];
					{
						unsigned int i;

						for(i=0;i<n;i++){
							work[i]=en[k][i][j];
						}
					}
					/*Compute row j of output using dgesl in transpose mode*/
					AIR_dgesl(mean,n,ipvt,work,TRUE);
					/*Copy this back into en*/
					{
						unsigned int i;

						for(i=0;i<n;i++){
							en[k][i][j]=work[i];
						}
					}
				}
			}
		}
	}
	return 0;
}
