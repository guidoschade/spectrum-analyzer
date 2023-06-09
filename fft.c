#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double	pii, pi2, p7, p7two, c22, s22;

int cfast(float * b, long * n);
int cfsst(register float	*b, long	*n);

int cford1(int m, register float *b);
int cford2(int m, register float *b);

int cfr2tr(register long off,register float	*b0, register float *b1);
int cfr4tr(long off, long nn, float	*b0,float *b1,float *b2,float *b3,float *b4,float *b5,float *b6,float *b7);
int cfr4syn(float off,float nn, float	*b0,float	*b1, float *b2, float *b3, float *b4, float *b5, float *b6, float *b7);

// DFT - discrete
void discreteft(float * b, int n)
{
  int y,z;
  float pi2=3.1415927*2, * cosin = NULL, * sinus = NULL, tnum, tmul;

  sinus = (float *) malloc((n+1)*sizeof(float));
  cosin = (float *) malloc((n+1)*sizeof(float));

  // clear arrays
  for (y=1;y<n;y++) {cosin[y]=0; cosin[y]=0;};
  cosin[0]=1;

  // calculate spectrum
  for (y=1;y<n;y++)
  {
   tmul=(pi2*y/(float)n);
   for (z=0;z<n;z++)
   { tnum=tmul*z; cosin[y]=cosin[y]+b[z]*cos(tnum); sinus[y]=sinus[y]+b[z]*sin(tnum); }
  }
  z=0; // change order in spectrum, was kind of mirrored before
  for (y=0;y<n/2;y++)
  {
    b[z]=cosin[y]; b[++z]=sinus[y];
    b[z]=cosin[n-y]; b[++z]=sinus[n-y];
  }
  //b[y]=(float)sqrt((cosin[y]*cosin[y])+(sinus[y]*sinus[y])); // do not calculate real value here
  free(sinus); free(cosin);
}



/* -----------------------------------------------------------------
				cfast.c

This is the FFT subroutine fast.f from the IEEE library recoded in C.
	It takes a pointer to a real vector b[i], for i = 0, 1, ...,
	N-1, and replaces it with its finite discrete fourier transform.

-------------------------------------------------------------------*/

int cfast(float * b, long * n)
{

/* The DC term is returned in location b[0] with b[1] set to 0.
	Thereafter, the i'th harmonic is returned as a complex
	number stored as b[2*i] + j b[2*i+1].  The N/2 harmonic
	is returned in b[N] with b[N+1] set to 0.  Hence, b must
	be dimensioned to size N+2.  The subroutine is called as
	fast(b,N) where N=2**M and b is the real array described
	above.
*/

  register float	pi8, tmp;

	register int	M;
  long  N = *n;

	register long	i, Nt, off;

	pii = 4. * atan(1.);
	pi2 = pii * 2.;
	pi8 = pii / 8.;
	p7 = 1. / sqrt(2.);
	p7two = 2. * p7;
	c22 = cos(pi8);
	s22 = sin(pi8);

/* determine M */

	for (i=1, Nt=2; i<20; i++,Nt *= 2) if (Nt==N) break;
	M = i;
	if (M==20){
		fprintf(stderr,"fast: N is not an allowable power of two\n");
		exit(1);
	}

/* do a radix 2 iteration first if one is required */

	Nt = 1;
	if (M%2 != 0){
		Nt = 2;
		off = N / Nt;
		cfr2tr(off,b,b+off);
	}

/* perform radix 4 iterations */

	if (M != 1) for (i=1; i<=M/2; i++){
		Nt *= 4;
		off = N / Nt;
		cfr4tr(off,Nt,b,b+off,b+2*off,b+3*off,b,b+off,b+2*off,b+3*off);
	}

/* perform in-place reordering */

	cford1(M,b);
	cford2(M,b);

	tmp = b[1];
	b[1] = 0.;
	b[N] = tmp;
	b[N+1] = 0.;

	for (i=3; i<N; i+=2) b[i] *= -1.;

	return(0);
}

/* -----------------------------------------------------------------
				cfsst.c

This is the FFT subroutine fsst.f from the IEEE library recoded in C.
	It takes a pointer to a vector b[i], for i = 0, 1, ..., N-1,
	and replaces it with its inverse DFT assuming real output.

-------------------------------------------------------------------*/

