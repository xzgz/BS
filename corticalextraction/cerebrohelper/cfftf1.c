/* Copyright 1995-2001 Roger P. Woods, M.D. */
/* Modified 5/22/01 */

/*
 *	N is the length of the complex sequence C
 *	C is a complex array of length N that contains the sequence to transform
 *	CH is a work array of length 2*N
 * 	WA is a work array of length 2*N
 *	IFAC is an integer work array of length at least 15
 *
 *	WA and IFAC must be initialized using a call to cffti1(n,wa,ifac) before calling this routine
 *	Multiple calls to cfftf1 and cfftb1 can be made with a single initialization so long as N is constant
 *
 * A call to cfftf1 followed by a call to cfftb1 will multiply the sequence by N
 *
 */

#include "AIR.h"

void AIR_cfftf1(const unsigned int n, double *c, double *ch, const double *wa, const unsigned int *ifac)

{	
	unsigned int nf=ifac[1];
	unsigned int na=0;
	unsigned int l1=1;
	unsigned int iw=0;
	{
		unsigned int k1;
		
		for(k1=1;k1<=nf;k1++){
			unsigned int ip=ifac[k1+1];
			unsigned int l2=ip*l1;
			unsigned int ido=n/l2;
			unsigned int idot=ido+ido;
			unsigned int idl1=idot*l1;
			if(ip==4){
				unsigned int ix2=iw+idot;
				unsigned int ix3=ix2+idot;
				if(na==0){
					AIR_passf4(idot,l1,c,ch,&wa[iw],&wa[ix2],&wa[ix3]);
				}
				else{
					AIR_passf4(idot,l1,ch,c,&wa[iw],&wa[ix2],&wa[ix3]);
				}
				na=1-na;
			}
			else if(ip==2){
				if(na==0){
					AIR_passf2(idot,l1,c,ch,&wa[iw]);
				}
				else{
					AIR_passf2(idot,l1,ch,c,&wa[iw]);
				}
				na=1-na;
			}
			else if(ip==3){
				unsigned int ix2=iw+idot;
				if(na==0){
					AIR_passf3(idot,l1,c,ch,&wa[iw],&wa[ix2]);
				}
				else{
					AIR_passf3(idot,l1,ch,c,&wa[iw],&wa[ix2]);
				}
				na=1-na;
			}
			else if (ip==5){
				unsigned int ix2=iw+idot;
				unsigned int ix3=ix2+idot;
				unsigned int ix4=ix3+idot;
				if(na==0){
					AIR_passf5(idot,l1,c,ch,&wa[iw],&wa[ix2],&wa[ix3],&wa[ix4]);
				}
				else{
					AIR_passf5(idot,l1,ch,c,&wa[iw],&wa[ix2],&wa[ix3],&wa[ix4]);
				}
				na=1-na;
			}
			else{
				unsigned int nac;
				
				if(na==0){
					AIR_passf(&nac,idot,ip,l1,idl1,c,c,c,ch,ch,&wa[iw]);
				}
				else{
					AIR_passf(&nac,idot,ip,l1,idl1,ch,ch,ch,c,c,&wa[iw]);
				}
				if(nac!=0) na=1-na;
			}

			l1=l2;
			iw+=(ip-1)*idot;
		}
	}
	if(na==0) return;
	{
		unsigned int i;
		
		for(i=0;i<2*n;i++){
			c[i]=ch[i];
		}
	}
	return;
}
