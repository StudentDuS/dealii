// ---------------------------------------------------------------------
//
// Copyright (C) 2020 - 2022 by the deal.II authors
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

#ifndef dealii_matrix_free_tools_h
#define dealii_matrix_free_tools_h

#include <deal.II/base/config.h>

#include <deal.II/grid/tria.h>

#include <deal.II/matrix_free/fe_evaluation.h>
#include <deal.II/matrix_free/matrix_free.h>
#include <deal.II/matrix_free/vector_access_internal.h>


DEAL_II_NAMESPACE_OPEN

/**
 * A namespace for utility functions in the context of matrix-free operator
 * evaluation.
 */
namespace MatrixFreeTools
{
  /**
   * Modify @p additional_data so that cells are categorized
   * according to their boundary IDs, making face integrals in the case of
   * cell-centric loop simpler.
   */
  template <int dim, typename AdditionalData>
  void
  categorize_by_boundary_ids(const Triangulation<dim> &tria,
                             AdditionalData &          additional_data);

  /**
   * Compute the diagonal of a linear operator (@p diagonal_global), given
   * @p matrix_free and the local cell integral operation @p local_vmult. The
   * vector is initialized to the right size in the function.
   *
   * The parameters @p dof_no, @p quad_no, and @p first_selected_component are
   * passed to the constructor of the FEEvaluation that is internally set up.
   */
  template <int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename VectorType>
  void
  compute_diagonal(
    const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
    VectorType &                                        diagonal_global,
    const std::function<void(FEEvaluation<dim,
                                          fe_degree,
                                          n_q_points_1d,
                                          n_components,
                                          Number,
                                          VectorizedArrayType> &)> &local_vmult,
    const unsigned int                                              dof_no  = 0,
    const unsigned int                                              quad_no = 0,
    const unsigned int first_selected_component = 0);

  /**
   * Same as above but with a class and a function pointer.
   */
  template <typename CLASS,
            int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename VectorType>
  void
  compute_diagonal(
    const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
    VectorType &                                        diagonal_global,
    void (CLASS::*cell_operation)(FEEvaluation<dim,
                                               fe_degree,
                                               n_q_points_1d,
                                               n_components,
                                               Number,
                                               VectorizedArrayType> &) const,
    const CLASS *      owning_class,
    const unsigned int dof_no                   = 0,
    const unsigned int quad_no                  = 0,
    const unsigned int first_selected_component = 0);


  /**
   * Compute the matrix representation of a linear operator (@p matrix), given
   * @p matrix_free and the local cell integral operation @p local_vmult.
   * Constrained entries on the diagonal are set to one.
   *
   * The parameters @p dof_no, @p quad_no, and @p first_selected_component are
   * passed to the constructor of the FEEvaluation that is internally set up.
   */
  template <int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename MatrixType>
  void
  compute_matrix(
    const MatrixFree<dim, Number, VectorizedArrayType> &            matrix_free,
    const AffineConstraints<Number> &                               constraints,
    MatrixType &                                                    matrix,
    const std::function<void(FEEvaluation<dim,
                                          fe_degree,
                                          n_q_points_1d,
                                          n_components,
                                          Number,
                                          VectorizedArrayType> &)> &local_vmult,
    const unsigned int                                              dof_no  = 0,
    const unsigned int                                              quad_no = 0,
    const unsigned int first_selected_component = 0);

  /**
   * Same as above but with a class and a function pointer.
   */
  template <typename CLASS,
            int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename MatrixType>
  void
  compute_matrix(
    const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
    const AffineConstraints<Number> &                   constraints,
    MatrixType &                                        matrix,
    void (CLASS::*cell_operation)(FEEvaluation<dim,
                                               fe_degree,
                                               n_q_points_1d,
                                               n_components,
                                               Number,
                                               VectorizedArrayType> &) const,
    const CLASS *      owning_class,
    const unsigned int dof_no                   = 0,
    const unsigned int quad_no                  = 0,
    const unsigned int first_selected_component = 0);



  /**
   * A wrapper around MatrixFree to help users to deal with DoFHandler
   * objects involving cells without degrees of freedom, i.e.,
   * cells using FENothing as element type.  In the following we call such cells
   * deactivated. All other cells are activated. In contrast to MatrixFree,
   * this class skips deactivated cells and faces between activated and
   * deactivated cells are treated as boundary faces.
   */
  template <int dim,
            typename Number,
            typename VectorizedArrayType = VectorizedArray<Number>>
  class ElementActivationAndDeactivationMatrixFree
  {
  public:
    /**
     * Struct that helps to configure
     * ElementActivationAndDeactivationMatrixFree.
     */
    struct AdditionalData
    {
      /**
       * Constructor.
       */
      AdditionalData(const unsigned int dof_index = 0)
        : dof_index(dof_index)
      {}

      /**
       * Index of the DoFHandler within MatrixFree to be used.
       */
      unsigned int dof_index;
    };

