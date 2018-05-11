#include "rlhafnian.h"
#include <stdio.h>
void evals(double z[], double complex vals[], int n){
  /*
    Calculates the eigenvalues of the complex n x n matrix z
    returns the eigenvalues in the array vals
    For the calculation of the eigenvalues it uses lapacke
  */
  lapack_int info;
  
  char jobvs='N';
  char sort ='N';

  lapack_int lda=n;
  lapack_int sdim=0;
  lapack_int ldvs=n;
  double wr[n];
  double wi[n];
    
  info= LAPACKE_dgees(LAPACK_ROW_MAJOR, jobvs, sort,
		      NULL, n, &(z[0]), lda,
		      &sdim, &(wr[0]), &(wi[0]),
		      NULL, ldvs );
  assert(info==0);
  for(int i=0;i<n;i++){
	  vals[i] = wr[i]+I*wi[i];
  }
  /*
  fprintf(stdout,"\n");
  for(int i=0;i<n;i++){
    fprintf(stdout,"%lf %lf\n",creal(vals[i]),cimag(vals[i]));
  }
  */
  
}





void powtrace(double z[], int n, double traces[], int l){
  /*
    given a complex matrix z of dimensions n x n
    it calculates traces[k] = tr(z^j) for 1 <= j <= l
    It does it by first finding the eigenvalues of the matrix z
    using the orutine evals
   */
  double complex vals[n];
  double complex pvals[n];
  
  evals(z,vals,n);
  int i,j;
  double complex sum;
  for(j=0;j<n;j++){
    pvals[j]=vals[j];
  }
  for(i=0;i<l;i++){
    sum=0.0;
    for(j=0;j<n;j++){
      sum+=pvals[j];
    }
    traces[i]=creal(sum);
    for(j=0;j<n;j++){
      pvals[j]=pvals[j]*vals[j];
    } 
  }
}

void dec2bin(char *dst, unsigned long long int x, sint len)
{
  /*
  Convert decimal number in  x to character vector dst of length len 
  representing binary number
  */
  
  char i; // this variable cannot be unsigned
  for (i = len - 1; i >= 0; --i)
    *dst++ = x >> i & 1;
}


sint find2(char *dst, sint len, sint *pos){
  /* Given a string of length len
     it finds in which positions it has a one
     and stores its position i, as 2*i and 2*i+1 in consecutive slots
     of the array pos.
     It also returns (twice) the number of ones in array dst
  */
  sint sum=0;
  sint j=0;
  for(sint i=0;i<len;i++){
    if(1==dst[i]){
      sum++;
      pos[2*j]=2*i;
      pos[2*j+1]=2*i+1;
      j++;
    }
  }
  return 2*sum;
}

double do_chunk(double mat[], int n, unsigned long long int X, unsigned long long int chunksize){
  /*
    This function calculates adds parts X to X+chunksize of Cygan and Pilipczuk formula for the 
    Hafnian of matrix mat
   */

  double res=0.0;
  for(unsigned long long int x=X;x<X+chunksize;x++){

    double summand=0.0;
    sint m=n/2;
    int i,j,k;
    double factor;
    double powfactor;
    
    
    char dst[m];
    sint pos[n];
    char sum;
    dec2bin(dst,x,m);
    sum=find2(dst,m,pos);
    double B[sum*sum];    
    for(i=0;i<sum;i++){
      for(j=0;j<sum;j++){
	B[i*sum+j] =  mat[pos[i]*n+((pos[j])^1)];
      }
    }
    double traces[m];
    powtrace(B,sum,traces,m);
    char cnt;
    sint cntindex;
    cnt=1;
    cntindex=0;
    double comb[2][m+1];
    for(i=0;i<n/2+1;i++){
      comb[0][i]=0.0;
    }
    comb[0][0]=1.0;
    
    for(i=1;i<=n/2;i++){
      factor=traces[i-1]/(2*i);
      powfactor=1.0;
      //the following line is significantly different than in Octave
      //It basically reassings  the first column of comb to the second and viceversa
      cnt=-cnt;
      cntindex=(1+cnt)/2;
      for(j=0;j<n/2+1;j++){
	comb[1-cntindex][j]=comb[cntindex][j];
      }
      for(j=1;j<=(n/(2*i));j++){
	powfactor=powfactor*factor/j;
	for(k=i*j+1;k<=n/2+1;k++){
	  comb[1-cntindex][k-1]=comb[1-cntindex][k-1]+comb[cntindex][k-i*j-1]*powfactor;
	}
      }
    }
    if(((sum/2)%2) == (n/2 %2)){
      summand= comb[1-cntindex][n/2];
    }
    else{
      summand=- comb[1-cntindex][n/2];
    }
    res+=summand;
  }
  
  return res; 
}

void haf(double mat[], int n, double res[]){
  /*
    This is a wrapper for the function hafnian. Instead of returning the Hafnian of the matrix
    mat by value it does by reference with two doubles for the real an imaginary respectively.
  */
  telem result=hafnian(mat,n);
  res[0]=creal(result);
  res[1]=cimag(result);
}

void dhaf(double *mat, int n, double *res){
  haf(mat, n, res);
}


telem hafnian(double *mat, int n){
  /*
    Add all the terms necessary to calculate the Hafnian of the matrix mat of size n
    It additionally uses open MP to parallelize the summation
  */
  assert(n%2==0);
  sint m=n/2;
  sint mm=m/2;
  telem res=0.0;
  unsigned long long int pow1=((unsigned long long int)pow(2.0, (double)m)) ;//-1;
  unsigned long long int workers=((unsigned long long int)pow(2.0, (double)mm));
  workers=MIN(workers,pow1);
  unsigned long long int chunksize=pow1/workers;

#pragma omp parallel
  {
    telem summand;
#pragma omp for
    for(unsigned long long int X=1;X<=pow1;X+=chunksize){
      summand=do_chunk(mat,n,X,chunksize);      
#pragma omp critical  
    res+=summand;
    }//end for X
  }
  return res;
}

telem dhafnian(double *mat, int n){
    return hafnian((double *)mat, n);
}