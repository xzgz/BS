/* Copyright 1999-2005 Roger P. Woods, M.D. */
/* Modified: 12/6/05 */

/*
 * AIR_Pixels *******create_vols()
 *
 * These routines allocate memory for multidimensional image data
 *
 * Note that the indices are called in reverse of the accessing order,
 *  e.g. volume=create_vol3(x,y,z) assures volume[z-1][y-1][x-1] to be 
 *  a legitimate address
 *
 * If a dimension is listed as zero, it and all dimensions to its left
 *  are unallocated and are set to NULL
 *
 * Returns:
 *	pointer to allocated memory if successful
 *	NULL pointer if unsuccessful
 *
 */

#include "AIR.h"

AIR_Pixels *********AIR_create_vol9(const unsigned int i, const unsigned int h, const unsigned int g, const unsigned int f, const unsigned int e, const unsigned int d, const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels *********high=(AIR_Pixels *********)malloc(a*sizeof(AIR_Pixels ********));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels ********low=AIR_create_vol8(i,h,g,f,e,d,c,b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels *********ptr;
			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels *********ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels ********AIR_create_vol8(const unsigned int h, const unsigned int g, const unsigned int f, const unsigned int e, const unsigned int d, const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels ********high=(AIR_Pixels ********)malloc(a*sizeof(AIR_Pixels *******));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels *******low=AIR_create_vol7(h,g,f,e,d,c,b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels ********ptr;
			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels ********ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels *******AIR_create_vol7(const unsigned int g, const unsigned int f, const unsigned int e, const unsigned int d, const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels *******high=(AIR_Pixels *******)malloc(a*sizeof(AIR_Pixels ******));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels ******low=AIR_create_vol6(g,f,e,d,c,b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels *******ptr;
			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels *******ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels ******AIR_create_vol6(const unsigned int f, const unsigned int e, const unsigned int d, const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels ******high=(AIR_Pixels ******)malloc(a*sizeof(AIR_Pixels *****));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels *****low=AIR_create_vol5(f,e,d,c,b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels ******ptr;

			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels ******ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels *****AIR_create_vol5(const unsigned int e, const unsigned int d, const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels *****high=(AIR_Pixels *****)malloc(a*sizeof(AIR_Pixels ****));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels ****low=AIR_create_vol4(e,d,c,b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels *****ptr;
			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels *****ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels ****AIR_create_vol4(const unsigned int d, const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels ****high=(AIR_Pixels ****)malloc(a*sizeof(AIR_Pixels ***));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels ***low=AIR_create_vol3(d,c,b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels ****ptr;
			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels ****ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels ***AIR_create_vol3(const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels ***high=(AIR_Pixels ***)malloc(a*sizeof(AIR_Pixels **));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels **low=AIR_create_vol2(c,b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels ***ptr;
			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels ***ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels **AIR_create_vol2(const unsigned int b, const unsigned int a)
{
	AIR_Pixels **high=(AIR_Pixels **)malloc(a*sizeof(AIR_Pixels *));
	if(!high) return NULL;

	if(b!=0){
		AIR_Pixels *low=AIR_create_vol1(b*a);
		if(!low){
			free(high);
			return NULL;
		}
		{
			AIR_Pixels **ptr;
			for(ptr=high;ptr<high+a;ptr++,low+=b){
				*ptr=low;
			}
		}
	}
	else{
		AIR_Pixels **ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}
AIR_Pixels *AIR_create_vol1(const unsigned int a)
{
	AIR_Pixels *high=(AIR_Pixels *)malloc(a*sizeof(AIR_Pixels));
	if(!high) return NULL;
	return high;
}

AIR_Pixels ****AIR_create_vol44D(const unsigned int d, const unsigned int c, const unsigned int b, const unsigned int a)
{
	AIR_Pixels ****high=(AIR_Pixels ****)malloc(a*sizeof(AIR_Pixels ***));
	if(!high) return NULL;

	if(b!=0){
	
		unsigned int tpoints;
		
		for(tpoints=0; tpoints<a; tpoints++){
		
			high[tpoints]=AIR_create_vol3(d,c,b);
			if(!high[tpoints]){
				for(tpoints++; tpoints<a; tpoints++) high[tpoints]=NULL;
				AIR_free_vol44D(high, a);
				return NULL;
			}
		}
	}
	else{
		AIR_Pixels ****ptr;
		for(ptr=high;ptr<high+a;ptr++){
			*ptr=NULL;
		}
	}
	return high;
}