    /**
     * Reinitialize class based on a given MatrixFree instance.
     * Particularly, the index of the valid FE is determined.
     *
     * @note At the moment, only DoFHandler objects are accepted
     * that are initialized with FECollection objects with at most
     * two finite elements (i.e., `FENothing` and another finite
     * element).
     */
    void
    reinit(const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
           const AdditionalData &additional_data = AdditionalData())
    {
      this->matrix_free = &matrix_free;

      std::vector<unsigned int> valid_fe_indices;

      const auto &fe_collection =
        matrix_free.get_dof_handler(additional_data.dof_index)
          .get_fe_collection();

      for (unsigned int i = 0; i < fe_collection.size(); ++i)
        if (fe_collection[i].n_dofs_per_cell() > 0)
          valid_fe_indices.push_back(i);

      // TODO: relax this so that arbitrary number of valid
      // FEs can be accepted
      AssertDimension(valid_fe_indices.size(), 1);

      fe_index_valid = *valid_fe_indices.begin();
    }

    /**
     * Loop over all activated cells.
     *
     * For the meaning of the parameters see MatrixFree::cell_loop().
     */
    template <typename VectorTypeOut, typename VectorTypeIn>
    void
    cell_loop(const std::function<void(
                const MatrixFree<dim, Number, VectorizedArrayType> &,
                VectorTypeOut &,
                const VectorTypeIn &,
                const std::pair<unsigned int, unsigned int> &)> &cell_operation,
              VectorTypeOut &                                    dst,
              const VectorTypeIn &                               src,
              const bool zero_dst_vector = false) const
    {
      const auto ebd_cell_operation = [&](const auto &matrix_free,
                                          auto &      dst,
                                          const auto &src,
                                          const auto &range) {
        const auto category = matrix_free.get_cell_range_category(range);

        if (category != fe_index_valid)
          return;

        cell_operation(matrix_free, dst, src, range);
      };

      matrix_free->template cell_loop<VectorTypeOut, VectorTypeIn>(
        ebd_cell_operation, dst, src, zero_dst_vector);
    }

    /**
     * Loop over all activated cells and faces. Faces between activated and
     * deactivated cells are treated as boundary faces with the boundary ID
     * numbers::internal_face_boundary_id.
     *
     * For the meaning of the parameters see MatrixFree::cell_loop().
     */
    template <typename VectorTypeOut, typename VectorTypeIn>
    void
    loop(const std::function<
           void(const MatrixFree<dim, Number, VectorizedArrayType> &,
                VectorTypeOut &,
                const VectorTypeIn &,
                const std::pair<unsigned int, unsigned int> &)> &cell_operation,
         const std::function<
           void(const MatrixFree<dim, Number, VectorizedArrayType> &,
                VectorTypeOut &,
                const VectorTypeIn &,
                const std::pair<unsigned int, unsigned int> &)> &face_operation,
         const std::function<
           void(const MatrixFree<dim, Number, VectorizedArrayType> &,
                VectorTypeOut &,
                const VectorTypeIn &,
                const std::pair<unsigned int, unsigned int> &,
                const bool)> &boundary_operation,
         VectorTypeOut &      dst,
         const VectorTypeIn & src,
         const bool           zero_dst_vector = false) const
    {
      const auto ebd_cell_operation = [&](const auto &matrix_free,
                                          auto &      dst,
                                          const auto &src,
                                          const auto &range) {
        const auto category = matrix_free.get_cell_range_category(range);

        if (category != fe_index_valid)
          return;

        cell_operation(matrix_free, dst, src, range);
      };

      const auto ebd_internal_or_boundary_face_operation =
        [&](const auto &matrix_free,
            auto &      dst,
            const auto &src,
            const auto &range) {
          const auto category = matrix_free.get_face_range_category(range);

          const unsigned int type =
            static_cast<unsigned int>(category.first == fe_index_valid) +
            static_cast<unsigned int>(category.second == fe_index_valid);

          if (type == 0)
            return; // deactivated face -> nothing to do

          if (type == 1) // boundary face
            boundary_operation(
              matrix_free, dst, src, range, category.first == fe_index_valid);
          else if (type == 2) // internal face
            face_operation(matrix_free, dst, src, range);
        };

      matrix_free->template loop<VectorTypeOut, VectorTypeIn>(
        ebd_cell_operation,
        ebd_internal_or_boundary_face_operation,
        ebd_internal_or_boundary_face_operation,
        dst,
        src,
        zero_dst_vector);
    }

  private:
    /**
     * Reference to the underlying MatrixFree object.
     */
    SmartPointer<const MatrixFree<dim, Number, VectorizedArrayType>>
      matrix_free;

    /**
     * Index of the valid FE. Currently, ony a single one is supported.
     */
    unsigned int fe_index_valid;
  };

  // implementations

#ifndef DOXYGEN

