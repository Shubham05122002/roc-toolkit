/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/converter_sink.h
//! @brief Converter sink pipeline.

#ifndef ROC_PIPELINE_CONVERTER_SINK_H_
#define ROC_PIPELINE_CONVERTER_SINK_H_

#include "roc_audio/channel_mapper_writer.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/null_writer.h"
#include "roc_audio/poison_writer.h"
#include "roc_audio/profiling_writer.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/optional.h"
#include "roc_core/scoped_ptr.h"
#include "roc_pipeline/config.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Converter sink pipeline.
//! @remarks
//!  - input: frames
//!  - output: frames
class ConverterSink : public sndio::ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    ConverterSink(const ConverterConfig& config,
                  audio::IFrameWriter* output_writer,
                  core::BufferFactory<audio::sample_t>& buffer_factory,
                  core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Get device type.
    virtual sndio::DeviceType type() const;

    //! Get device state.
    virtual sndio::DeviceState state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Get sample specification of the sink.
    virtual audio::SampleSpec sample_spec() const;

    //! Get latency of the sink.
    virtual core::nanoseconds_t latency() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    audio::NullWriter null_writer_;

    core::Optional<audio::ChannelMapperWriter> channel_mapper_writer_;

    core::Optional<audio::PoisonWriter> resampler_poisoner_;
    core::Optional<audio::ResamplerWriter> resampler_writer_;
    core::ScopedPtr<audio::IResampler> resampler_;

    core::Optional<audio::PoisonWriter> pipeline_poisoner_;

    core::Optional<audio::ProfilingWriter> profiler_;

    audio::IFrameWriter* audio_writer_;

    const ConverterConfig config_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONVERTER_SINK_H_
