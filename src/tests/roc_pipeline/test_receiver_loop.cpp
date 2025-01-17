/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/scheduler.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_allocator.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/receiver_loop.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

namespace {

enum { MaxBufSize = 1000 };

const core::nanoseconds_t MaxBufDuration = core::Millisecond * 10;

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);
core::BufferFactory<uint8_t> byte_buffer_factory(allocator, MaxBufSize, true);
packet::PacketFactory packet_factory(allocator, true);

rtp::FormatMap format_map;

class TaskIssuer : public IPipelineTaskCompleter {
public:
    TaskIssuer(PipelineLoop& pipeline)
        : pipeline_(pipeline)
        , slot_(NULL)
        , task_create_slot_(NULL)
        , task_create_endpoint_(NULL)
        , task_delete_endpoint_(NULL)
        , done_(false) {
    }

    ~TaskIssuer() {
        delete task_create_slot_;
        delete task_create_endpoint_;
        delete task_delete_endpoint_;
    }

    void start() {
        task_create_slot_ = new ReceiverLoop::Tasks::CreateSlot();
        pipeline_.schedule(*task_create_slot_, *this);
    }

    void wait_done() const {
        while (!done_) {
            core::sleep_for(core::ClockMonotonic, core::Microsecond * 10);
        }
    }

    virtual void pipeline_task_completed(PipelineTask& task) {
        roc_panic_if_not(task.success());

        if (&task == task_create_slot_) {
            slot_ = task_create_slot_->get_handle();
            roc_panic_if_not(slot_);
            task_create_endpoint_ = new ReceiverLoop::Tasks::CreateEndpoint(
                slot_, address::Iface_AudioSource, address::Proto_RTP);
            pipeline_.schedule(*task_create_endpoint_, *this);
            return;
        }

        if (&task == task_create_endpoint_) {
            task_delete_endpoint_ = new ReceiverLoop::Tasks::DeleteEndpoint(
                slot_, address::Iface_AudioSource);
            pipeline_.schedule(*task_delete_endpoint_, *this);
            return;
        }

        if (&task == task_delete_endpoint_) {
            done_ = true;
            return;
        }

        roc_panic("unexpected task");
    }

private:
    PipelineLoop& pipeline_;

    ReceiverLoop::SlotHandle slot_;

    ReceiverLoop::Tasks::CreateSlot* task_create_slot_;
    ReceiverLoop::Tasks::CreateEndpoint* task_create_endpoint_;
    ReceiverLoop::Tasks::DeleteEndpoint* task_delete_endpoint_;

    core::Atomic<int> done_;
};

} // namespace

TEST_GROUP(receiver_loop) {
    test::Scheduler scheduler;

    ReceiverConfig config;

    void setup() {
        config.common.internal_frame_length = MaxBufDuration;

        config.common.resampling = false;
        config.common.timing = false;
    }
};

TEST(receiver_loop, endpoints_sync) {
    ReceiverLoop receiver(scheduler, config, format_map, packet_factory,
                          byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.valid());

    ReceiverLoop::SlotHandle slot = NULL;

    {
        ReceiverLoop::Tasks::CreateSlot task;
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());

        slot = task.get_handle();
    }

    {
        ReceiverLoop::Tasks::CreateEndpoint task(slot, address::Iface_AudioSource,
                                                 address::Proto_RTP);
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_writer());
    }

    {
        ReceiverLoop::Tasks::DeleteEndpoint task(slot, address::Iface_AudioSource);
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
    }
}

TEST(receiver_loop, endpoints_async) {
    ReceiverLoop receiver(scheduler, config, format_map, packet_factory,
                          byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.valid());

    TaskIssuer ti(receiver);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

} // namespace pipeline
} // namespace roc
