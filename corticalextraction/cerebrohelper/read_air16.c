/* Copyright 1995-2011 Roger P. Woods, M.D. */
/* Modified: 3/11/11 */

/*
 * reads an AIR reslice parameter file into an air struct
 *
 * program will also attempt to convert old 12 parameter
 * AIR files into new 15 parameter files
 *
 * program will also attempt to convert 15 AIR files that were
 * generated with 8 byte values for s_hash and r_hash
 * into 4 byte values
 *
 * returns:
 *	0 if successful
 *	error code if unsuccessful
 */

#include "AIR.h"
#include <errno.h>


static void swap_Air16(struct AIR_Air16 *air1){

	/* Byte swap Air16 struct members */
	
	AIR_swapbytes(&(air1->s.bits),sizeof(air1->s.bits)/2,1);
	AIR_swapbytes(&(air1->s.x_dim),sizeof(air1->s.x_dim)/2,1);
	AIR_swapbytes(&(air1->s.y_dim),sizeof(air1->s.y_dim)/2,1);
	AIR_swapbytes(&(air1->s.z_dim),sizeof(air1->s.z_dim)/2,1);
	AIR_swapbytes(&(air1->s.x_size),sizeof(air1->s.x_size)/2,1);
	AIR_swapbytes(&(air1->s.y_size),sizeof(air1->s.y_size)/2,1);
	AIR_swapbytes(&(air1->s.z_size),sizeof(air1->s.z_size)/2,1);
	
	AIR_swapbytes(&(air1->r.bits),sizeof(air1->r.bits)/2,1);
	AIR_swapbytes(&(air1->r.x_dim),sizeof(air1->r.x_dim)/2,1);
	AIR_swapbytes(&(air1->r.y_dim),sizeof(air1->r.y_dim)/2,1);
	AIR_swapbytes(&(air1->r.z_dim),sizeof(air1->r.z_dim)/2,1);
	AIR_swapbytes(&(air1->r.x_size),sizeof(air1->r.x_size)/2,1);
	AIR_swapbytes(&(air1->r.y_size),sizeof(air1->r.y_size)/2,1);
	AIR_swapbytes(&(air1->r.z_size),sizeof(air1->r.z_size)/2,1);
	
	AIR_swapbytes(&(air1->s_hash),sizeof(air1->s_hash)/2,1);
	AIR_swapbytes(&(air1->r_hash),sizeof(air1->r_hash)/2,1);
	
	AIR_swapbytes(&(air1->s_volume),sizeof(air1->s_volume)/2,1);
	AIR_swapbytes(&(air1->r_volume),sizeof(air1->r_volume)/2,1);
			
	AIR_swapbytes(&(air1->e[0][0]),sizeof(air1->e[0][0])/2,1);
	AIR_swapbytes(&(air1->e[0][1]),sizeof(air1->e[0][1])/2,1);
	AIR_swapbytes(&(air1->e[0][2]),sizeof(air1->e[0][2])/2,1);
	AIR_swapbytes(&(air1->e[0][3]),sizeof(air1->e[0][3])/2,1);
	AIR_swapbytes(&(air1->e[1][0]),sizeof(air1->e[1][0])/2,1);
	AIR_swapbytes(&(air1->e[1][1]),sizeof(air1->e[1][1])/2,1);
	AIR_swapbytes(&(air1->e[1][2]),sizeof(air1->e[1][2])/2,1);
	AIR_swapbytes(&(air1->e[1][3]),sizeof(air1->e[1][3])/2,1);
	AIR_swapbytes(&(air1->e[2][0]),sizeof(air1->e[2][0])/2,1);
	AIR_swapbytes(&(air1->e[2][1]),sizeof(air1->e[2][1])/2,1);
	AIR_swapbytes(&(air1->e[2][2]),sizeof(air1->e[2][2])/2,1);
	AIR_swapbytes(&(air1->e[2][3]),sizeof(air1->e[2][3])/2,1);
	AIR_swapbytes(&(air1->e[3][0]),sizeof(air1->e[3][0])/2,1);
	AIR_swapbytes(&(air1->e[3][1]),sizeof(air1->e[3][1])/2,1);
	AIR_swapbytes(&(air1->e[3][2]),sizeof(air1->e[3][2])/2,1);
	AIR_swapbytes(&(air1->e[3][3]),sizeof(air1->e[3][3])/2,1);
}