  template <int dim, typename AdditionalData>
  void
  categorize_by_boundary_ids(const Triangulation<dim> &tria,
                             AdditionalData &          additional_data)
  {
    // ... determine if we are on an active or a multigrid level
    const unsigned int level = additional_data.mg_level;
    const bool         is_mg = (level != numbers::invalid_unsigned_int);

    // ... create empty list for the category of each cell
    if (is_mg)
      additional_data.cell_vectorization_category.assign(
        std::distance(tria.begin(level), tria.end(level)), 0);
    else
      additional_data.cell_vectorization_category.assign(tria.n_active_cells(),
                                                         0);

    // ... set up scaling factor
    std::vector<unsigned int> factors(GeometryInfo<dim>::faces_per_cell);

    auto bids = tria.get_boundary_ids();
    std::sort(bids.begin(), bids.end());

    {
      unsigned int n_bids = bids.size() + 1;
      int          offset = 1;
      for (unsigned int i = 0; i < GeometryInfo<dim>::faces_per_cell;
           i++, offset = offset * n_bids)
        factors[i] = offset;
    }

    const auto to_category = [&](const auto &cell) {
      unsigned int c_num = 0;
      for (unsigned int i = 0; i < GeometryInfo<dim>::faces_per_cell; ++i)
        {
          auto &face = *cell->face(i);
          if (face.at_boundary() && !cell->has_periodic_neighbor(i))
            c_num +=
              factors[i] * (1 + std::distance(bids.begin(),
                                              std::find(bids.begin(),
                                                        bids.end(),
                                                        face.boundary_id())));
        }
      return c_num;
    };

    if (!is_mg)
      {
        for (auto cell = tria.begin_active(); cell != tria.end(); ++cell)
          {
            if (cell->is_locally_owned())
              additional_data
                .cell_vectorization_category[cell->active_cell_index()] =
                to_category(cell);
          }
      }
    else
      {
        for (auto cell = tria.begin(level); cell != tria.end(level); ++cell)
          {
            if (cell->is_locally_owned_on_level())
              additional_data.cell_vectorization_category[cell->index()] =
                to_category(cell);
          }
      }

    // ... finalize set up of matrix_free
    additional_data.hold_all_faces_to_owned_cells        = true;
    additional_data.cell_vectorization_categories_strict = true;
    additional_data.mapping_update_flags_faces_by_cells =
      additional_data.mapping_update_flags_inner_faces |
      additional_data.mapping_update_flags_boundary_faces;
  }

  namespace internal
  {
    template <typename Number>
    struct LocalCSR
    {
      LocalCSR()
        : row{0}
      {}

      std::vector<unsigned int> row_lid_to_gid;
      std::vector<unsigned int> row;
      std::vector<unsigned int> col;
      std::vector<Number>       val;
    };

    template <int dim,
              int fe_degree,
              int n_q_points_1d,
              int n_components,
              typename Number,
              typename VectorizedArrayType>
    class ComputeDiagonalHelper
    {
    public:
      static const unsigned int n_lanes = VectorizedArrayType::size();

      ComputeDiagonalHelper(FEEvaluation<dim,
                                         fe_degree,
                                         n_q_points_1d,
                                         n_components,
                                         Number,
                                         VectorizedArrayType> &phi)
        : phi(phi)
      {}

