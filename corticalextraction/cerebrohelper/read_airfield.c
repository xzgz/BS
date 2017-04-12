/* Copyright 2001-2002 Roger P. Woods, M.D. */
/* Modified: 3/11/11 */

/*
 * Loads a vector field
 *
 * If an error occurs, an error code will be returned and both *field2 and *field3 will be NULL
 *
 * If no error occurs, 0 is returned and either field2 or field3 (not both) will be non-NULL
 * airfield1->coord will flag the dimensions (and hence which field is non-NULL)
 */
 
#include "AIR.h"

static void free_function_on_error(/*@null@*/ float *****field3, /*@null@*/ float ****field2)
{
	if(*field2) AIR_free_3f(*field2);
	if(*field3) AIR_free_4f(*field3);
}

/* This static method is for reading AIR_Field structs that were generated with
 * 8 byte r_hash and s_hash values
 */
static AIR_Error read_bad_airfield(const char *filename, struct AIR_Field *airfield1, float *****field3, float ****field2)
{
	struct AIR_Badfield airfield2;
 	*field2=NULL;
 	*field3=NULL;
 	{
	 	FILE *fp=fopen(filename,"rb");
	 	
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
	 	AIR_Boolean must_swap=FALSE;
#endif
	 	
	 	if(!fp){
	 	
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("\nunable to open vector field file '%s'\n",filename);
			{
				AIR_Error errcode=AIR_fprobr(filename);
				if(errcode==0) return AIR_UNSPECIFIED_FILE_READ_ERROR;
				return(errcode);
			}
		}
		if(fread(&airfield2, sizeof(struct AIR_Badfield),1,fp)!=1){
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("\nfailed to read data from vector field file '%s'\n",filename);
			(void)fclose(fp);
			return AIR_READ_VECTOR_FILE_ERROR;
		}
		strncpy(airfield1->s_file, airfield2.s_file, AIR_CONFIG_MAX_PATH_LENGTH);
		strncpy(airfield1->r_file, airfield2.r_file, AIR_CONFIG_MAX_PATH_LENGTH);
		strncpy(airfield1->comment, airfield2.comment, AIR_CONFIG_MAX_COMMENT_LENGTH);
		strncpy(airfield1->reserved, airfield2.reserved, AIR_CONFIG_RESERVED_LENGTH);
		strncpy(airfield1->pad, airfield2.pad, 4);
		
		airfield1->coord=airfield2.coord;

		airfield1->s.bits=airfield2.s.bits;
		airfield1->s.x_dim=airfield2.s.x_dim;
		airfield1->s.y_dim=airfield2.s.y_dim;
		airfield1->s.z_dim=airfield2.s.z_dim;
		airfield1->s.x_size=airfield2.s.x_size;
		airfield1->s.y_size=airfield2.s.y_size;
		airfield1->s.z_size=airfield2.s.z_size;
		airfield1->r.bits=airfield2.r.bits;
		airfield1->r.x_dim=airfield2.r.x_dim;
		airfield1->r.y_dim=airfield2.r.y_dim;
		airfield1->r.z_dim=airfield2.r.z_dim;
		airfield1->r.x_size=airfield2.r.x_size;
		airfield1->r.y_size=airfield2.r.y_size;
		airfield1->r.z_size=airfield2.r.z_size;
		airfield1->s_hash=airfield2.s_hash;
		airfield1->r_hash=airfield2.r_hash;
		airfield1->s_volume=airfield2.s_volume;
		airfield1->r_volume=airfield2.r_volume;

#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
		if(airfield1->s.bits> (unsigned int)sqrt((double)UINT_MAX)){
		
			must_swap=TRUE;
		
			AIR_swapbytes(&(airfield1->coord),sizeof(airfield1->coord)/2,1);
			AIR_swapbytes(&(airfield1->s_hash),sizeof(airfield1->s_hash)/2,1);
			AIR_swapbytes(&(airfield1->r_hash),sizeof(airfield1->r_hash)/2,1);
			AIR_swapbytes(&(airfield1->s_volume),sizeof(airfield1->s_volume)/2,1);
			AIR_swapbytes(&(airfield1->r_volume),sizeof(airfield1->r_volume)/2,1);
			
			AIR_swapbytes(&(airfield1->s.bits),sizeof(airfield1->s.bits)/2,1);
			AIR_swapbytes(&(airfield1->s.x_dim),sizeof(airfield1->s.x_dim)/2,1);
			AIR_swapbytes(&(airfield1->s.y_dim),sizeof(airfield1->s.y_dim)/2,1);
			AIR_swapbytes(&(airfield1->s.z_dim),sizeof(airfield1->s.z_dim)/2,1);
			AIR_swapbytes(&(airfield1->s.x_size),sizeof(airfield1->s.x_size)/2,1);
			AIR_swapbytes(&(airfield1->s.y_size),sizeof(airfield1->s.y_size)/2,1);
			AIR_swapbytes(&(airfield1->s.z_size),sizeof(airfield1->s.z_size)/2,1);
			
			AIR_swapbytes(&(airfield1->r.bits),sizeof(airfield1->r.bits)/2,1);
			AIR_swapbytes(&(airfield1->r.x_dim),sizeof(airfield1->r.x_dim)/2,1);
			AIR_swapbytes(&(airfield1->r.y_dim),sizeof(airfield1->r.y_dim)/2,1);
			AIR_swapbytes(&(airfield1->r.z_dim),sizeof(airfield1->r.z_dim)/2,1);
			AIR_swapbytes(&(airfield1->r.x_size),sizeof(airfield1->r.x_size)/2,1);
			AIR_swapbytes(&(airfield1->r.y_size),sizeof(airfield1->r.y_size)/2,1);
			AIR_swapbytes(&(airfield1->r.z_size),sizeof(airfield1->r.z_size)/2,1);
		}
#endif
		if(airfield1->coord==3){
			*field3=AIR_matrix4f(3,airfield1->s.x_dim,airfield1->s.y_dim,airfield1->s.z_dim);
			if(!*field3){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("memory allocation failure\n");
				(void)fclose(fp);
				return AIR_MEMORY_ALLOCATION_ERROR;
			}
			if(fread(*field3[0][0][0],sizeof(float),airfield1->s.x_dim*airfield1->s.y_dim*airfield1->s.z_dim*3,fp)!=airfield1->s.x_dim*airfield1->s.y_dim*airfield1->s.z_dim*3){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'\n",filename);
				free_function_on_error(field3,field2);
				(void)fclose(fp);
				return AIR_READ_VECTOR_FILE_ERROR;	
			}
			if(fgetc(fp)!=EOF){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'--file was too long\n",filename);
				free_function_on_error(field3,field2);
				(void)fclose(fp);
				return AIR_READ_VECTOR_FILE_ERROR;	
			}
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
			if(must_swap){
				AIR_swapbytes(****field3,sizeof(field3[0][0][0][0][0])/2,(size_t)3*(size_t)airfield1->s.x_dim*(size_t)airfield1->s.y_dim*(size_t)airfield1->s.z_dim);
			}
#endif
		}
		else if(airfield1->coord==2){
			*field2=AIR_matrix3f(2,airfield1->s.x_dim,airfield1->s.y_dim);
			if(!*field2){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("memory allocation failure\n");
				(void)fclose(fp);
				return AIR_MEMORY_ALLOCATION_ERROR;
			}
			if(fread(*field2[0][0],sizeof(float),airfield1->s.x_dim*airfield1->s.y_dim*2,fp)!=airfield1->s.x_dim*airfield1->s.y_dim*2){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'\n",filename);
				free_function_on_error(field3,field2);
				(void)fclose(fp);
				return AIR_READ_VECTOR_FILE_ERROR;	
			}
			if(fgetc(fp)!=EOF){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'--file was too long\n",filename);
				free_function_on_error(field3,field2);				
				(void)fclose(fp);
				return AIR_READ_VECTOR_FILE_ERROR;	
			}
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
			if(must_swap){
				AIR_swapbytes(***field2,sizeof(field2[0][0][0][0])/2,(size_t)2*(size_t)airfield1->s.x_dim*(size_t)airfield1->s.y_dim);
			}
#endif
		}
		else{
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("unimplemented number of coordinates in vector field file %s\n",filename);
			free_function_on_error(field3,field2);			
			(void)fclose(fp);
			return(AIR_READ_VECTOR_FILE_ERROR);
		}
		if(fclose(fp)!=0){
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("failed to close input file %s\n",filename);
			free_function_on_error(field3,field2);
			return(AIR_CANT_CLOSE_READ_ERROR);
		}
	}
	return 0;
}

 
AIR_Error AIR_read_airfield(const char *filename, struct AIR_Field *airfield1, float *****field3, float ****field2)
 {
 	*field2=NULL;
 	*field3=NULL;
 	{
	 	FILE *fp=fopen(filename,"rb");
	 	
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
	 	AIR_Boolean must_swap=FALSE;
#endif
	 	
	 	if(!fp){
	 	
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("\nunable to open vector field file '%s'\n",filename);
			{
				AIR_Error errcode=AIR_fprobr(filename);
				if(errcode==0) return AIR_UNSPECIFIED_FILE_READ_ERROR;
				return(errcode);
			}
		}
		if(fread(airfield1,sizeof(struct AIR_Field),1,fp)!=1){
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("\nfailed to read data from vector field file '%s'\n",filename);
			(void)fclose(fp);
			return AIR_READ_VECTOR_FILE_ERROR;
		}
		
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
		if(airfield1->s.bits> (unsigned int)sqrt((double)UINT_MAX)){
		
			must_swap=TRUE;
		
			AIR_swapbytes(&(airfield1->coord),sizeof(airfield1->coord)/2,1);
			AIR_swapbytes(&(airfield1->s_hash),sizeof(airfield1->s_hash)/2,1);
			AIR_swapbytes(&(airfield1->r_hash),sizeof(airfield1->r_hash)/2,1);
			AIR_swapbytes(&(airfield1->s_volume),sizeof(airfield1->s_volume)/2,1);
			AIR_swapbytes(&(airfield1->r_volume),sizeof(airfield1->r_volume)/2,1);
			
			AIR_swapbytes(&(airfield1->s.bits),sizeof(airfield1->s.bits)/2,1);
			AIR_swapbytes(&(airfield1->s.x_dim),sizeof(airfield1->s.x_dim)/2,1);
			AIR_swapbytes(&(airfield1->s.y_dim),sizeof(airfield1->s.y_dim)/2,1);
			AIR_swapbytes(&(airfield1->s.z_dim),sizeof(airfield1->s.z_dim)/2,1);
			AIR_swapbytes(&(airfield1->s.x_size),sizeof(airfield1->s.x_size)/2,1);
			AIR_swapbytes(&(airfield1->s.y_size),sizeof(airfield1->s.y_size)/2,1);
			AIR_swapbytes(&(airfield1->s.z_size),sizeof(airfield1->s.z_size)/2,1);
			
			AIR_swapbytes(&(airfield1->r.bits),sizeof(airfield1->r.bits)/2,1);
			AIR_swapbytes(&(airfield1->r.x_dim),sizeof(airfield1->r.x_dim)/2,1);
			AIR_swapbytes(&(airfield1->r.y_dim),sizeof(airfield1->r.y_dim)/2,1);
			AIR_swapbytes(&(airfield1->r.z_dim),sizeof(airfield1->r.z_dim)/2,1);
			AIR_swapbytes(&(airfield1->r.x_size),sizeof(airfield1->r.x_size)/2,1);
			AIR_swapbytes(&(airfield1->r.y_size),sizeof(airfield1->r.y_size)/2,1);
			AIR_swapbytes(&(airfield1->r.z_size),sizeof(airfield1->r.z_size)/2,1);
		}
#endif
		if(airfield1->coord==3){
			*field3=AIR_matrix4f(3,airfield1->s.x_dim,airfield1->s.y_dim,airfield1->s.z_dim);
			if(!*field3){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("memory allocation failure\n");
				(void)fclose(fp);
				return AIR_MEMORY_ALLOCATION_ERROR;
			}
			if(fread(*field3[0][0][0],sizeof(float),airfield1->s.x_dim*airfield1->s.y_dim*airfield1->s.z_dim*3,fp)!=airfield1->s.x_dim*airfield1->s.y_dim*airfield1->s.z_dim*3){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'\n",filename);
				free_function_on_error(field3,field2);
				(void)fclose(fp);
				return AIR_READ_VECTOR_FILE_ERROR;	
			}
			if(fgetc(fp)!=EOF){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'--file was too long\n",filename);
				free_function_on_error(field3,field2);
				(void)fclose(fp);
				return read_bad_airfield(filename, airfield1, field3, field2);
			}
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
			if(must_swap){
				AIR_swapbytes(****field3,sizeof(field3[0][0][0][0][0])/2,(size_t)3*(size_t)airfield1->s.x_dim*(size_t)airfield1->s.y_dim*(size_t)airfield1->s.z_dim);
			}
#endif
		}
		else if(airfield1->coord==2){
			*field2=AIR_matrix3f(2,airfield1->s.x_dim,airfield1->s.y_dim);
			if(!*field2){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("memory allocation failure\n");
				(void)fclose(fp);
				return AIR_MEMORY_ALLOCATION_ERROR;
			}
			if(fread(*field2[0][0],sizeof(float),airfield1->s.x_dim*airfield1->s.y_dim*2,fp)!=airfield1->s.x_dim*airfield1->s.y_dim*2){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'\n",filename);
				free_function_on_error(field3,field2);
				(void)fclose(fp);
				return AIR_READ_VECTOR_FILE_ERROR;	
			}
			if(fgetc(fp)!=EOF){
				printf("%s: %d: ",__FILE__,__LINE__);
				printf("\nfailed to read data from vector field file '%s'--file was too long\n",filename);
				free_function_on_error(field3,field2);				
				(void)fclose(fp);
				return read_bad_airfield(filename, airfield1, field3, field2);	
			}
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
			if(must_swap){
				AIR_swapbytes(***field2,sizeof(field2[0][0][0][0])/2,(size_t)2*(size_t)airfield1->s.x_dim*(size_t)airfield1->s.y_dim);
			}
#endif
		}
		else{
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("unimplemented number of coordinates in vector field file %s\n",filename);
			free_function_on_error(field3,field2);			
			(void)fclose(fp);
			return(AIR_READ_VECTOR_FILE_ERROR);
		}
		if(fclose(fp)!=0){
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("failed to close input file %s\n",filename);
			free_function_on_error(field3,field2);
			return(AIR_CANT_CLOSE_READ_ERROR);
		}
	}
	return 0;
}
