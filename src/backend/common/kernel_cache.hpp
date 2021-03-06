/*******************************************************
 * Copyright (c) 2020, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#if !defined(AF_CPU)

#include <Kernel.hpp>
#include <backend.hpp>
#include <common/TemplateTypename.hpp>

#include <string>
#include <vector>

namespace common {

/// \brief Find/Create-Cache a Kernel that fits the given criteria
///
/// This function takes in two vectors of strings apart from the main Kernel
/// name, match criteria, to find a suitable kernel in the Kernel cache. It
/// builds and caches a new Kernel object if one isn't found in the cache.
///
/// The paramter \p key has to be the unique name for a given kernel.
/// The key has to be present in one of the entries of KernelMap defined in
/// the header EnqueueArgs.hpp.
///
/// The parameter \p templateArgs is a list of stringified template arguments of
/// the kernel. These strings are used to generate the template instantiation
/// expression of the kernel during compilation stage. This string is used as
/// key to kernel cache map. At some point in future, the idea is to use these
/// instantiation strings to generate template instatiations in online compiler.
///
/// The paramter \p compileOpts is a list of strings that lets you add
/// definitions such as `-D<NAME>` or `-D<NAME>=<VALUE>` to the compiler. To
/// enable easy stringification of variables into their definition equation,
/// three helper macros are provided: TemplateArg, DefineKey and DefineValue.
///
/// Example Usage: transpose
///
/// \code
/// static const std::string src(transpose_cuh, transpose_cuh_len);
/// auto transpose = getKernel("cuda::transpose", {src},
///         {
///           TemplateTypename<T>(),
///           TemplateArg(conjugate),
///           TemplateArg(is32multiple)
///         },
///         {
///           DefineValue(THREADS_Y) // Results in a definition
///                                  // "-D THREADS_Y=<Value of THREADS_Y>"
///           DefineKeyValue(DIMY, threads_y)  // Results in a definition
///                                            // "-D DIMY=<Value of threads_y>"
///         }
///         );
/// \endcode
///
/// \param[in] kernelName is the name of the kernel qualified as kernel in code
/// \param[in] sources is the list of source strings to be compiled if required
/// \param[in] templateArgs is a vector of strings containing stringified names
///            of the template arguments of kernel to be compiled.
/// \param[in] compileOpts is a vector of strings that enables the user to
///            add definitions such as `-D<NAME>` or `-D<NAME>=<VALUE>` for
///            the kernel compilation.
///
detail::Kernel findKernel(const std::string& kernelName,
                          const std::vector<std::string>& sources,
                          const std::vector<TemplateArg>& templateArgs,
                          const std::vector<std::string>& compileOpts = {});

}  // namespace common

#endif
