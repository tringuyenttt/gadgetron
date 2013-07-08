#pragma once

#include "gpBbSolver.h"
#include "hoNDArray_operators.h"
#include "hoNDArray_elemwise.h"
#include "hoNDArray_blas.h"
#include "real_utilities.h"
#include "vector_td_utilities.h"

#include <omp.h>

namespace Gadgetron{

  template <class T> class hoGpBbSolver : public gpBbSolver< hoNDArray<T> >
  {  
  public:

    hoGpBbSolver() : gpBbSolver< hoNDArray<T> >() {};
    virtual ~hoGpBbSolver() {};
        
    virtual void solver_non_negativity_filter(hoNDArray<T> *xdata, hoNDArray<T> *gdata)
    {
      typedef typename realType<T>::Type REAL;

      T* x = xdata->get_data_ptr();
      T* g = gdata->get_data_ptr();

#ifdef USE_OMP
#pragma omp parallel for
#endif
      for( int i=0; i < xdata->get_number_of_elements(); i++ )
	if( (real(x[i]) <= REAL(0)) && (real(g[i]) > 0) ) 
	  g[i]=T(0);
    }
  };
}