int cfsst(register float	*b, long	*n)
{

/* This subroutine synthesizes the real vector b[k] for k=0, 1,
	..., N-1 from the fourier coefficients stored in the b
	array of size N+2.  The DC term is in location b[0] with
	b[1] equal to 0.  The i'th harmonic is a complex number
	stored as b[2*i] + j b[2*i+1].  The N/2 harmonic is in
	b[N] with b[N+1] equal to 0. The subroutine is called as
	fsst(b,N) where N=2**M and b is the real array described
	above.
*/

	float	pi8, invN;

	register int	M;
	long	N = *n;

	register long	i, Nt, off;

	pii = 4. * atan(1.);
	pi2 = pii * 2.;
	pi8 = pii / 8.;
	p7 = 1. / sqrt(2.);
	p7two = 2. * p7;
	c22 = cos(pi8);
	s22 = sin(pi8);

/* determine M */

	for (i=1, Nt=2; i<20; i++,Nt *= 2) if (Nt==N) break;
	M = i;
	if (M==20){
		fprintf(stderr,"fast: N is not an allowable power of two\n");
		exit(1);
	}

	b[1] = b[N];

	for (i=3; i<N; i+=2) b[i] *= -1.;

/* scale the input by N */

	invN = 1. / N;
	for (i=0; i<N; i++) b[i] *= invN;

/* scramble the inputs */

	cford2(M,b);
	cford1(M,b);

/* perform radix 4 iterations */

	Nt = 4*N;
	if (M != 1) for (i=1; i<=M/2; i++){
		Nt /= 4;
		off = N / Nt;
		cfr4syn(off,Nt,b,b+off,b+2*off,b+3*off,b,b+off,b+2*off,b+3*off);
	}

/* do a radix 2 iteration if one is required */

	if (M%2 != 0){
		off = N / 2;
		cfr2tr(off,b,b+off);
	}

	return(1);
}

/*---------------------------------------
	cford1.c in-place reordering subroutine
#--------------------------------------*/
int cford1(int m, register float *b)
{
	register long k, kl, n, j;
	register float t;

	k = 4;
	kl = 2;
	n = (long)pow(2.,(float)m);
	for (j = 4; j <= n; j+=2) {
		if (k>j) {
			t = b[j-1];
			b[j-1] = b[k-1];
			b[k-1] = t;
		}
		k = k-2;
		if (k<=kl) {
			k = 2*j;
			kl = j;
		}
	}

	return(1);
}