      void
      reinit(const unsigned int cell)
      {
        this->phi.reinit(cell);
        // STEP 1: get relevant information from FEEvaluation
        const auto &       dof_info        = phi.get_dof_info();
        const unsigned int n_fe_components = dof_info.start_components.back();
        const unsigned int dofs_per_component = phi.dofs_per_component;
        const auto &       matrix_free        = phi.get_matrix_free();

        // if we have a block vector with components with the same DoFHandler,
        // each component is described with same set of constraints and
        // we consider the shift in components only during access of the global
        // vector
        const unsigned int first_selected_component =
          n_fe_components == 1 ? 0 : phi.get_first_selected_component();

        const unsigned int n_lanes_filled =
          matrix_free.n_active_entries_per_cell_batch(cell);

        std::array<const unsigned int *, n_lanes> dof_indices{};
        {
          for (unsigned int v = 0; v < n_lanes_filled; ++v)
            dof_indices[v] =
              dof_info.dof_indices.data() +
              dof_info
                .row_starts[(cell * n_lanes + v) * n_fe_components +
                            first_selected_component]
                .first;
        }

        // STEP 2: setup CSR storage of transposed locally-relevant
        //   constraint matrix
        c_pools = std::array<internal::LocalCSR<Number>, n_lanes>();

        for (unsigned int v = 0; v < n_lanes_filled; ++v)
          {
            unsigned int index_indicators, next_index_indicators;

            index_indicators =
              dof_info
                .row_starts[(cell * n_lanes + v) * n_fe_components +
                            first_selected_component]
                .second;
            next_index_indicators =
              dof_info
                .row_starts[(cell * n_lanes + v) * n_fe_components +
                            first_selected_component + 1]
                .second;

            // STEP 2a: setup locally-relevant constraint matrix in a
            //   coordinate list (COO)
            std::vector<std::tuple<unsigned int, unsigned int, Number>>
              locally_relevant_constrains; // (constrained local index,
                                           // global index of dof which
                                           // constrains, weight)

            if (n_components == 1 || n_fe_components == 1)
              {
                unsigned int ind_local = 0;
                for (; index_indicators != next_index_indicators;
                     ++index_indicators, ++ind_local)
                  {
                    const std::pair<unsigned short, unsigned short> indicator =
                      dof_info.constraint_indicator[index_indicators];

                    for (unsigned int j = 0; j < indicator.first;
                         ++j, ++ind_local)
                      locally_relevant_constrains.emplace_back(
                        ind_local, dof_indices[v][j], 1.0);

                    dof_indices[v] += indicator.first;

                    const Number *data_val =
                      matrix_free.constraint_pool_begin(indicator.second);
                    const Number *end_pool =
                      matrix_free.constraint_pool_end(indicator.second);

                    for (; data_val != end_pool; ++data_val, ++dof_indices[v])
                      locally_relevant_constrains.emplace_back(ind_local,
                                                               *dof_indices[v],
                                                               *data_val);
                  }

                AssertIndexRange(ind_local, dofs_per_component + 1);

                for (; ind_local < dofs_per_component;
                     ++dof_indices[v], ++ind_local)
                  locally_relevant_constrains.emplace_back(ind_local,
                                                           *dof_indices[v],
                                                           1.0);
              }
            else
              {
                // case with vector-valued finite elements where all
                // components are included in one single vector. Assumption:
                // first come all entries to the first component, then all
                // entries to the second one, and so on. This is ensured by
                // the way MatrixFree reads out the indices.
                for (unsigned int comp = 0; comp < n_components; ++comp)
                  {
                    unsigned int ind_local = 0;

                    // check whether there is any constraint on the current
                    // cell
                    for (; index_indicators != next_index_indicators;
                         ++index_indicators, ++ind_local)
                      {
                        const std::pair<unsigned short, unsigned short>
                          indicator =
                            dof_info.constraint_indicator[index_indicators];

                        // run through values up to next constraint
                        for (unsigned int j = 0; j < indicator.first;
                             ++j, ++ind_local)
                          locally_relevant_constrains.emplace_back(
                            comp * dofs_per_component + ind_local,
                            dof_indices[v][j],
                            1.0);
                        dof_indices[v] += indicator.first;

                        const Number *data_val =
                          matrix_free.constraint_pool_begin(indicator.second);
                        const Number *end_pool =
                          matrix_free.constraint_pool_end(indicator.second);

                        for (; data_val != end_pool;
                             ++data_val, ++dof_indices[v])
                          locally_relevant_constrains.emplace_back(
                            comp * dofs_per_component + ind_local,
                            *dof_indices[v],
                            *data_val);
                      }

                    AssertIndexRange(ind_local, dofs_per_component + 1);

                    // get the dof values past the last constraint
                    for (; ind_local < dofs_per_component;
                         ++dof_indices[v], ++ind_local)
                      locally_relevant_constrains.emplace_back(
                        comp * dofs_per_component + ind_local,
                        *dof_indices[v],
                        1.0);

                    if (comp + 1 < n_components)
                      {
                        next_index_indicators =
                          dof_info
                            .row_starts[(cell * n_lanes + v) * n_fe_components +
                                        first_selected_component + comp + 2]
                            .second;
                      }
                  }
              }
            // STEP 2b: sort and make unique

            // sort vector
            std::sort(locally_relevant_constrains.begin(),
                      locally_relevant_constrains.end(),
                      [](const auto &a, const auto &b) {
                        if (std::get<0>(a) < std::get<0>(b))
                          return true;
                        return (std::get<0>(a) == std::get<0>(b)) &&
                               (std::get<1>(a) < std::get<1>(b));
                      });

            // make sure that all entries are unique
            locally_relevant_constrains.erase(
              unique(locally_relevant_constrains.begin(),
                     locally_relevant_constrains.end(),
                     [](const auto &a, const auto &b) {
                       return (std::get<1>(a) == std::get<1>(b)) &&
                              (std::get<0>(a) == std::get<0>(b));
                     }),
              locally_relevant_constrains.end());

            // STEP 2c: apply hanging-node constraints
            if (dof_info.hanging_node_constraint_masks.size() > 0 &&
                dof_info.hanging_node_constraint_masks_comp.size() > 0 &&
                dof_info
                  .hanging_node_constraint_masks_comp[phi.get_active_fe_index()]
                                                     [first_selected_component])
              {
                const auto mask =
                  dof_info.hanging_node_constraint_masks[cell * n_lanes + v];

                // cell has hanging nodes
                if (mask != dealii::internal::MatrixFreeFunctions::
                              unconstrained_compressed_constraint_kind)
                  {
                    // check if hanging node internpolation matrix has been set
                    // up
                    if (locally_relevant_constrains_hn_map.find(mask) ==
                        locally_relevant_constrains_hn_map.end())
                      {
                        // 1) collect hanging-node constraints for cell assuming
                        // scalar finite element
                        AlignedVector<VectorizedArrayType> values_dofs(
                          dofs_per_component);

                        std::array<dealii::internal::MatrixFreeFunctions::
                                     compressed_constraint_kind,
                                   VectorizedArrayType::size()>
                          constraint_mask;
                        constraint_mask.fill(
                          dealii::internal::MatrixFreeFunctions::
                            unconstrained_compressed_constraint_kind);

                        constraint_mask[0] = mask;

                        std::vector<
                          std::tuple<unsigned int, unsigned int, Number>>
                          locally_relevant_constrains_hn;

                        for (unsigned int i = 0; i < dofs_per_component; ++i)
                          {
                            for (unsigned int j = 0; j < dofs_per_component;
                                 ++j)
                              values_dofs[j][0] = static_cast<Number>(i == j);

                            dealii::internal::FEEvaluationHangingNodesFactory<
                              dim,
                              Number,
                              VectorizedArrayType>::apply(1,
                                                          phi.get_shape_info()
                                                            .data.front()
                                                            .fe_degree,
                                                          phi.get_shape_info(),
                                                          false,
                                                          constraint_mask,
                                                          values_dofs.data());

                            for (unsigned int j = 0; j < dofs_per_component;
                                 ++j)
                              if (1e-10 < std::abs(values_dofs[j][0]) &&
                                  (j != i ||
                                   1e-10 < std::abs(values_dofs[j][0] - 1.0)))
                                locally_relevant_constrains_hn.emplace_back(
                                  j, i, values_dofs[j][0]);
                          }


                        std::sort(locally_relevant_constrains_hn.begin(),
                                  locally_relevant_constrains_hn.end(),
                                  [](const auto &a, const auto &b) {
                                    if (std::get<0>(a) < std::get<0>(b))
                                      return true;
                                    return (std::get<0>(a) == std::get<0>(b)) &&
                                           (std::get<1>(a) < std::get<1>(b));
                                  });

                        // 1b) extend for multiple components
                        const unsigned int n_hn_constraints =
                          locally_relevant_constrains_hn.size();
                        locally_relevant_constrains_hn.resize(n_hn_constraints *
                                                              n_components);

                        for (unsigned int c = 0; c < n_components; ++c)
                          for (unsigned int i = 0; i < n_hn_constraints; ++i)
                            locally_relevant_constrains_hn[c *
                                                             n_hn_constraints +
                                                           i] =
                              std::tuple<unsigned int, unsigned int, Number>{
                                std::get<0>(locally_relevant_constrains_hn[i]) +
                                  c * dofs_per_component,
                                std::get<1>(locally_relevant_constrains_hn[i]) +
                                  c * dofs_per_component,
                                std::get<2>(locally_relevant_constrains_hn[i])};


                        locally_relevant_constrains_hn_map[mask] =
                          locally_relevant_constrains_hn;
                      }

                    const auto &locally_relevant_constrains_hn =
                      locally_relevant_constrains_hn_map[mask];

                    // 2) perform vmult with other constraints
                    std::vector<std::tuple<unsigned int, unsigned int, Number>>
                      locally_relevant_constrains_temp;

                    for (unsigned int i = 0;
                         i < dofs_per_component * n_components;
                         ++i)
                      {
                        const auto lower_bound_fu = [](const auto &a,
                                                       const auto &b) {
                          return std::get<0>(a) < b;
                        };

                        const auto upper_bound_fu = [](const auto &a,
                                                       const auto &b) {
                          return a < std::get<0>(b);
                        };

                        const auto i_begin = std::lower_bound(
                          locally_relevant_constrains_hn.begin(),
                          locally_relevant_constrains_hn.end(),
                          i,
                          lower_bound_fu);
                        const auto i_end = std::upper_bound(
                          locally_relevant_constrains_hn.begin(),
                          locally_relevant_constrains_hn.end(),
                          i,
                          upper_bound_fu);

                        if (i_begin == i_end)
                          {
                            // dof is not constrained by hanging-node constraint
                            // (identity matrix): simply copy constraints
                            const auto j_begin = std::lower_bound(
                              locally_relevant_constrains.begin(),
                              locally_relevant_constrains.end(),
                              i,
                              lower_bound_fu);
                            const auto j_end = std::upper_bound(
                              locally_relevant_constrains.begin(),
                              locally_relevant_constrains.end(),
                              i,
                              upper_bound_fu);

                            for (auto v = j_begin; v != j_end; ++v)
                              locally_relevant_constrains_temp.emplace_back(*v);
                          }
                        else
                          {
                            // dof is constrained: build transitive closure
                            for (auto v0 = i_begin; v0 != i_end; ++v0)
                              {
                                const auto j_begin = std::lower_bound(
                                  locally_relevant_constrains.begin(),
                                  locally_relevant_constrains.end(),
                                  std::get<1>(*v0),
                                  lower_bound_fu);
                                const auto j_end = std::upper_bound(
                                  locally_relevant_constrains.begin(),
                                  locally_relevant_constrains.end(),
                                  std::get<1>(*v0),
                                  upper_bound_fu);

                                for (auto v1 = j_begin; v1 != j_end; ++v1)
                                  locally_relevant_constrains_temp.emplace_back(
                                    std::get<0>(*v0),
                                    std::get<1>(*v1),
                                    std::get<2>(*v0) * std::get<2>(*v1));
                              }
                          }
                      }

                    locally_relevant_constrains =
                      locally_relevant_constrains_temp;
                  }
              }

            // STEP 2d: transpose COO
            std::sort(locally_relevant_constrains.begin(),
                      locally_relevant_constrains.end(),
                      [](const auto &a, const auto &b) {
                        if (std::get<1>(a) < std::get<1>(b))
                          return true;
                        return (std::get<1>(a) == std::get<1>(b)) &&
                               (std::get<0>(a) < std::get<0>(b));
                      });

            // STEP 2e: translate COO to CRS
            auto &c_pool = c_pools[v];
            {
              if (locally_relevant_constrains.size() > 0)
                c_pool.row_lid_to_gid.emplace_back(
                  std::get<1>(locally_relevant_constrains.front()));
              for (const auto &j : locally_relevant_constrains)
                {
                  if (c_pool.row_lid_to_gid.back() != std::get<1>(j))
                    {
                      c_pool.row_lid_to_gid.push_back(std::get<1>(j));
                      c_pool.row.push_back(c_pool.val.size());
                    }

                  c_pool.col.emplace_back(std::get<0>(j));
                  c_pool.val.emplace_back(std::get<2>(j));
                }

              if (c_pool.val.size() > 0)
                c_pool.row.push_back(c_pool.val.size());
            }
          }
        // STEP 3: compute element matrix A_e, apply
        //   locally-relevant constraints C_e^T * A_e * C_e, and get the
        //   the diagonal entry
        //     (C_e^T * A_e * C_e)(i,i)
        //   or
        //     C_e^T(i,:) * A_e * C_e(:,i).
        //
        //   Since, we compute the element matrix column-by-column and as a
        //   result never actually have the full element matrix, we actually
        //   perform following steps:
        //    1) loop over all columns of the element matrix
        //     a) compute column i
        //     b) compute for each j (rows of C_e^T):
        //          (C_e^T(j,:) * A_e(:,i)) * C_e(i,j)
        //       or
        //          (C_e^T(j,:) * A_e(:,i)) * C_e^T(j,i)
        //       This gives a contribution the j-th entry of the
        //       locally-relevant diagonal and comprises the multiplication
        //       by the locally-relevant constraint matrix from the left and
        //       the right. There is no contribution to the j-th vector
        //       entry if the j-th row of C_e^T is empty or C_e^T(j,i) is
        //       zero.

        // set size locally-relevant diagonal
        for (unsigned int v = 0; v < n_lanes_filled; ++v)
          diagonals_local_constrained[v].assign(
            c_pools[v].row_lid_to_gid.size() *
              (n_fe_components == 1 ? n_components : 1),
            Number(0.0));
      }

