/*******************************************************
 * Copyright (c) 2017, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <Param.hpp>
#include <common/dispatch.hpp>
#include <common/kernel_cache.hpp>
#include <debug_opencl.hpp>
#include <kernel_headers/nonmax_suppression.hpp>
#include <kernel_headers/trace_edge.hpp>
#include <memory.hpp>
#include <traits.hpp>

#include <string>
#include <vector>

namespace opencl {
namespace kernel {
constexpr int THREADS_X = 16;
constexpr int THREADS_Y = 16;

template<typename T>
void nonMaxSuppression(Param output, const Param magnitude, const Param dx,
                       const Param dy) {
    using cl::EnqueueArgs;
    using cl::NDRange;
    using std::string;
    using std::vector;

    static const string src(nonmax_suppression_cl, nonmax_suppression_cl_len);
    vector<string> compileOpts = {
        DefineKeyValue(T, dtype_traits<T>::getName()),
        DefineKeyValue(SHRD_MEM_HEIGHT, THREADS_X + 2),
        DefineKeyValue(SHRD_MEM_WIDTH, THREADS_Y + 2),
    };
    compileOpts.emplace_back(getTypeBuildDefinition<T>());

    auto nonMaxOp = common::findKernel("nonMaxSuppressionKernel", {src},
                                       {TemplateTypename<T>()}, compileOpts);

    NDRange threads(kernel::THREADS_X, kernel::THREADS_Y, 1);

    // Launch only threads to process non-border pixels
    int blk_x = divup(magnitude.info.dims[0] - 2, threads[0]);
    int blk_y = divup(magnitude.info.dims[1] - 2, threads[1]);

    // launch batch * blk_x blocks along x dimension
    NDRange global(blk_x * magnitude.info.dims[2] * threads[0],
                   blk_y * magnitude.info.dims[3] * threads[1], 1);

    nonMaxOp(EnqueueArgs(getQueue(), global, threads), *output.data,
             output.info, *magnitude.data, magnitude.info, *dx.data, dx.info,
             *dy.data, dy.info, blk_x, blk_y);
    CL_DEBUG_FINISH(getQueue());
}

template<typename T>
void initEdgeOut(Param output, const Param strong, const Param weak) {
    using cl::EnqueueArgs;
    using cl::NDRange;
    using std::string;
    using std::vector;

    static const string src(trace_edge_cl, trace_edge_cl_len);

    vector<string> compileOpts = {
        DefineKeyValue(T, dtype_traits<T>::getName()),
        DefineKey(INIT_EDGE_OUT),
    };
    compileOpts.emplace_back(getTypeBuildDefinition<T>());

    auto initOp = common::findKernel("initEdgeOutKernel", {src},
                                     {TemplateTypename<T>()}, compileOpts);

    NDRange threads(kernel::THREADS_X, kernel::THREADS_Y, 1);

    // Launch only threads to process non-border pixels
    int blk_x = divup(strong.info.dims[0] - 2, threads[0]);
    int blk_y = divup(strong.info.dims[1] - 2, threads[1]);

    // launch batch * blk_x blocks along x dimension
    NDRange global(blk_x * strong.info.dims[2] * threads[0],
                   blk_y * strong.info.dims[3] * threads[1], 1);

    initOp(EnqueueArgs(getQueue(), global, threads), *output.data, output.info,
           *strong.data, strong.info, *weak.data, weak.info, blk_x, blk_y);

    CL_DEBUG_FINISH(getQueue());
}

template<typename T>
void suppressLeftOver(Param output) {
    using cl::EnqueueArgs;
    using cl::NDRange;
    using std::string;
    using std::vector;

    static const string src(trace_edge_cl, trace_edge_cl_len);

    vector<string> compileOpts = {
        DefineKeyValue(T, dtype_traits<T>::getName()),
        DefineKey(SUPPRESS_LEFT_OVER),
    };
    compileOpts.emplace_back(getTypeBuildDefinition<T>());

    auto finalOp = common::findKernel("suppressLeftOverKernel", {src},
                                      {TemplateTypename<T>()}, compileOpts);

    NDRange threads(kernel::THREADS_X, kernel::THREADS_Y, 1);

    // Launch only threads to process non-border pixels
    int blk_x = divup(output.info.dims[0] - 2, threads[0]);
    int blk_y = divup(output.info.dims[1] - 2, threads[1]);

    // launch batch * blk_x blocks along x dimension
    NDRange global(blk_x * output.info.dims[2] * threads[0],
                   blk_y * output.info.dims[3] * threads[1], 1);

    finalOp(EnqueueArgs(getQueue(), global, threads), *output.data, output.info,
            blk_x, blk_y);
    CL_DEBUG_FINISH(getQueue());
}

template<typename T>
void edgeTrackingHysteresis(Param output, const Param strong,
                            const Param weak) {
    using cl::EnqueueArgs;
    using cl::NDRange;
    using std::string;
    using std::vector;

    static const string src(trace_edge_cl, trace_edge_cl_len);

    vector<string> compileOpts = {
        DefineKeyValue(T, dtype_traits<T>::getName()),
        DefineKey(EDGE_TRACER),
        DefineKeyValue(SHRD_MEM_HEIGHT, THREADS_X + 2),
        DefineKeyValue(SHRD_MEM_WIDTH, THREADS_Y + 2),
        DefineKeyValue(TOTAL_NUM_THREADS, THREADS_X * THREADS_Y),
    };
    compileOpts.emplace_back(getTypeBuildDefinition<T>());

    auto edgeTraceOp = common::findKernel("edgeTrackKernel", {src},
                                          {TemplateTypename<T>()}, compileOpts);

    NDRange threads(kernel::THREADS_X, kernel::THREADS_Y);

    // Launch only threads to process non-border pixels
    int blk_x = divup(weak.info.dims[0] - 2, threads[0]);
    int blk_y = divup(weak.info.dims[1] - 2, threads[1]);

    // launch batch * blk_x blocks along x dimension
    NDRange global(blk_x * weak.info.dims[2] * threads[0],
                   blk_y * weak.info.dims[3] * threads[1], 1);

    initEdgeOut<T>(output, strong, weak);

    int notFinished = 1;
    auto dContinue  = memAlloc<T>(sizeof(int));

    while (notFinished > 0) {
        notFinished = 0;
        edgeTraceOp.setScalar(dContinue.get(), notFinished);
        edgeTraceOp(EnqueueArgs(getQueue(), global, threads), *output.data,
                    output.info, blk_x, blk_y, *dContinue);
        CL_DEBUG_FINISH(getQueue());
        notFinished = edgeTraceOp.getScalar(dContinue.get());
    }
    suppressLeftOver<T>(output);
}
}  // namespace kernel
}  // namespace opencl
