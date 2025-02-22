// ---------------------------------------------------------------------
//
// Copyright (C) 2018 - 2019 by the deal.II authors
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


#include <deal.II/base/utilities.h>

#include <deal.II/lac/read_write_vector.h>
#include <deal.II/lac/trilinos_tpetra_vector.h>

#include <iostream>
#include <vector>

#include "../tests.h"

// Check LinearAlgebra::TpetraWrappers::Vector add and sadd.

template <typename Number>
void
test()
{
  IndexSet     parallel_partitioner_1(10);
  IndexSet     parallel_partitioner_2(10);
  unsigned int rank = Utilities::MPI::this_mpi_process(MPI_COMM_WORLD);
  if (rank == 0)
    {
      parallel_partitioner_1.add_range(0, 5);
      parallel_partitioner_2.add_range(0, 3);
    }
  else
    {
      parallel_partitioner_1.add_range(5, 10);
      parallel_partitioner_2.add_range(3, 10);
    }
  parallel_partitioner_1.compress();
  parallel_partitioner_2.compress();
  LinearAlgebra::TpetraWrappers::Vector<Number> a(parallel_partitioner_1,
                                                  MPI_COMM_WORLD);
  LinearAlgebra::TpetraWrappers::Vector<Number> b(parallel_partitioner_1,
                                                  MPI_COMM_WORLD);
  LinearAlgebra::TpetraWrappers::Vector<Number> c(parallel_partitioner_2,
                                                  MPI_COMM_WORLD);

  IndexSet read_write_index_set(10);
  if (rank == 0)
    read_write_index_set.add_range(0, 5);
  else
    read_write_index_set.add_range(5, 10);
  read_write_index_set.compress();

  LinearAlgebra::ReadWriteVector<Number> read_write_1(read_write_index_set);
  LinearAlgebra::ReadWriteVector<Number> read_write_2(read_write_index_set);
  LinearAlgebra::ReadWriteVector<Number> read_write_3(read_write_index_set);
  if (rank == 0)
    {
      for (unsigned int i = 0; i < 5; ++i)
        {
          read_write_1[i] = i;
          read_write_2[i] = 5. + i;
        }
    }
  else
    {
      for (unsigned int i = 5; i < 10; ++i)
        {
          read_write_1[i] = i;
          read_write_2[i] = 5. + i;
        }
    }

  a.import(read_write_1, VectorOperation::insert);
  b.import(read_write_2, VectorOperation::insert);
  c.import(read_write_2, VectorOperation::insert);

  a.add(1.);
  read_write_3.import(a, VectorOperation::insert);
  if (rank == 0)
    {
      for (unsigned int i = 0; i < 5; ++i)
        AssertThrow(Number(1.) + read_write_1[i] == read_write_3[i],
                    ExcMessage("Problem in add(scalar)."));
    }
  else
    {
      for (unsigned int i = 5; i < 10; ++i)
        AssertThrow(Number(1.) + read_write_1[i] == read_write_3[i],
                    ExcMessage("Problem in add(scalar)."));
    }

  a.add(2., b);
  read_write_3.import(a, VectorOperation::insert);
  if (rank == 0)
    {
      for (unsigned int i = 0; i < 5; ++i)
        AssertThrow(Number(1.) + read_write_1[i] +
                        Number(2.) * read_write_2[i] ==
                      read_write_3[i],
                    ExcMessage("Problem in add(scalar,Vector)."));
    }
  else
    {
      for (unsigned int i = 5; i < 10; ++i)
        AssertThrow(Number(1.) + read_write_1[i] +
                        Number(2.) * read_write_2[i] ==
                      read_write_3[i],
                    ExcMessage("Problem in add(scalar,Vector)."));
    }


  LinearAlgebra::TpetraWrappers::Vector<Number> d(a);
  a.add(2., b, 3., d);
  read_write_3.import(a, VectorOperation::insert);
  if (rank == 0)
    {
      for (unsigned int i = 0; i < 5; ++i)
        AssertThrow(Number(4.) + Number(4.) * read_write_1[i] +
                        Number(10.) * read_write_2[i] ==
                      read_write_3[i],
                    ExcMessage("Problem in add(scalar,Vector,scalar,Vector)."));
    }
  else
    {
      for (unsigned int i = 5; i < 10; ++i)
        AssertThrow(Number(4.) + Number(4.) * read_write_1[i] +
                        Number(10.) * read_write_2[i] ==
                      read_write_3[i],
                    ExcMessage("Problem in add(scalar,Vector,scalar,Vector)."));
    }


  a.import(read_write_1, VectorOperation::insert);
  a.sadd(3., 2., c);
  read_write_3.import(a, VectorOperation::insert);
  if (rank == 0)
    {
      for (unsigned int i = 0; i < 5; ++i)
        AssertThrow(Number(3.) * read_write_1[i] +
                        Number(2.) * read_write_2[i] ==
                      read_write_3[i],
                    ExcMessage("Problem in sadd(scalar,scalar,Vector)."));
    }
  else
    {
      for (unsigned int i = 5; i < 10; ++i)
        AssertThrow(Number(3.) * read_write_1[i] +
                        Number(2.) * read_write_2[i] ==
                      read_write_3[i],
                    ExcMessage("Problem in sadd(scalar,scalar,Vector)."));
    }


  a.import(read_write_1, VectorOperation::insert);
  a.scale(b);
  read_write_3.import(a, VectorOperation::insert);
  if (rank == 0)
    {
      for (unsigned int i = 0; i < 5; ++i)
        AssertThrow(read_write_1[i] * read_write_2[i] == read_write_3[i],
                    ExcMessage("Problem in scale."));
    }
  else
    {
      for (unsigned int i = 5; i < 10; ++i)
        AssertThrow(read_write_1[i] * read_write_2[i] == read_write_3[i],
                    ExcMessage("Problem in scale."));
    }


  a.equ(2., c);
  read_write_3.import(a, VectorOperation::insert);
  if (rank == 0)
    {
      for (unsigned int i = 0; i < 5; ++i)
        AssertThrow(Number(2.) * read_write_2[i] == read_write_3[i],
                    ExcMessage("Problem in scale."));
    }
  else
    {
      for (unsigned int i = 5; i < 10; ++i)
        AssertThrow((Number)2. * read_write_2[i] == read_write_3[i],
                    ExcMessage("Problem in equ."));
    }


  AssertThrow(b.l1_norm() == 95., ExcMessage("Problem in l1_norm."));

  const double eps = 1e-6;
  AssertThrow(std::fabs(b.l2_norm() - 31.3847096) < eps,
              ExcMessage("Problem in l2_norm"));

  AssertThrow(b.linfty_norm() == 14., ExcMessage("Problem in linfty_norm."));

  a.import(read_write_1, VectorOperation::insert);
  const Number val = a.add_and_dot(2., a, b);
  AssertThrow(val == Number(1530.), ExcMessage("Problem in add_and_dot"));
}


int
main(int argc, char **argv)
{
  initlog();
  deallog.depth_console(0);

  Utilities::MPI::MPI_InitFinalize mpi_init(argc, argv, 1);

  test<double>();

  deallog << "OK" << std::endl;

  return 0;
}