      void
      prepare_basis_vector(const unsigned int i)
      {
        this->i = i;

        // compute i-th column of element stiffness matrix:
        // this could be simply performed as done at the moment with
        // matrix-free operator evaluation applied to a ith-basis vector
        for (unsigned int j = 0; j < phi.dofs_per_cell; ++j)
          phi.begin_dof_values()[j] = static_cast<Number>(i == j);
      }

      void
      submit()
      {
        // if we have a block vector with components with the same DoFHandler,
        // we need to figure out which component and which DoF within the
        // component are we currently considering
        const unsigned int n_fe_components =
          phi.get_dof_info().start_components.back();
        const unsigned int comp =
          n_fe_components == 1 ? i / phi.dofs_per_component : 0;
        const unsigned int i_comp =
          n_fe_components == 1 ? (i % phi.dofs_per_component) : i;

        // apply local constraint matrix from left and from right:
        // loop over all rows of transposed constrained matrix
        for (unsigned int v = 0;
             v < phi.get_matrix_free().n_active_entries_per_cell_batch(
                   phi.get_current_cell_index());
             ++v)
          {
            const auto &c_pool = c_pools[v];

            for (unsigned int j = 0; j < c_pool.row.size() - 1; ++j)
              {
                // check if the result will be zero, so that we can skip
                // the following computations -> binary search
                const auto scale_iterator =
                  std::lower_bound(c_pool.col.begin() + c_pool.row[j],
                                   c_pool.col.begin() + c_pool.row[j + 1],
                                   i_comp);

                // explanation: j-th row of C_e^T is empty (see above)
                if (scale_iterator == c_pool.col.begin() + c_pool.row[j + 1])
                  continue;

                // explanation: C_e^T(j,i) is zero (see above)
                if (*scale_iterator != i_comp)
                  continue;

                // apply constraint matrix from the left
                Number temp = 0.0;
                for (unsigned int k = c_pool.row[j]; k < c_pool.row[j + 1]; ++k)
                  temp += c_pool.val[k] *
                          phi.begin_dof_values()[comp * phi.dofs_per_component +
                                                 c_pool.col[k]][v];

                // apply constraint matrix from the right
                diagonals_local_constrained
                  [v][j + comp * c_pools[v].row_lid_to_gid.size()] +=
                  temp *
                  c_pool.val[std::distance(c_pool.col.begin(), scale_iterator)];
              }
          }
      }

