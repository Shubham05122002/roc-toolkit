/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet_factory.h
//! @brief Packet factory.

#ifndef ROC_PACKET_PACKET_FACTORY_H_
#define ROC_PACKET_PACKET_FACTORY_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/slab_pool.h"

namespace roc {
namespace packet {

class Packet;

//! Packet factory.
class PacketFactory : public core::NonCopyable<> {
public:
    //! Constructor.
    PacketFactory(core::IAllocator& allocator, bool poison);

    //! Create new packet;
    core::SharedPtr<Packet> new_packet();

private:
    friend class core::FactoryAllocation<PacketFactory>;

    void destroy(Packet&);

    core::SlabPool pool_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_FACTORY_H_
