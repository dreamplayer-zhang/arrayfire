/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <Array.hpp>
#include <common/SparseArray.hpp>
#include <sparse.hpp>
#include <optypes.hpp>

namespace cpu
{
// These two functions cannot be overloaded by return type.
// So have to give them separate names.
template<typename T, af_op_t op>
Array<T> arithOpD(const common::SparseArray<T> &lhs, const Array<T> &rhs,
                 const bool reverse = false);

template<typename T, af_op_t op>
common::SparseArray<T> arithOpS(const common::SparseArray<T> &lhs, const Array<T> &rhs,
                                const bool reverse = false);

template<typename T, af_op_t op>
common::SparseArray<T> arithOp(const common::SparseArray<T> &lhs,
                               const common::SparseArray<T> &rhs);
}