      template <typename VectorType>
      inline void
      distribute_local_to_global(
        std::array<VectorType *, n_components> &diagonal_global)
      {
        // STEP 4: assembly results: add into global vector
        const unsigned int n_fe_components =
          phi.get_dof_info().start_components.back();

        for (unsigned int v = 0;
             v < phi.get_matrix_free().n_active_entries_per_cell_batch(
                   phi.get_current_cell_index());
             ++v)
          // if we have a block vector with components with the same
          // DoFHandler, we need to loop over all components manually and
          // need to apply the correct shift
          for (unsigned int j = 0; j < c_pools[v].row.size() - 1; ++j)
            for (unsigned int comp = 0;
                 comp < (n_fe_components == 1 ?
                           static_cast<unsigned int>(n_components) :
                           1);
                 ++comp)
              ::dealii::internal::vector_access_add(
                *diagonal_global[n_fe_components == 1 ? comp : 0],
                c_pools[v].row_lid_to_gid[j],
                diagonals_local_constrained
                  [v][j + comp * c_pools[v].row_lid_to_gid.size()]);
      }

    private:
      FEEvaluation<dim,
                   fe_degree,
                   n_q_points_1d,
                   n_components,
                   Number,
                   VectorizedArrayType> &phi;

      unsigned int i;

      std::array<internal::LocalCSR<Number>, n_lanes> c_pools;