/*-------------------------------
	cford2.c in-place reordering subroutine
-------------------------------*/
int cford2(int m, register float *b)
{
	float	t;
	register long ij, ji, k, n;

	long	jj1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12,
		j13, j14, j15, j16, j17, j18, 
		l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12,
		l13, l14, l15, l16, l17, l18, l19,  l[19];

	n = (long)pow(2.,(float)m);
	l[0] = n;
	for (k = 2; k <= m; k++) l[k-1] = l[k-2]/2;
	for (k = m; k <= 18; k++) l[k] = 2;
	ij = 2;

	l1 = l[18];
	l2 = l[17];
	l3 = l[16];
	l4 = l[15];
	l5 = l[14];
	l6 = l[13];
	l7 = l[12];
	l8 = l[11];
	l9 = l[10];
	l10 = l[9];
	l11 = l[8];
	l12 = l[7];
	l13 = l[6];
	l14 = l[5];
	l15 = l[4];
	l16 = l[3];
	l17 = l[2];
	l18 = l[1];
	l19 = l[0];


	/* for (jj1=2; jj1<=l1; jj1+=2){ */
	jj1 = 2;
loop1:	 /* for (j2=jj1; j2<=l2; j2+=l1){ */
	 j2 = jj1;
loop2:	  /* for (j3=j2; j3<=l3; j3+=l2){ */
	  j3 = j2;
loop3:	   /* for (j4=j3; j4<=l4; j4+=l3){ */
	   j4 = j3;
loop4:	    /* for (j5=j4; j5<=l5; j5+=l4){ */
	    j5 = j4;
loop5:	     /* for (j6=j5; j6<=l6; j6+=l5){ */
	     j6 = j5;
loop6:	      /* for (j7=j6; j7<=l7; j7+=l6){ */
	      j7 = j6;
loop7:	       /* for (j8=j7; j8<=l8; j8+=l7){ */
	       j8 = j7;
loop8:		/* for (j9=j8; j9<=l9; j9+=l8){ */
		j9 = j8;
loop9:		 /* for (j10=j9; j10<=l10; j10+=l9){ */
		 j10 = j9;
loop10:		  /* for (j11=j10; j11<=l11; j11+=l10){ */
		  j11 = j10;
loop11:		   /* for (j12=j11; j12<=l12; j12+=l11){ */
		   j12 = j11;
loop12:		    /* for (j13=j12; j13<=l13; j13+=l12){ */
		    j13 = j12;
loop13:		     /* for (j14=j13; j14<=l14; j14+=l13){ */
		     j14 = j13;
loop14:		      for (j15=j14; j15<=l15; j15+=l14){
		       for (j16=j15; j16<=l16; j16+=l15){
			for (j17=j16; j17<=l17; j17+=l16){
			 for (j18=j17; j18<=l18; j18+=l17){
			  for (ji=j18; ji<=l19; ji+=l18){
		 	   if (ij<ji) {
			    t = b[ij-2];
			    b[ij-2] = b[ji-2];
			    b[ji-2] = t;
			    t = b[ij-1];
			    b[ij-1] = b[ji-1];
			    b[ji-1] = t;
			   }
			   ij = ij+2;
			  }
			 }
			}
		       }
		      }
		     j14 += l13;
		     if (j14 <= l14) goto loop14;
		     /* } */
		    j13 += l12;
		    if (j13 <= l13) goto loop13;
		    /* } */
		   j12 += l11;
		   if (j12 <= l12) goto loop12;
		   /* } */
		  j11 += l10;
		  if (j11 <= l11) goto loop11;
		  /* } */
		 j10 += l9;
		 if (j10 <= l10) goto loop10;
		 /* } */
		j9 += l8;
		if (j9 <= l9) goto loop9;
		/* } */
	       j8 += l7;
	       if (j8 <= l8) goto loop8;
	       /* } */
	      j7 += l6;
	      if (j7 <= l7) goto loop7;
	      /* } */
	     j6 += l5;
	     if (j6 <= l6) goto loop6;
	     /* } */
	    j5 += l4;
	    if (j5 <= l5) goto loop5;
	    /* } */
	   j4 += l3;
	   if (j4 <= l4) goto loop4;
	   /* } */
	  j3 += l2;
	  if (j3 <= l3) goto loop3;
	  /* } */
	 j2 += l1;
	 if (j2 <= l2) goto loop2;
	 /* } */
	jj1 += 2;
	if (jj1 <= l1) goto loop1;

	return(0);
}

/*---------------------------
	cfr2tr.c Radix 2 iteration subroutine.
---------------------------*/

int cfr2tr(register long off,register float	*b0, register float *b1)
{
	register long	i;
	register float	tmp;

	for (i=0; i<off; i++){
		tmp = b0[i] + b1[i];
		b1[i] = b0[i] - b1[i];
		b0[i] = tmp;
	}

	return(0);
}


