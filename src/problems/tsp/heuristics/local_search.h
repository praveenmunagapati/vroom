#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

/*

This file is part of VROOM.

Copyright (c) 2015-2017, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include <list>
#include <numeric>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/log/trivial.hpp>

#include "../../../structures/abstract/matrix.h"
#include "../../../structures/typedefs.h"

class local_search {
private:
  const matrix<distance_t>& _matrix;
  const bool _is_symmetric_matrix;
  std::vector<index_t> _edges;
  unsigned _nb_threads;
  std::vector<index_t> _rank_limits;
  std::vector<index_t> _sym_two_opt_rank_limits;

public:
  local_search(const matrix<distance_t>& matrix,
               bool is_symmetric_matrix,
               const std::list<index_t>& tour,
               unsigned nb_threads);

  distance_t relocate_step();

  distance_t perform_all_relocate_steps();

  distance_t avoid_loop_step();

  distance_t perform_all_avoid_loop_steps();

  distance_t two_opt_step();

  distance_t asym_two_opt_step();

  distance_t perform_all_two_opt_steps();

  distance_t perform_all_asym_two_opt_steps();

  distance_t or_opt_step();

  distance_t perform_all_or_opt_steps();

  std::list<index_t> get_tour(index_t first_index) const;
};

#endif