      // local storage: buffer so that we access the global vector once
      // note: may be larger then dofs_per_cell in the presence of
      // constraints!
      std::array<std::vector<Number>, n_lanes> diagonals_local_constrained;

      std::map<
        dealii::internal::MatrixFreeFunctions::compressed_constraint_kind,
        std::vector<std::tuple<unsigned int, unsigned int, Number>>>
        locally_relevant_constrains_hn_map;
    };

  } // namespace internal

  template <int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename VectorType>
  void
  compute_diagonal(
    const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
    VectorType &                                        diagonal_global,
    const std::function<void(FEEvaluation<dim,
                                          fe_degree,
                                          n_q_points_1d,
                                          n_components,
                                          Number,
                                          VectorizedArrayType> &)> &local_vmult,
    const unsigned int                                              dof_no,
    const unsigned int                                              quad_no,
    const unsigned int first_selected_component)
  {
    int dummy = 0;

    std::array<typename dealii::internal::BlockVectorSelector<
                 VectorType,
                 IsBlockVector<VectorType>::value>::BaseVectorType *,
               n_components>
      diagonal_global_components;

    for (unsigned int d = 0; d < n_components; ++d)
      diagonal_global_components[d] = dealii::internal::
        BlockVectorSelector<VectorType, IsBlockVector<VectorType>::value>::
          get_vector_component(diagonal_global, d + first_selected_component);

    const auto &dof_info = matrix_free.get_dof_info(dof_no);

    if (dof_info.start_components.back() == 1)
      for (unsigned int comp = 0; comp < n_components; ++comp)
        {
          Assert(diagonal_global_components[comp] != nullptr,
                 ExcMessage("The finite element underlying this FEEvaluation "
                            "object is scalar, but you requested " +
                            std::to_string(n_components) +
                            " components via the template argument in "
                            "FEEvaluation. In that case, you must pass an "
                            "std::vector<VectorType> or a BlockVector to " +
                            "read_dof_values and distribute_local_to_global."));
          dealii::internal::check_vector_compatibility(
            *diagonal_global_components[comp], matrix_free, dof_info);
        }
    else
      {
        dealii::internal::check_vector_compatibility(
          *diagonal_global_components[0], matrix_free, dof_info);
      }

    matrix_free.template cell_loop<VectorType, int>(
      [&](const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
          VectorType &,
          const int &,
          const std::pair<unsigned int, unsigned int> &range) mutable {
        FEEvaluation<dim,
                     fe_degree,
                     n_q_points_1d,
                     n_components,
                     Number,
                     VectorizedArrayType>
          phi(matrix_free, range, dof_no, quad_no, first_selected_component);

        internal::ComputeDiagonalHelper<dim,
                                        fe_degree,
                                        n_q_points_1d,
                                        n_components,
                                        Number,
                                        VectorizedArrayType>
          helper(phi);

        for (unsigned int cell = range.first; cell < range.second; ++cell)
          {
            helper.reinit(cell);

            for (unsigned int i = 0; i < phi.dofs_per_cell; ++i)
              {
                helper.prepare_basis_vector(i);
                local_vmult(phi);
                helper.submit();
              }

            helper.distribute_local_to_global(diagonal_global_components);
          }
      },
      diagonal_global,
      dummy,
      false);
  }

  template <typename CLASS,
            int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename VectorType>
  void
  compute_diagonal(
    const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
    VectorType &                                        diagonal_global,
    void (CLASS::*cell_operation)(FEEvaluation<dim,
                                               fe_degree,
                                               n_q_points_1d,
                                               n_components,
                                               Number,
                                               VectorizedArrayType> &) const,
    const CLASS *      owning_class,
    const unsigned int dof_no,
    const unsigned int quad_no,
    const unsigned int first_selected_component)
  {
    compute_diagonal<dim,
                     fe_degree,
                     n_q_points_1d,
                     n_components,
                     Number,
                     VectorizedArrayType,
                     VectorType>(
      matrix_free,
      diagonal_global,
      [&](auto &feeval) { (owning_class->*cell_operation)(feeval); },
      dof_no,
      quad_no,
      first_selected_component);
  }

  namespace internal
  {
    /**
     * If value type of matrix and constrains equals, return a reference
     * to the given AffineConstraint instance.
     */
    template <typename MatrixType,
              typename Number,
              std::enable_if_t<std::is_same<
                typename std::remove_const<typename std::remove_reference<
                  typename MatrixType::value_type>::type>::type,
                typename std::remove_const<typename std::remove_reference<
                  Number>::type>::type>::value> * = nullptr>
    const AffineConstraints<typename MatrixType::value_type> &
    create_new_affine_constraints_if_needed(
      const MatrixType &,
      const AffineConstraints<Number> &constraints,
      std::unique_ptr<AffineConstraints<typename MatrixType::value_type>> &)
    {
      return constraints;
    }

    /**
     * If value type of matrix and constrains do not equal, a new
     * AffineConstraint instance with the value type of the matrix is
     * created and a reference to it is returned.
     */
    template <typename MatrixType,
              typename Number,
              std::enable_if_t<!std::is_same<
                typename std::remove_const<typename std::remove_reference<
                  typename MatrixType::value_type>::type>::type,
                typename std::remove_const<typename std::remove_reference<
                  Number>::type>::type>::value> * = nullptr>
    const AffineConstraints<typename MatrixType::value_type> &
    create_new_affine_constraints_if_needed(
      const MatrixType &,
      const AffineConstraints<Number> &constraints,
      std::unique_ptr<AffineConstraints<typename MatrixType::value_type>>
        &new_constraints)
    {
      new_constraints =
        std::make_unique<AffineConstraints<typename MatrixType::value_type>>();
      new_constraints->copy_from(constraints);

      return *new_constraints;
    }
  } // namespace internal

