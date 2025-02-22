// ---------------------------------------------------------------------
//
// Copyright (C) 2006 - 2021 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------



// an adaptation of the mesh_3d_22 test.  check that VectorTools::interpolate
// works for FE_Q(p) elements correctly on a mesh with a cell that has a wrong
// face orientation in the interior of the domain. when using a MappingQ(3), we
// calculated interpolation points wrongly

#include <deal.II/base/function.h>
#include <deal.II/base/quadrature_lib.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/mapping_q.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include <deal.II/lac/vector.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/vector_tools.h>

#include <vector>

#include "../tests.h"

template <int dim>
class F : public Function<dim>
{
public:
  F(const unsigned int q)
    : q(q)
  {}

  virtual double
  value(const Point<dim> &p, const unsigned int) const
  {
    double v = 0;
    for (unsigned int d = 0; d < dim; ++d)
      for (unsigned int i = 0; i <= q; ++i)
        v += (d + 1) * (i + 1) * std::pow(p[d], 1. * i);
    return v;
  }

private:
  const unsigned int q;
};



template <int dim>
void
test(Triangulation<dim> &triangulation)
{
  // create a MappingQ(3) which is used on ALL
  // cells
  MappingQ<3> mapping(3);
  for (unsigned int p = 1; p < 7 - dim; ++p)
    {
      FE_Q<dim>       fe(p);
      DoFHandler<dim> dof_handler(triangulation);
      dof_handler.distribute_dofs(fe);

      Vector<double> interpolant(dof_handler.n_dofs());
      Vector<float>  error(triangulation.n_active_cells());
      for (unsigned int q = 0; q <= p + 2; ++q)
        {
          // interpolate the function
          VectorTools::interpolate(mapping,
                                   dof_handler,
                                   F<dim>(q),
                                   interpolant);

          // then compute the interpolation error
          VectorTools::integrate_difference(mapping,
                                            dof_handler,
                                            interpolant,
                                            F<dim>(q),
                                            error,
                                            QGauss<dim>(q + 2),
                                            VectorTools::L2_norm);
          deallog << fe.get_name() << ", P_" << q
                  << ", rel. error=" << error.l2_norm() / interpolant.l2_norm()
                  << std::endl;
          if (q <= p)
            Assert(error.l2_norm() < 1e-12 * interpolant.l2_norm(),
                   ExcInternalError());
        }
    }
}



int
main()
{
  initlog();
  Triangulation<3> triangulation;
  GridGenerator::hyper_ball(triangulation);
  triangulation.reset_manifold(0);
  triangulation.refine_global(1);
  test<3>(triangulation);
}