/*---------------------------
	cfr4tr.c Radix 4 iteration subroutine.
---------------------------*/
int cfr4tr(long off, long nn, float	*b0,float *b1,float *b2,float *b3,float *b4,float *b5,float *b6,float *b7)
{
	register long    i, j, k;
	long	off4, jj0, jlast, k0, kl, ji, jl, jr,
		jj1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12,
		j13, j14, j15, j16, j17, j18, jthet, th2,
		l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12,
		l13, l14, l15, l16, l17, l18, l19,  l[19];

	float	piovn, arg, c1, c2, c3, s1, s2, s3, r1, r5, pr, pi,
		t0, t1, t2, t3, t4, t5, t6, t7;

/* jthet is a reversed binary counter; jr steps two at a time to
	locate real parts of intermediate results, and ji locates
	the imaginary part corresponding to jr.
*/

	l[0] = nn / 4;
	for (i=1; i<19; i++){
		if (l[i-1] < 2) l[i-1] = 2;
		else if (l[i-1] != 2){
			l[i] = l[i-1] / 2;
			continue;
		}
		l[i] = 2;
	}

	l1 = l[18];
	l2 = l[17];
	l3 = l[16];
	l4 = l[15];
	l5 = l[14];
	l6 = l[13];
	l7 = l[12];
	l8 = l[11];
	l9 = l[10];
	l10 = l[9];
	l11 = l[8];
	l12 = l[7];
	l13 = l[6];
	l14 = l[5];
	l15 = l[4];
	l16 = l[3];
	l17 = l[2];
	l18 = l[1];
	l19 = l[0];

	piovn = pii / nn;
	ji = 3;
	jl = 2;
	jr = 2;

	/* for (j1=2; jj1<=l1; jj1+=2){ */
	jj1 = 2;
loop1:	 /* for (j2=jj1; j2<=l2; j2+=l1){ */
	 j2 = jj1;
loop2:	  /* for (j3=j2; j3<=l3; j3+=l2){ */
	  j3 = j2;
loop3:	   /* for (j4=j3; j4<=l4; j4+=l3){ */
	   j4 = j3;
loop4:	    /* for (j5=j4; j5<=l5; j5+=l4){ */
	    j5 = j4;
loop5:	     /* for (j6=j5; j6<=l6; j6+=l5){ */
	     j6 = j5;
loop6:	      /* for (j7=j6; j7<=l7; j7+=l6){ */
	      j7 = j6;
loop7:	       /* for (j8=j7; j8<=l8; j8+=l7){ */
	       j8 = j7;
loop8:		/* for (j9=j8; j9<=l9; j9+=l8){ */
		j9 = j8;
loop9:		 /* for (j10=j9; j10<=l10; j10+=l9){ */
		 j10 = j9;
loop10:		  /* for (j11=j10; j11<=l11; j11+=l10){ */
		  j11 = j10;
loop11:		   /* for (j12=j11; j12<=l12; j12+=l11){ */
		   j12 = j11;
loop12:		    /* for (j13=j12; j13<=l13; j13+=l12){ */
		    j13 = j12;
loop13:		     /* for (j14=j13; j14<=l14; j14+=l13){ */
		     j14 = j13;
loop14:		      for (j15=j14; j15<=l15; j15+=l14){
		       for (j16=j15; j16<=l16; j16+=l15){
			for (j17=j16; j17<=l17; j17+=l16){
			 for (j18=j17; j18<=l18; j18+=l17){
			  for (jthet=j18; jthet<=l19; jthet+=l18){
			   th2 = jthet - 2;
			   if (th2 <= 0){
			    for (i=0; i<off; i++){
			     t0 = b0[i] + b2[i];
			     t1 = b1[i] + b3[i];
			     b2[i] = b0[i] - b2[i];
			     b3[i] = b1[i] - b3[i];
			     b0[i] = t0 + t1;
			     b1[i] = t0 - t1;
			    }
			    if (nn > 4){
			     k0 = 4 * off;
			     kl = k0 + off;
			     for (i=k0; i<kl; i++){
			      pr = p7 * (b1[i] - b3[i]);
			      pi = p7 * (b1[i] + b3[i]);
			      b3[i] = b2[i] + pi;
			      b1[i] = pi - b2[i];
			      b2[i] = b0[i] - pr;
			      b0[i] = b0[i] + pr;
			     }
			    }
			   }
			   else{
			    arg = th2 * piovn;
			    c1 = cos(arg);
			    s1 = sin(arg);
			    c2 = c1*c1 - s1*s1;
			    s2 = c1*s1 + c1*s1;
			    c3 = c1*c2 - s1*s2;
			    s3 = c2*s1 + s2*c1;
			    off4 = 4 * off;
			    jj0 = jr * off4;
			    k0 = ji * off4;
			    jlast = jj0 + off;
			    for (j=jj0; j<jlast; j++){
			     k = k0 + j - jj0;
			     r1 = b1[j]*c1 - b5[k]*s1;
			     r5 = b1[j]*s1 + b5[k]*c1;
			     t2 = b2[j]*c2 - b6[k]*s2;
			     t6 = b2[j]*s2 + b6[k]*c2;
			     t3 = b3[j]*c3 - b7[k]*s3;
			     t7 = b3[j]*s3 + b7[k]*c3;
			     t0 = b0[j]+t2;
			     t4 = b4[k]+t6;
			     t2 = b0[j]-t2;
			     t6 = b4[k]-t6;
			     t1 = r1+t3;
			     t5 = r5+t7;
			     t3 = r1-t3;
			     t7 = r5-t7;
			     b0[j] = t0+t1;
			     b7[k] = t4+t5;
			     b6[k] = t0-t1;
			     b1[j] = t5-t4;
			     b2[j] = t2-t7;
			     b5[k] = t6+t3;
			     b4[k] = t2+t7;
			     b3[j] = t3-t6;
			    }
			    jr += 2;
			    ji -= 2;
			    if (ji <= jl){
			     ji = 2*jr - 1;
			     jl = jr;
			    }
			   } 
			  }
			 }
			}
		       }
		      }
		     j14 += l13;
		     if (j14 <= l14) goto loop14;
		     /* } */
		    j13 += l12;
		    if (j13 <= l13) goto loop13;
		    /* } */
		   j12 += l11;
		   if (j12 <= l12) goto loop12;
		   /* } */
		  j11 += l10;
		  if (j11 <= l11) goto loop11;
		  /* } */
		 j10 += l9;
		 if (j10 <= l10) goto loop10;
		 /* } */
		j9 += l8;
		if (j9 <= l9) goto loop9;
		/* } */
	       j8 += l7;
	       if (j8 <= l8) goto loop8;
	       /* } */
	      j7 += l6;
	      if (j7 <= l7) goto loop7;
	      /* } */
	     j6 += l5;
	     if (j6 <= l6) goto loop6;
	     /* } */
	    j5 += l4;
	    if (j5 <= l5) goto loop5;
	    /* } */
	   j4 += l3;
	   if (j4 <= l4) goto loop4;
	   /* } */
	  j3 += l2;
	  if (j3 <= l3) goto loop3;
	  /* } */
	 j2 += l1;
	 if (j2 <= l2) goto loop2;
	 /* } */
	jj1 += 2;
	if (jj1 <= l1) goto loop1;

	return(0);
}