  template <int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename MatrixType>
  void
  compute_matrix(
    const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
    const AffineConstraints<Number> &                   constraints_in,
    MatrixType &                                        matrix,
    const std::function<void(FEEvaluation<dim,
                                          fe_degree,
                                          n_q_points_1d,
                                          n_components,
                                          Number,
                                          VectorizedArrayType> &)> &local_vmult,
    const unsigned int                                              dof_no,
    const unsigned int                                              quad_no,
    const unsigned int first_selected_component)
  {
    std::unique_ptr<AffineConstraints<typename MatrixType::value_type>>
      constraints_for_matrix;
    const AffineConstraints<typename MatrixType::value_type> &constraints =
      internal::create_new_affine_constraints_if_needed(matrix,
                                                        constraints_in,
                                                        constraints_for_matrix);

    matrix_free.template cell_loop<MatrixType, MatrixType>(
      [&](const auto &, auto &dst, const auto &, const auto range) {
        FEEvaluation<dim,
                     fe_degree,
                     n_q_points_1d,
                     n_components,
                     Number,
                     VectorizedArrayType>
          integrator(
            matrix_free, range, dof_no, quad_no, first_selected_component);

        const unsigned int dofs_per_cell = integrator.dofs_per_cell;

        std::vector<types::global_dof_index> dof_indices(dofs_per_cell);
        std::vector<types::global_dof_index> dof_indices_mf(dofs_per_cell);

        std::array<FullMatrix<typename MatrixType::value_type>,
                   VectorizedArrayType::size()>
          matrices;

        std::fill_n(matrices.begin(),
                    VectorizedArrayType::size(),
                    FullMatrix<typename MatrixType::value_type>(dofs_per_cell,
                                                                dofs_per_cell));

        const auto lexicographic_numbering =
          matrix_free
            .get_shape_info(dof_no,
                            quad_no,
                            first_selected_component,
                            integrator.get_active_fe_index(),
                            integrator.get_active_quadrature_index())
            .lexicographic_numbering;

        for (auto cell = range.first; cell < range.second; ++cell)
          {
            integrator.reinit(cell);

            const unsigned int n_filled_lanes =
              matrix_free.n_active_entries_per_cell_batch(cell);

            for (unsigned int v = 0; v < n_filled_lanes; ++v)
              matrices[v] = 0.0;

            for (unsigned int j = 0; j < dofs_per_cell; ++j)
              {
                for (unsigned int i = 0; i < dofs_per_cell; ++i)
                  integrator.begin_dof_values()[i] =
                    static_cast<Number>(i == j);

                local_vmult(integrator);

                for (unsigned int i = 0; i < dofs_per_cell; ++i)
                  for (unsigned int v = 0; v < n_filled_lanes; ++v)
                    matrices[v](i, j) = integrator.begin_dof_values()[i][v];
              }

            for (unsigned int v = 0; v < n_filled_lanes; ++v)
              {
                const auto cell_v =
                  matrix_free.get_cell_iterator(cell, v, dof_no);

                if (matrix_free.get_mg_level() != numbers::invalid_unsigned_int)
                  cell_v->get_mg_dof_indices(dof_indices);
                else
                  cell_v->get_dof_indices(dof_indices);

                for (unsigned int j = 0; j < dof_indices.size(); ++j)
                  dof_indices_mf[j] = dof_indices[lexicographic_numbering[j]];

                constraints.distribute_local_to_global(matrices[v],
                                                       dof_indices_mf,
                                                       dst);
              }
          }
      },
      matrix,
      matrix);

    matrix.compress(VectorOperation::add);
  }

  template <typename CLASS,
            int dim,
            int fe_degree,
            int n_q_points_1d,
            int n_components,
            typename Number,
            typename VectorizedArrayType,
            typename MatrixType>
  void
  compute_matrix(
    const MatrixFree<dim, Number, VectorizedArrayType> &matrix_free,
    const AffineConstraints<Number> &                   constraints,
    MatrixType &                                        matrix,
    void (CLASS::*cell_operation)(FEEvaluation<dim,
                                               fe_degree,
                                               n_q_points_1d,
                                               n_components,
                                               Number,
                                               VectorizedArrayType> &) const,
    const CLASS *      owning_class,
    const unsigned int dof_no,
    const unsigned int quad_no,
    const unsigned int first_selected_component)
  {
    compute_matrix<dim,
                   fe_degree,
                   n_q_points_1d,
                   n_components,
                   Number,
                   VectorizedArrayType,
                   MatrixType>(
      matrix_free,
      constraints,
      matrix,
      [&](auto &feeval) { (owning_class->*cell_operation)(feeval); },
      dof_no,
      quad_no,
      first_selected_component);
  }

#endif // DOXYGEN

} // namespace MatrixFreeTools

DEAL_II_NAMESPACE_CLOSE


#endif
