// x1 and x2 may alias to each other. If no-aliasing assumed, the loop can be parallelized.
#include <omp.h> 

void foo(double *x,int jp,int begin,int end,double rh1)
{
  double *x1;
  double *x2;
  x1 = x;
  x2 = x1 + jp;
  
#pragma omp parallel for firstprivate (end,rh1)
  for (int i = begin; i <= end - 1; i += 1) {
    x1[i] += rh1;
    x2[i] -= rh1;
  }
}