/*-------------------------------
	cfr4syn.c radix 4 synthesis
-------------------------------*/
int cfr4syn(float off,float nn, float	*b0,float	*b1, float *b2, float *b3, float *b4, float *b5, float *b6, float *b7)
{
	float	piovn, arg, c1, c2, c3, s1, s2, s3,
		t0, t1, t2, t3, t4, t5, t6, t7;

	register long i, j, k;

	long	off4, jj0, jlast, k0, kl, ji, jl, jr,
		jj1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12,
		j13, j14, j15, j16, j17, j18, jthet, th2,
		l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12,
		l13, l14, l15, l16, l17, l18, l19,
		l[19];

/* jthet is a reversed binary counter; jr steps two at a time to
	locate real parts of intermediate results, and ji locates
	the imaginary part corresponding to jr.
*/

	l[0] = (long) nn/4;
	for (i=1; i<19; i++){
		if (l[i-1]<2) l[i-1] = 2;
		else if (l[i-1]!=2) {
			l[i] = l[i-1]/2;
			continue;
		}
		l[i] = 2;
	}

	l1 = l[18];
	l2 = l[17];
	l3 = l[16];
	l4 = l[15];
	l5 = l[14];
	l6 = l[13];
	l7 = l[12];
	l8 = l[11];
	l9 = l[10];
	l10 = l[9];
	l11 = l[8];
	l12 = l[7];
	l13 = l[6];
	l14 = l[5];
	l15 = l[4];
	l16 = l[3];
	l17 = l[2];
	l18 = l[1];
	l19 = l[0];

	piovn = pii / nn;
	ji = 3;
	jl = 2;
	jr = 2;

	/* for (jj1=2; jj1<=l1; jj1+=2){ */
	jj1 = 2;
loop1:	 /* for (j2=jj1; j2<=l2; j2+=l1){ */
	 j2 = jj1;
loop2:	  /* for (j3=j2; j3<=l3; j3+=l2){ */
	  j3 = j2;
loop3:	   /* for (j4=j3; j4<=l4; j4+=l3){ */
	   j4 = j3;
loop4:	    /* for (j5=j4; j5<=l5; j5+=l4){ */
	    j5 = j4;
loop5:	     /* for (j6=j5; j6<=l6; j6+=l5){ */
	     j6 = j5;
loop6:	      /* for (j7=j6; j7<=l7; j7+=l6){ */
	      j7 = j6;
loop7:	       /* for (j8=j7; j8<=l8; j8+=l7){ */
	       j8 = j7;
loop8:		/* for (j9=j8; j9<=l9; j9+=l8){ */
		j9 = j8;
loop9:		 /* for (j10=j9; j10<=l10; j10+=l9){ */
		 j10 = j9;
loop10:		  /* for (j11=j10; j11<=l11; j11+=l10){ */
		  j11 = j10;
loop11:		   /* for (j12=j11; j12<=l12; j12+=l11){ */
		   j12 = j11;
loop12:		    /* for (j13=j12; j13<=l13; j13+=l12){ */
		    j13 = j12;
loop13:		     /* for (j14=j13; j14<=l14; j14+=l13){ */
		     j14 = j13;
loop14:		      for (j15=j14; j15<=l15; j15+=l14){
		       for (j16=j15; j16<=l16; j16+=l15){
			for (j17=j16; j17<=l17; j17+=l16){
			 for (j18=j17; j18<=l18; j18+=l17){
			  for (jthet=j18; jthet<=l19; jthet+=l18){
			   th2 = jthet - 2;
			   if (th2 > 0) {
			    arg = th2 * piovn;
			    c1 = cos(arg);
			    s1 = -sin(arg);
			    c2 = c1*c1 - s1*s1;
			    s2 = c1*s1 + c1*s1;
			    c3 = c1*c2 - s1*s2;
			    s3 = c2*s1 + s2*c1;
			    off4 = (long) (4 * off);
			    jj0 = jr * off4;
			    k0 = ji * off4;
			    jlast = (long) (jj0 + off);
			    for (j=jj0; j<jlast; j++){
			     k = k0+j-jj0;
			     t0 = b0[j] + b6[k];
			     t1 = b7[k] - b1[j];
			     t2 = b0[j] - b6[k];
			     t3 = b7[k] + b1[j];
			     t4 = b2[j] + b4[k];
			     t5 = b5[k] - b3[j];
			     t6 = b5[k] + b3[j];
			     t7 = b4[k] - b2[j];
			     b0[j] = t0 + t4;
			     b4[k] = t1 + t5;
			     b1[j] = (t2+t6)*c1 - (t3+t7)*s1;
			     b5[k] = (t2+t6)*s1 + (t3+t7)*c1;
			     b2[j] = (t0-t4)*c2 - (t1-t5)*s2;
			     b6[k] = (t0-t4)*s2 + (t1-t5)*c2;
			     b3[j] = (t2-t6)*c3 - (t3-t7)*s3;
			     b7[k] = (t2-t6)*s3 + (t3-t7)*c3;
			    }
			    jr = jr + 2;
			    ji = ji - 2;
			    if (ji <= jl){
			     ji = 2*jr - 1;
			     jl = jr;
			    }
			   }
			   else {
			    for (i=0; i<off; i++){
			     t0 = b0[i] + b1[i];
			     t1 = b0[i] - b1[i];
			     t2 = b2[i] * 2.0;
			     t3 = b3[i] * 2.0;
			     b0[i] = t0 + t2;
			     b2[i] = t0 - t2;
			     b1[i] = t1 + t3;
			     b3[i] = t1 - t3;
			    }
			    if (nn>4) {
			     k0 = (long)(4*off);
			     kl = (long)(k0 + off);
			     for (i=k0; i<kl; i++) {
			      t2 = b0[i] - b2[i];
			      t3 = b1[i] + b3[i];
			      b0[i] = (b0[i] + b2[i]) * 2.0;
			      b2[i] = (b3[i] - b1[i]) * 2.0;
			      b1[i] = (t2 + t3) * p7two;
			      b3[i] = (t3 - t2) * p7two;
			     }
			    }
			   }
			  }
			 }
			}
		       }
		      }
		     j14 += l13;
		     if (j14 <= l14) goto loop14;
		     /* } */
		    j13 += l12;
		    if (j13 <= l13) goto loop13;
		    /* } */
		   j12 += l11;
		   if (j12 <= l12) goto loop12;
		   /* } */
		  j11 += l10;
		  if (j11 <= l11) goto loop11;
		  /* } */
		 j10 += l9;
		 if (j10 <= l10) goto loop10;
		 /* } */
		j9 += l8;
		if (j9 <= l9) goto loop9;
		/* } */
	       j8 += l7;
	       if (j8 <= l8) goto loop8;
	       /* } */
	      j7 += l6;
	      if (j7 <= l7) goto loop7;
	      /* } */
	     j6 += l5;
	     if (j6 <= l6) goto loop6;
	     /* } */
	    j5 += l4;
	    if (j5 <= l5) goto loop5;
	    /* } */
	   j4 += l3;
	   if (j4 <= l4) goto loop4;
	   /* } */
	  j3 += l2;
	  if (j3 <= l3) goto loop3;
	  /* } */
	 j2 += l1;
	 if (j2 <= l2) goto loop2;
	 /* } */
	jj1 += 2;
	if (jj1 <= l1) goto loop1;
	/* } */

	return(0);
}

