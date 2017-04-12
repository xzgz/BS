/* Copyright 1995-2011 Roger P. Woods, M.D. */
/* Modified 3/13/11 */

/*
 * AIR_Pixels ***r_persp_lin()
 *
 * This routine will reslice a file based on the unit vectors es
 *
 * returns:
 *	pointer to resliced data if successful
 *	NULL pointer if unsuccessful
 *	error code in *errcode
 *
 */

#include "AIR.h"

AIR_Pixels ***AIR_r_persp_lin(AIR_Pixels ***volume, const struct AIR_Key_info *stats, struct AIR_Key_info *new_stats, double **es, const double scale, AIR_Error *errcode)

{
	if(stats->x_dim==0 || stats->y_dim==0 || stats->z_dim==0 || new_stats->x_dim==0 || new_stats->y_dim==0 || new_stats->z_dim==0){
		printf("%s: %d: ",__FILE__,__LINE__);
		printf("Invalid volume dimension\n");
		*errcode=AIR_VOLUME_ZERO_DIM_ERROR;
		return NULL;
	}
	{
		double
			x_max1=(stats->x_dim-1),
			y_max1=(stats->y_dim-1),
			z_max1=(stats->z_dim-1);
	
		unsigned int
			x_dim2=new_stats->x_dim,
			y_dim2=new_stats->y_dim,
			z_dim2=new_stats->z_dim;
	
		double
			e00=es[0][0],
			e01=es[0][1],
			e02=es[0][2],
			e03=es[0][3],
			e10=es[1][0],
			e11=es[1][1],
			e12=es[1][2],
			e13=es[1][3],
			e20=es[2][0],
			e21=es[2][1],
			e22=es[2][2],
			e23=es[2][3],
			e30=es[3][0],
			e31=es[3][1],
			e32=es[3][2],
			e33=es[3][3];
	
	
		AIR_Pixels ***new_volume=AIR_create_vol3(new_stats->x_dim,new_stats->y_dim,new_stats->z_dim);
		if(!new_volume){
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("unable to allocate memory to reslice file\n");
			*errcode=AIR_MEMORY_ALLOCATION_ERROR;
			return NULL;
		}
		AIR_zeros(new_volume,new_stats);	/*This must be called to use the breaks associated with x_in*/
	
	
		/* Reslice */
		{
			AIR_Pixels ***k3=new_volume;
			AIR_Pixels ***k3_end=k3+z_dim2;
				
			double
				x_k=e30,
				y_k=e31,
				z_k=e32,
				t_k=e33;
	
			for (; k3<k3_end; k3++,x_k+=e20,y_k+=e21,z_k+=e22,t_k+=e23){
	
				AIR_Pixels **j3=*k3;
				AIR_Pixels **j3_end=j3+y_dim2;
					
				double
					x_j=x_k,
					y_j=y_k,
					z_j=z_k,
					t_j=t_k;
	
				for (; j3<j3_end; j3++,x_j+=e10,y_j+=e11,z_j+=e12,t_j+=e13){
	
					AIR_Boolean x_in=FALSE;
	
					AIR_Pixels *i3=*j3;
					AIR_Pixels *i3_end=i3+x_dim2;
						
					double
						x_i=x_j,
						y_i=y_j,
						z_i=z_j,
						t_i=t_j;
	
					for (; i3<i3_end; i3++,x_i+=e00,y_i+=e01,z_i+=e02,t_i+=e03){
	
						double x_p=x_i/t_i;
	
						if(x_p>=0.0 && x_p<=x_max1){
	
							double y_p=y_i/t_i;
	
							if(y_p>=0.0 && y_p<=y_max1){
	
								double z_p=z_i/t_i;
	
								if(z_p>=0.0 && z_p<=z_max1){
	
									unsigned int x_down=x_p;
									unsigned int y_down=y_p;
									unsigned int z_down=z_p;
									
									double a=x_p-x_down;
									double b=y_p-y_down;
									double c=z_p-z_down;
									
									double d=1.0-a;
									double e=1.0-b;
									double f=1.0-c;
	
									x_in=TRUE;
	
									/* Compute interpolated value */
									{
										AIR_Pixels ***k2=volume+z_down;
										AIR_Pixels **j2=*k2+y_down;
										AIR_Pixels *i2=*j2+x_down;
											
										double total=*i2*d*e*f;
	
										if (a!=0.0){
											i2++;
											total+=*i2*a*e*f;
										}
	
										if (b!=0.0){
											j2++;
											i2=*j2+x_down;
											total+=*i2*d*b*f;
	
											if (a!=0.0){
												i2++;
												total+=*i2*a*b*f;
											}
										}
	
										if (c!=0.0){
											k2++;
											j2=*k2+y_down;
											i2=*j2+x_down;
											total+=*i2*d*e*c;
	
											if (a!=0.0){
												i2++;
												total+=*i2*a*e*c;
											}
	
											if (b!=0.0){
												j2++;
												i2=*j2+x_down;
												total+=*i2*d*b*c;
	
												if (a!=0.0){
													i2++;
													total+=*i2*a*b*c;
												}
											}
										}
										total*=scale;
										total+=.5;
										if(total>(double)AIR_CONFIG_MAX_POSS_VALUE) *i3=AIR_CONFIG_MAX_POSS_VALUE;
										else *i3=(AIR_Pixels)total;
									}
								}
								else if(x_in) break;
							}
							else if(x_in) break;
						}
						else if(x_in) break;
					}
				}
			}
		}
		*errcode=0;
		return new_volume;
	}
}
