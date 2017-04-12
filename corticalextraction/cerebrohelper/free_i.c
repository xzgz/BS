/* Copyright 1999-2001 Roger P. Woods, M.D. */
/* Modified: 5/22/01 */

/*
 * These routines free memory
 *
 * Note check for null pointer before proceding to next lower level
 */

#include "AIR.h"

void AIR_free_7i(int *******pixels)
{
	if(*pixels) AIR_free_6i(*pixels);
	free(pixels);
}
void AIR_free_6i(int ******pixels)
{
	if(*pixels) AIR_free_5i(*pixels);
	free(pixels);
}
void AIR_free_5i(int *****pixels)
{
	if(*pixels) AIR_free_4i(*pixels);
	free(pixels);
}
void AIR_free_4i(int ****pixels)
{
	if(*pixels) AIR_free_3i(*pixels);
	free(pixels);
}
void AIR_free_3i(int ***pixels)
{
	if(*pixels) AIR_free_2i(*pixels);
	free(pixels);
}
void AIR_free_2i(int **pixels)
{
	if(*pixels) AIR_free_1i(*pixels);
	free(pixels);
}
void AIR_free_1i(int *pixels)
{
	free(pixels);
}
