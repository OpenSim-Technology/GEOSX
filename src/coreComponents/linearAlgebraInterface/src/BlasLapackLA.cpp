/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Copyright (c) 2019, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory
 *
 * LLNL-CODE-746361
 *
 * All rights reserved. See COPYRIGHT for details.
 *
 * This file is part of the GEOSX Simulation Framework.
 *
 * GEOSX is a free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (as published by the
 * Free Software Foundation) version 2.1 dated February 1999.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/**
 * @file BlasLapack.hpp
 */

// Include the corresponding header file.
#include "BlasLapackLA.hpp"

// BLAS and LAPACK function declaration
extern "C"
{
  #include "BlasLapackFunctions.hpp"
}

// Put everything under the geosx namespace.
namespace geosx
{

//-------------------------------------------------------Mathematical methods---

real64 BlasLapackLA::vectorNorm1( array1d<real64> const & X )
{
  int const INCX = 1;
  int const N = integer_conversion<int>( X.size() );
  return dasum_( &N,
                 X.data(),
                 &INCX);
}

real64 BlasLapackLA::vectorNorm2( array1d<real64> const & X )
{
  int const INCX = 1;
  int const N = integer_conversion<int>( X.size() );
  return dnrm2_( &N,
                 X.data(),
                 &INCX);
}

real64 BlasLapackLA::vectorNormInf( array1d<real64> const & X )
                                    {
  int const INCX = 1;
  int const N = integer_conversion<int>( X.size() );
  int ind = idamax_( &N,
                     X.data(),
                     &INCX);
  ind -= 1; // Fortran convention, subtract 1
  return std::abs( X(ind) );
}

real64 BlasLapackLA::determinant( array2d<real64> const & A )
                                  {
  // --- check that matrix is square
  GEOS_ASSERT_MSG( A.size( 0 ) == A.size( 1 ) &&
                   A.size( 0 ) > 0,
                   "Matrix must be square with order greater than zero" );

  real64 det;
  switch( A.size( 0 ) )
  {
    case 1:
    {
      det =  A( 0, 0 );
      break;
    }
    case 2:
    {
      det = A( 0, 0 ) * A( 1, 1 ) - A( 0, 1 ) * A( 1, 0 );
      break;
    }
    case 3:
    {
      det =
      A( 0, 0 ) * ( A( 1, 1 ) * A( 2, 2 ) - A( 1, 2 ) * A( 2, 1 ) ) +
      A( 0, 1 ) * ( A( 1, 2 ) * A( 2, 0 ) - A( 1, 0 ) * A( 2, 2 ) ) +
      A( 0, 2 ) * ( A( 1, 0 ) * A( 2, 1 ) - A( 1, 1 ) * A( 2, 0 ) );
      break;
    }
    case 4:
    {
      det =
      A( 0, 0 ) * ( A( 1, 1 ) * ( A( 2, 2 ) * A( 3, 3 ) - A( 3, 2 ) * A( 2, 3 ) ) -
                    A( 1, 2 ) * ( A( 2, 1 ) * A( 3, 3 ) - A( 3, 1 ) * A( 2, 3 ) ) +
                    A( 1, 3 ) * ( A( 2, 1 ) * A( 3, 2 ) - A( 3, 1 ) * A( 2, 2 ) )
                  ) -
      A( 0, 1 ) * ( A( 1, 0 ) * ( A( 2, 2 ) * A( 3, 3 ) - A( 3, 2 ) * A( 2, 3 ) ) -
                    A( 1, 2 ) * ( A( 2, 0 ) * A( 3, 3 ) - A( 3, 0 ) * A( 2, 3 ) ) +
                    A( 1, 3 ) * ( A( 2, 0 ) * A( 3, 2 ) - A( 3, 0 ) * A( 2, 2 ) )
                  ) +
      A( 0, 2 ) * ( A( 1, 0 ) * ( A( 2, 1 ) * A( 3, 3 ) - A( 3, 1 ) * A( 2, 3 ) ) -
                    A( 1, 1 ) * ( A( 2, 0 ) * A( 3, 3 ) - A( 3, 0 ) * A( 2, 3 ) ) +
                    A( 1, 3 ) * ( A( 2, 0 ) * A( 3, 1 ) - A( 3, 0 ) * A( 2, 1 ) )
                  ) -
      A( 0, 3 ) * ( A( 1, 0 ) * ( A( 2, 1 ) * A( 3, 2 ) - A( 3, 1 ) * A( 2, 2 ) ) -
                    A( 1, 1 ) * ( A( 2, 0 ) * A( 3, 2 ) - A( 3, 0 ) * A( 2, 2 ) ) +
                    A( 1, 2 ) * ( A( 2, 0 ) * A( 3, 1 ) - A( 3, 0 ) * A( 2, 1 ) )
                  );
      break;
    }
    default:
    {
      // Compute the determinant via LU factorization
      int const NN = integer_conversion<int>( A.size(0) );
      int INFO;
      array1d<int> IPIV( NN );
      array2d<double> LUfactor( A );

      // We compute the LU factors for the transpose matrix, i.e. choosing the
      // LAPACK_COL_MAJOR ordering, to avoid transposition/copy requires for
      // LAPACK_ROW_MAJOR ordering.
      dgetrf_( &NN,
               &NN,
               LUfactor.data(),
               &NN,
               IPIV.data(),
               &INFO);

      GEOS_ASSERT_MSG( INFO == 0, "LAPACK dgetrf error code: " << INFO );

      det = 1.0;
      for( int i = 0 ; i < NN ; ++i )
      {
        if( IPIV[i] != i + 1 ) //IPIV is based on Fortran convention (counting from 1)
        {
          det *= -LUfactor( i, i);
        }
        else
        {
          det *= LUfactor( i, i);
        }
      }

      break;
    }
  }

  return det;

}

real64 BlasLapackLA::matrixNormInf( array2d<real64> const & A )
{
  // Computed as one-norm of the transpose matrix
  char const NORM = '1';
  int const  M = integer_conversion<int>( A.size(0) );
  int const  N = integer_conversion<int>( A.size(1) );
  return dlange_(&NORM,
                 &N,
                 &M,
                 A.data(),
                 &N,
                 nullptr);

}

real64 BlasLapackLA::matrixNorm1( array2d<real64> const & A )
{
  // Computed as infinity-norm of the transpose matrix
  char const NORM = 'I';
  int const M = integer_conversion<int>( A.size(0) );
  int const N = integer_conversion<int>( A.size(1) );
  array1d<double> WORK(N);
  return  dlange_( &NORM,
                   &N,
                   &M,
                   A.data(),
                   &N,
                   WORK.data());
}

real64 BlasLapackLA::matrixNormFrobenius( array2d<real64> const & A )
{
  // Computed using the transpose matrix
  char const NORM = 'F';
  int const M = integer_conversion<int>( A.size(0) );
  int const N = integer_conversion<int>( A.size(1) );
  return dlange_(&NORM,
                 &N,
                 &M,
                 A.data(),
                 &N,
                 nullptr);

}

void BlasLapackLA::vectorVectorAdd( array1d<real64> const & X,
                                    array1d<real64> & Y,
                                    real64 const alpha )
{

  GEOS_ASSERT_MSG( X.size() == Y.size(),
                   "Vector dimensions not compatible for sum" );

  int const INCX = 1;
  int const INCY = 1;
  int const N = static_cast<int>( X.size() );
  daxpy_( &N,
          &alpha,
          X.data(),
          &INCX,
          Y.data(),
          &INCY );

  return;
}

void BlasLapackLA::matrixMatrixAdd( array2d<real64> const & A,
                                    array2d<real64> & B,
                                    real64 const alpha )
{

  GEOS_ASSERT_MSG( A.size( 0 ) == B.size( 0 ) &&
                   A.size( 1 ) == B.size( 1 ),
                   "Matrix dimensions not compatible for sum" );

  int const INCX = 1;
  int const INCY = 1;
  int const N = static_cast<int>( A.size() );
  daxpy_( &N,
          &alpha,
          A.data(),
          &INCX,
          B.data(),
          &INCY );

  return;
}

void BlasLapackLA::vectorScale( array1d<real64> & X,
                                real64 alpha )
{

  int const INCX = 1;
  int const N = static_cast<int>( X.size() );
  dscal_( &N,
          &alpha,
          X.data(),
          &INCX);

  return;
}

void BlasLapackLA::matrixScale( array2d<real64> & A,
                                real64 alpha )
{

  int const INCX = 1;
  int const N = static_cast<int>( A.size() );
  dscal_( &N,
          &alpha,
          A.data(),
          &INCX);

  return;
}

real64 BlasLapackLA::vectorDot( array1d<real64> const & X,
                                array1d<real64> const & Y )
{
  GEOS_ASSERT_MSG( X.size() == Y.size(),
                   "Vector dimensions not compatible for dot product" );

  int const INCX = 1;
  int const INCY = 1;
  int const N = static_cast<int>( X.size() );
  return ddot_( &N,
                X.data(),
                &INCX,
                Y.data(),
                &INCY );

}

void BlasLapackLA::matrixVectorMultiply( array2d<real64> const & A,
                                         array1d<real64> const & X,
                                         array1d<real64> & Y,
                                         real64 const alpha,
                                         real64 const beta )
{
  GEOS_ASSERT_MSG( A.size( 1 ) == X.size() &&
                   A.size( 0 ) == Y.size(),
                   "Matrix, source vector and destination vector not compatible" );

  int const M = integer_conversion<int>( A.size( 0 ) );
  int const N = 1;
  int const K = integer_conversion<int>( A.size( 1 ) );

  // A*X = Y is computed as X^T * A^T = Y^T, i.e. accessing the transpose
  // matrix using a column-major layout
  char const TRANS1 = 'N';
  char const TRANS2 = 'N';

  dgemm_(&TRANS1,
         &TRANS2,
         &N,
         &M,
         &K,
         &alpha,
         X.data(),
         &N,
         A.data(),
         &K,
         &beta,
         Y.data(),
         &N);

  return;
}

void BlasLapackLA::matrixTVectorMultiply( array2d<real64> const & A,
                                          array1d<real64> const & X,
                                          array1d<real64> & Y,
                                          real64 const alpha,
                                          real64 const beta )
{
  GEOS_ASSERT_MSG( A.size( 0 ) == X.size() &&
                       A.size( 1 ) == Y.size(),
                   "Matrix, source vector and destination vector not compatible" );

  int const M = integer_conversion<int>( A.size( 1 ) );
  int const N = 1;
  int const K = integer_conversion<int>( A.size( 0 ) );

  // A^T*X = Y is computed as X^T * A = Y^T, i.e. accessing the transpose
  // matrix using a column-major layout
  char const TRANS1 = 'N';
  char const TRANS2 = 'T';

  dgemm_(&TRANS1,
         &TRANS2,
         &N,
         &M,
         &K,
         &alpha,
         X.data(),
         &N,
         A.data(),
         &M,
         &beta,
         Y.data(),
         &N);

  return;
}

void BlasLapackLA::matrixMatrixMultiply( array2d<real64> const & A,
                                         array2d<real64> const & B,
                                         array2d<real64> & C,
                                         real64 const alpha,
                                         real64 const beta )
{

  GEOS_ASSERT_MSG( C.size( 0 ) == A.size( 0 ) &&
                   C.size( 1 ) == B.size( 1 ) &&
                   A.size( 1 ) == B.size( 0 ),
                   "Matrix dimensions not compatible for product" );

  int const M = integer_conversion<int>( A.size( 0 ) );
  int const N = integer_conversion<int>( B.size( 1 ) );
  int const K = integer_conversion<int>( A.size( 1 ) );

  // A*B = C is computed as B^T * A^T = C^T, i.e. accessing the transpose
  // matrices using a column-major layout
  char const TRANS1 = 'N';
  char const TRANS2 = 'N';

  dgemm_(&TRANS1,
         &TRANS2,
         &N,
         &M,
         &K,
         &alpha,
         B.data(),
         &N,
         A.data(),
         &K,
         &beta,
         C.data(),
         &N);

  return;
}

void BlasLapackLA::matrixTMatrixMultiply( array2d<real64> const & A,
                                          array2d<real64> const & B,
                                          array2d<real64> & C,
                                          real64 const alpha,
                                          real64 const beta )
{

  GEOS_ASSERT_MSG( C.size( 0 ) == A.size( 1 ) &&
                   C.size( 1 ) == B.size( 1 ) &&
                   A.size( 0 ) == B.size( 0 ),
                   "Matrix dimensions not compatible for product" );

  int const M = integer_conversion<int>( A.size( 1 ) );
  int const N = integer_conversion<int>( B.size( 1 ) );
  int const K = integer_conversion<int>( A.size( 0 ) );

  // A^T*B = C is computed as B^T * A = C^T, i.e. accessing the transpose
  // matrices using a column-major layout

  char const TRANS1 = 'N';
  char const TRANS2 = 'T';

  dgemm_(&TRANS1,
         &TRANS2,
         &N,
         &M,
         &K,
         &alpha,
         B.data(),
         &N,
         A.data(),
         &M,
         &beta,
         C.data(),
         &N);

  return;
}

void BlasLapackLA::matrixMatrixTMultiply( array2d<real64> const & A,
                                          array2d<real64> const & B,
                                          array2d<real64> & C,
                                          real64 const alpha,
                                          real64 const beta )
{

  GEOS_ASSERT_MSG( C.size( 0 ) == A.size( 0 ) &&
                       C.size( 1 ) == B.size( 0 ) &&
                       A.size( 1 ) == B.size( 1 ),
                   "Matrix dimensions not compatible for product" );

  int const M = integer_conversion<int>( A.size( 0 ) );
  int const N = integer_conversion<int>( B.size( 0 ) );
  int const K = integer_conversion<int>( A.size( 1 ) );

  // A*B^T = C is computed as B * A^T = C^T, i.e. accessing the transpose
  // matrices using a column-major layout

  char const TRANS1 = 'T';
  char const TRANS2 = 'N';

  dgemm_(&TRANS1,
         &TRANS2,
         &N,
         &M,
         &K,
         &alpha,
         B.data(),
         &K,
         A.data(),
         &K,
         &beta,
         C.data(),
         &N);

  return;
}

void BlasLapackLA::matrixTMatrixTMultiply( array2d<real64> const & A,
                                           array2d<real64> const & B,
                                           array2d<real64> & C,
                                           real64 const alpha,
                                           real64 const beta )
{

  GEOS_ASSERT_MSG( C.size( 0 ) == A.size( 1 ) &&
                       C.size( 1 ) == B.size( 0 ) &&
                       A.size( 0 ) == B.size( 1 ),
                   "Matrix dimensions not compatible for product" );

  int const M = integer_conversion<int>( A.size( 1 ) );
  int const N = integer_conversion<int>( B.size( 0 ) );
  int const K = integer_conversion<int>( A.size( 0 ) );

  // A^T*B^T = C is computed as B * A = C^T, i.e. accessing the transpose
  // matrices using a column-major layout

  char const TRANS1 = 'T';
  char const TRANS2 = 'T';

  dgemm_(&TRANS1,
         &TRANS2,
         &N,
         &M,
         &K,
         &alpha,
         B.data(),
         &K,
         A.data(),
         &M,
         &beta,
         C.data(),
         &N);

  return;
}

void BlasLapackLA::matrixInverse( array2d<real64> const & A,
                                  array2d<real64> & Ainv )
{
  real64 detA;
  matrixInverse( A,
                 Ainv,
                 detA );
}

void BlasLapackLA::matrixInverse( array2d<real64> const & A,
                                  array2d<real64> & Ainv,
                                  real64 & detA )
{
  // --- Check that source matrix is square
  int const NN = integer_conversion<int>(A.size( 0 ));
  GEOS_ASSERT_MSG( NN > 0 &&
                   NN == A.size( 1 ),
                   "Matrix must be square" );

  // --- Check that inverse matrix has appropriate dimension
  GEOS_ASSERT_MSG( Ainv.size( 0 ) == NN &&
                   Ainv.size( 1 ) == NN,
                   "Inverse matrix has wrong dimensions" );

  // --- Check if matrix is singular by computing the determinant
  //     note: if order greater than 3 we compute the determinant by
  //           first constructing the LU factors, later reused for calculating
  //           the inverse.
  array1d<int> IPIV;
  array1d<double> INV_WORK;

  if (NN <= 3)
  {
    detA = determinant(A);
  }
  else
  {
    // Copy A in Ainv
    matrixCopy(A, Ainv);

    // Declare workspace for permutations and scratch array
    IPIV.resize(NN);
    INV_WORK.resize(NN);

    // Compute determinant (not done calling directly the function determinant
    // (avoid computing twice LUfactors, currently stored in Ainv, needed for
    // computing the inverse). Again we compute the LU factors for the
    // transpose matrix, i.e. choosing the LAPACK_COL_MAJOR ordering, to
    // avoid transposition/copy requires for LAPACK_ROW_MAJOR ordering.
    int INFO;
    dgetrf_( &NN,
             &NN,
             Ainv.data(),
             &NN,
             IPIV.data(),
             &INFO);

    GEOS_ASSERT_MSG( INFO == 0, "LAPACK dgetrf error code: " << INFO );

    detA = 1.0;
    for( int i = 0 ; i < NN ; ++i )
    {
      if( IPIV[i] != i + 1 ) //IPIV is based on Fortran convention (counting from 1)
      {
        detA *= -Ainv(i,i);
      }
      else
      {
        detA *= Ainv(i,i);
      }
    }
  }

  // Check if matrix is singular
  GEOS_ASSERT_MSG( std::fabs(detA) >
                   std::numeric_limits<real64>::epsilon() *
                   matrixNormFrobenius(A),
                   "Matrix is singular" );
  real64 oneOverDetA = 1. / detA;

  // --- Compute inverse
  switch( NN )
    {
      case 1:
      {
        Ainv( 0, 0 ) = oneOverDetA;
        break;
      }

      case 2:
      {
        Ainv( 0, 0 ) =  A( 1, 1 ) * oneOverDetA;
        Ainv( 0, 1 ) = -A( 0, 1 ) * oneOverDetA;
        Ainv( 1, 0 ) = -A( 1, 0 ) * oneOverDetA;
        Ainv( 1, 1 ) =  A( 0, 0 ) * oneOverDetA;
        break;
      }

      case 3:
      {
        Ainv( 0, 0 ) = ( A( 1, 1 ) * A( 2, 2 ) -
                         A( 1, 2 ) * A( 2, 1 ) ) * oneOverDetA;
        Ainv( 0, 1 ) = ( A( 0, 2 ) * A( 2, 1 ) -
                         A( 0, 1 ) * A( 2, 2 ) ) * oneOverDetA;
        Ainv( 0, 2 ) = ( A( 0, 1 ) * A( 1, 2 ) -
                         A( 0, 2 ) * A( 1, 1 ) ) * oneOverDetA;
        Ainv( 1, 0 ) = ( A( 1, 2 ) * A( 2, 0 ) -
                         A( 1, 0 ) * A( 2, 2 ) ) * oneOverDetA;
        Ainv( 1, 1 ) = ( A( 0, 0 ) * A( 2, 2 ) -
                         A( 0, 2 ) * A( 2, 0 ) ) * oneOverDetA;
        Ainv( 1, 2 ) = ( A( 0, 2 ) * A( 1, 0 ) -
                         A( 0, 0 ) * A( 1, 2 ) ) * oneOverDetA;
        Ainv( 2, 0 ) = ( A( 1, 0 ) * A( 2, 1 ) -
                         A( 1, 1 ) * A( 2, 0 ) ) * oneOverDetA;
        Ainv( 2, 1 ) = ( A( 0, 1 ) * A( 2, 0 ) -
                         A( 0, 0 ) * A( 2, 1 ) ) * oneOverDetA;
        Ainv( 2, 2 ) = ( A( 0, 0 ) * A( 1, 1 ) -
                         A( 0, 1 ) * A( 1, 0 ) ) * oneOverDetA;
        break;
      }
      default:
    {
    // Invert (LAPACK function DGETRI). The LU factors computed for the
    // transpose matrix stored in Ainv are used.
    int INFO;
    dgetri_( &NN,
             Ainv.data(),
             &NN,
             IPIV.data(),
             INV_WORK.data(),
             &NN,
             &INFO);

    GEOS_ASSERT_MSG( INFO == 0, "LAPACK dgetri error code: " << INFO );

    break;
    }
  }
  return;
}

void BlasLapackLA::vectorCopy( array1d<real64> const & X,
                               array1d<real64> & Y )
{
  GEOS_ASSERT_MSG( X.size() == Y.size(),
                   "Vector dimensions not compatible for copying" );

  int const INCX = 1;
  int const INCY = 1;
  int const N = static_cast<int>( X.size() );
  dcopy_( &N,
          X.data(),
          &INCX,
          Y.data(),
          &INCY );

  return;
}

void BlasLapackLA::matrixCopy( array2d<real64> const & A,
                               array2d<real64> & B )
{
  GEOS_ASSERT_MSG( A.size(0) == B.size(0) &&
                   A.size(1) == B.size(1),
                   "Matrix dimensions not compatible for copying" );

  int const INCX = 1;
  int const INCY = 1;
  int const N = static_cast<int>( A.size() );
  dcopy_( &N,
          A.data(),
          &INCX,
          B.data(),
          &INCY );

  return;
}

//----------------------------------------------------------------I/O methods---
// vector nice output
void BlasLapackLA::printVector( array1d<real64> const & X )
{
  for( int i = 0 ; i < X.size() ; ++i )
  {
    printf( "%10.2e ", X[i] );
    printf( "\n" );
  }
}

// vector nice output
void BlasLapackLA::printMatrix( array2d<real64> const & A )
{
  for( int i = 0 ; i < A.size( 0 ) ; ++i )
  {
    for( int j = 0 ; j < A.size( 1 ) ; ++j )
    {
      printf( "%10.2e ", A( i, j ) );
    }
    printf( "\n" );
  }
}

} // end geosx namespace