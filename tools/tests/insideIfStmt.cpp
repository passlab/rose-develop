//Test case for nodal accumulation pattern buried inside if statement
// 
//
// A positive test case: with a pattern to be matched
void foo(double* x, int jp, int kp, int begin, int end, double rh1)
{
  //Condition 1: pointer declaration, 4 or 8 pointers
  double * x1, *x2, *x3, *x4; 

  //Condition 2:  pointer initialization, using other pointers on rhs
  x1 = x;
  x2 = x +1; 
  x3 = x1 + jp; 
  x4 = x1 + kp; 

  //Condition 3:  A regular loop or a RAJA loop
  for (int i = begin; i< end; i++)
  {
    if (i%2==0)
    {
      // Condition 4: accumulation pattern: lhs accum-op rhs
      // lhs : array element access x[i]: x is pointer type, i is loop index 
      // rhs: a scalar double type
      // accum-op:   +=, -=, *=, /=, MIN (), MAX() 
      x1[i] += rh1; 
      x2[i] -= rh1; 
      x3[i] *= rh1; 
      x4[i] /= rh1; 
    }
  } 
}

//Test case for nodal accumulation pattern
// using RAJA::for_all() as a loop
//
namespace RAJA
{
  typedef int Index_type;
  // new specialized policy

 // input code , template 1
  template < typename EXEC_POLICY_T, typename LOOP_BODY >
    void forall ( Index_type begin, Index_type end, LOOP_BODY loop_body)
    {
      forall ( EXEC_POLICY_T(), begin, end, loop_body );
    }
  // some prebuilt policies
  // the type for specialization

  struct seq_exec
  {
  }
  ;
  // Some prebuilt specialization for sequential and parallel executions
  template < typename LOOP_BODY >
    void forall ( seq_exec, Index_type begin, Index_type end, LOOP_BODY loop_body )
    {
      ;
#pragma novector
      for ( Index_type ii = begin; ii < end; ++ ii ) {
        loop_body ( ii );
      }
      ;
    }

  // end namespace
}


void foo2(double* x, int jp, int kp, RAJA::Index_type begin, RAJA::Index_type end, double rh1)
{
   //Condition 1: pointer declaration, 4 or 8 pointers
   double * x1, *x2, *x3, *x4;

   //Condition 2:  pointer initialization, using other pointers on rhs
   x1 = x;
   x2 = x +1;
   x3 = x1 + jp;
   x4 = x1 + kp;

   //Condition 3:  A regular loop or a RAJA loop
   RAJA::forall <class RAJA::seq_exec> (begin, end, [=](int i)
   {
     if (i%2==0)
     {
       // Condition 4: accumulation pattern: lhs accum-op rhs
       // lhs : array element access x[i]: x is pointer type, i is loop index 
       // rhs: a scalar double type
       // accum-op:   +=, -=, *=, /=, MIN (), MAX() 
       x1[i] += rh1;
       x2[i] -= rh1;
       x3[i] *= rh1;
       x4[i] /= rh1;
     }
   } );
}

// initialization of pointers are buried within if-statement
void foo3(double* x, int jp, int kp, RAJA::Index_type begin, RAJA::Index_type end, double rh1)
{
  //Condition 1: pointer declaration, 4 or 8 pointers
  double * x1, *x2, *x3, *x4;

  //Condition 2:  pointer initialization, using other pointers on rhs
  if (jp%2==0)
  {
    x1 = x;
    x2 = x +2;
    x3 = x1 + jp+2;
    x4 = x1 + kp+3;
  }
  else
  {

    x1 = x;
    x2 = x +1;
    x3 = x1 + jp;
    x4 = x1 + kp;
  }

  //Condition 3:  A regular loop or a RAJA loop
  RAJA::forall <class RAJA::seq_exec> (begin, end, [=](int i)
      {
      if (i%2==0)
      {
      // Condition 4: accumulation pattern: lhs accum-op rhs
      // lhs : array element access x[i]: x is pointer type, i is loop index 
      // rhs: a scalar double type
      // accum-op:   +=, -=, *=, /=, MIN (), MAX() 
      x1[i] += rh1;
      x2[i] -= rh1;
      x3[i] *= rh1;
      x4[i] /= rh1;
      }
      } );
}