AIR_Error AIR_read_air16(const char *filename, struct AIR_Air16 *air1)
{
	{
		FILE *fp=fopen(filename,"rb");
		if(!fp){
	
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("unable to open file %s for input\n",filename);
			{
				AIR_Error error=AIR_fprobr(filename);
				if(error==0) return AIR_UNSPECIFIED_FILE_READ_ERROR;
				return error;
			}
		}
		{
			long int file_length;
			{
				if(fseek(fp, 0L, SEEK_END)!=0){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("unable to parse contents of file %s for input\n", filename);
					(void)fclose(fp);
					return AIR_UNSPECIFIED_FILE_READ_ERROR;
				}
				file_length=ftell(fp);
				if(file_length<=0){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("unable to parse contents of file %s for input\n", filename);
					(void)fclose(fp);
					return AIR_UNSPECIFIED_FILE_READ_ERROR;
				}
				errno=0;
				rewind(fp);
				if(errno!=0){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("unable to parse contents of file %s for input\n", filename);
					(void)fclose(fp);
					return AIR_UNSPECIFIED_FILE_READ_ERROR;
				}
			}
			
			if((size_t)file_length==sizeof(struct AIR_Air16)){
				
				/* Read directly as normal AIR file */
				if(fread(air1,1,sizeof(struct AIR_Air16),fp)!=sizeof(struct AIR_Air16)){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("file read error for file %s\n",filename);
					(void)fclose(fp);
					return AIR_READ_AIR_FILE_ERROR;
				}
				if(fclose(fp)!=0){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("error closing file %s\n",filename);
					return AIR_CANT_CLOSE_READ_ERROR;
				}				
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
				if(air1->s.bits> (unsigned int)sqrt((double)UINT_MAX)) swap_Air16(air1);
#endif
				return 0;
			}

			if(((size_t)file_length==sizeof(struct AIR_Badair16))){
			
				/* Read as air file with 8 byte s_hash and r_hash */
				struct AIR_Badair16 air2;
				
				if(fread(&air2,1,sizeof(struct AIR_Badair16),fp)!=sizeof(struct AIR_Badair16)){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("file read error for file %s\n",filename);
					(void)fclose(fp);
					return AIR_READ_AIR_FILE_ERROR;
				}
				if(fclose(fp)!=0){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("error closing file %s\n",filename);
					return AIR_CANT_CLOSE_READ_ERROR;
				}
				strncpy(air1->s_file,air2.s_file, AIR_CONFIG_MAX_PATH_LENGTH);
				strncpy(air1->r_file,air2.r_file, AIR_CONFIG_MAX_PATH_LENGTH);
				strncpy(air1->comment,air2.comment, AIR_CONFIG_MAX_COMMENT_LENGTH);
				strncpy(air1->reserved,air2.reserved, AIR_CONFIG_RESERVED_LENGTH);
				air1->s.bits=air2.s.bits;
				air1->s.x_dim=air2.s.x_dim;
				air1->s.y_dim=air2.s.y_dim;
				air1->s.z_dim=air2.s.z_dim;
				air1->s.x_size=air2.s.x_size;
				air1->s.y_size=air2.s.y_size;
				air1->s.z_size=air2.s.z_size;
				air1->r.bits=air2.r.bits;
				air1->r.x_dim=air2.r.x_dim;
				air1->r.y_dim=air2.r.y_dim;
				air1->r.z_dim=air2.r.z_dim;
				air1->r.x_size=air2.r.x_size;
				air1->r.y_size=air2.r.y_size;
				air1->r.z_size=air2.r.z_size;
				air1->s_hash=air2.s_hash;
				air1->r_hash=air2.r_hash;
				air1->s_volume=air2.s_volume;
				air1->r_volume=air2.r_volume;
				air1->e[0][0]=air2.e[0][0];
				air1->e[0][1]=air2.e[0][1];
				air1->e[0][2]=air2.e[0][2];
				air1->e[0][3]=air2.e[0][3];
				air1->e[1][0]=air2.e[1][0];
				air1->e[1][1]=air2.e[1][1];
				air1->e[1][2]=air2.e[1][2];
				air1->e[1][3]=air2.e[1][3];
				air1->e[2][0]=air2.e[2][0];
				air1->e[2][1]=air2.e[2][1];
				air1->e[2][2]=air2.e[2][2];
				air1->e[2][3]=air2.e[2][3];
				air1->e[3][0]=air2.e[3][0];
				air1->e[3][1]=air2.e[3][1];
				air1->e[3][2]=air2.e[3][2];
				air1->e[3][3]=air2.e[3][3];
				
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
				if(air1->s.bits> (unsigned int)sqrt((double)UINT_MAX)) swap_Air16(air1);
#endif
				return 0;
			}
			
			if(((size_t)file_length==sizeof(struct AIR_Oldair))){
			
				/* Read as an old 12 parameter air file */
				struct AIR_Oldair air2;
				
				if(fread(&air2,1,sizeof(struct AIR_Oldair),fp)!=sizeof(struct AIR_Oldair)){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("file read error for file %s\n",filename);
					(void)fclose(fp);
					return AIR_READ_AIR_FILE_ERROR;
				}
				if(fclose(fp)!=0){
					printf("%s: %d: ",__FILE__,__LINE__);
					printf("error closing file %s\n",filename);
					return AIR_CANT_CLOSE_READ_ERROR;
				}
				strncpy(air1->s_file,air2.s_file, AIR_CONFIG_MAX_PATH_LENGTH);
				strncpy(air1->r_file,air2.r_file, AIR_CONFIG_MAX_PATH_LENGTH);
				strncpy(air1->comment,air2.comment, AIR_CONFIG_MAX_COMMENT_LENGTH);
				strncpy(air1->reserved,air2.reserved, AIR_CONFIG_RESERVED_LENGTH);
				air1->s.bits=air2.s.bits;
				air1->s.x_dim=air2.s.x_dim;
				air1->s.y_dim=air2.s.y_dim;
				air1->s.z_dim=air2.s.z_dim;
				air1->s.x_size=air2.s.x_size;
				air1->s.y_size=air2.s.y_size;
				air1->s.z_size=air2.s.z_size;
				air1->r.bits=air2.r.bits;
				air1->r.x_dim=air2.r.x_dim;
				air1->r.y_dim=air2.r.y_dim;
				air1->r.z_dim=air2.r.z_dim;
				air1->r.x_size=air2.r.x_size;
				air1->r.y_size=air2.r.y_size;
				air1->r.z_size=air2.r.z_size;
				air1->s_hash=air2.s_hash;
				air1->r_hash=air2.r_hash;
				air1->s_volume=air2.s_volume;
				air1->r_volume=air2.r_volume;
				air1->e[0][0]=air2.e[1][0];
				air1->e[0][1]=air2.e[1][1];
				air1->e[0][2]=air2.e[1][2];
				air1->e[0][3]=0.0;
				air1->e[1][0]=air2.e[2][0];
				air1->e[1][1]=air2.e[2][1];
				air1->e[1][2]=air2.e[2][2];
				air1->e[1][3]=0.0;
				air1->e[2][0]=air2.e[3][0];
				air1->e[2][1]=air2.e[3][1];
				air1->e[2][2]=air2.e[3][2];
				air1->e[2][3]=0.0;
				air1->e[3][0]=air2.e[0][0];
				air1->e[3][1]=air2.e[0][1];
				air1->e[3][2]=air2.e[0][2];
				air1->e[3][3]=1.0;
#if(AIR_CONFIG_AUTO_BYTESWAP!=0)
			
				if(air1->s.bits> (unsigned int)sqrt((double)UINT_MAX)){
					/* Pre-swap these values so that they'll be swapped back to their starting values */
					AIR_swapbytes(&(air1->e[0][3]),sizeof(air1->e[0][3])/2,1);
					AIR_swapbytes(&(air1->e[1][3]),sizeof(air1->e[1][3])/2,1);
					AIR_swapbytes(&(air1->e[2][3]),sizeof(air1->e[2][3])/2,1);
					AIR_swapbytes(&(air1->e[3][3]),sizeof(air1->e[3][3])/2,1);
					
					/* Now swap everything */
					swap_Air16(air1);
				}
#endif
				return 0;
			}
		}
		if(fclose(fp)!=0){
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("error closing file %s\n",filename);
			return AIR_CANT_CLOSE_READ_ERROR;
		}
	}

	/* The file could not be read as any known form of AIR file
	 * See if it can be read as a 1st order .warp file
	 */
	{
		struct AIR_Warp air2;
		AIR_Error errcode;
	
		double **e2=AIR_read_airw(filename, &air2, &errcode);
		if(e2!=NULL){
			if(air2.order<2 && (air2.coord==2 || air2.coord==3)){

				strncpy(air1->s_file,air2.s_file, AIR_CONFIG_MAX_PATH_LENGTH);
				strncpy(air1->r_file,air2.r_file, AIR_CONFIG_MAX_PATH_LENGTH);
				strncpy(air1->comment,air2.comment, AIR_CONFIG_MAX_COMMENT_LENGTH);
				strncpy(air1->reserved,air2.reserved, AIR_CONFIG_RESERVED_LENGTH);
				air1->s.bits=air2.s.bits;
				air1->s.x_dim=air2.s.x_dim;
				air1->s.y_dim=air2.s.y_dim;
				air1->s.z_dim=air2.s.z_dim;
				air1->s.x_size=air2.s.x_size;
				air1->s.y_size=air2.s.y_size;
				air1->s.z_size=air2.s.z_size;
				air1->r.bits=air2.r.bits;
				air1->r.x_dim=air2.r.x_dim;
				air1->r.y_dim=air2.r.y_dim;
				air1->r.z_dim=air2.r.z_dim;
				air1->r.x_size=air2.r.x_size;
				air1->r.y_size=air2.r.y_size;
				air1->r.z_size=air2.r.z_size;
				air1->s_hash=air2.s_hash;
				air1->r_hash=air2.r_hash;
				air1->s_volume=air2.s_volume;
				air1->r_volume=air2.r_volume;
				
				air1->e[3][0]=e2[0][0];
				air1->e[3][1]=e2[1][0];
				
				if(air2.coord==3) air1->e[3][2]=e2[2][0];
				else air1->e[3][2]=0.0;
				
				if(air2.order>0){
					
					air1->e[0][0]=e2[0][1];
					air1->e[0][1]=e2[1][1];
					
					if(air2.coord==3) air1->e[0][2]=e2[2][1];
					else air1->e[0][2]=0.0;
					
					air1->e[1][0]=e2[0][2];
					air1->e[1][1]=e2[1][2];
					
					if(air2.coord==3) air1->e[1][2]=e2[2][2];
					else air1->e[1][2]=0.0;
					
					if(air2.coord==3){
						
						air1->e[2][0]=e2[0][3];
						air1->e[2][1]=e2[1][3];
						air1->e[2][2]=e2[2][3];
					}
					else{
						air1->e[2][0]=0.0;
						air1->e[2][1]=0.0;
						air1->e[2][2]=1.0;
					}
				}
				else{
					air1->e[0][0]=1.0;
					air1->e[0][1]=0.0;
					air1->e[0][2]=0.0;
					air1->e[1][0]=0.0;
					air1->e[1][1]=1.0;
					air1->e[1][2]=0.0;
					air1->e[2][0]=0.0;
					air1->e[2][1]=0.0;
					air1->e[2][2]=1.0;
				}
	
				air1->e[0][3]=0.0;
				air1->e[1][3]=0.0;
				air1->e[2][3]=0.0;
				air1->e[3][3]=1.0;
				
				/* Convert to .air file format */
				{
					double pixel_size_s=air1->s.x_size;
					if(air1->s.y_size<pixel_size_s) pixel_size_s=air1->s.y_size;
					if(air1->s.z_size<pixel_size_s) pixel_size_s=air1->s.z_size;
					
					{
						unsigned int i;
						
						for(i=0;i<4;i++){
							air1->e[0][i]/=(air1->s.x_size/pixel_size_s);
							air1->e[1][i]/=(air1->s.y_size/pixel_size_s);
							air1->e[2][i]/=(air1->s.z_size/pixel_size_s);
						}
					}
				}
				AIR_free_2(e2);
				return 0;
			}
			AIR_free_2(e2);
		}
	}
	printf("%s: %d: ",__FILE__,__LINE__);
	printf("file read error for file %s--EOF not at expected location\n",filename);
	return AIR_READ_AIR_FILE_ERROR;
}
