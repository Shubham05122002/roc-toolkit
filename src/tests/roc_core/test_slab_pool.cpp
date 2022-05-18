/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slab_pool.h"

namespace roc {
namespace core {

namespace {

struct Object : NonCopyable<> {
    static long n_objects;

    char padding[1000];

    Object() {
        n_objects++;
    }

    ~Object() {
        n_objects--;
    }
};

long Object::n_objects = 0;

} // namespace

TEST_GROUP(slab_pool) {
    HeapAllocator allocator;
};

TEST(slab_pool, allocate_deallocate) {
    SlabPool pool(allocator, sizeof(Object), true);

    void* memory = pool.allocate();
    CHECK(memory);

    Object* object = new (memory) Object;

    LONGS_EQUAL(1, Object::n_objects);

    pool.deallocate(object);

    LONGS_EQUAL(1, Object::n_objects);
}

TEST(slab_pool, allocate_deallocate_many) {
    {
        SlabPool pool(allocator, sizeof(Object), true);

        Object* objects[1 + 2 + 4] = {};

        LONGS_EQUAL(0, allocator.num_allocations());
        LONGS_EQUAL(0, Object::n_objects);

        size_t n_objs = 0;

        for (; n_objs < 1; n_objs++) {
            objects[n_objs] = new (pool) Object;
            CHECK(objects[n_objs]);
        }

        LONGS_EQUAL(1, allocator.num_allocations());
        LONGS_EQUAL(1, Object::n_objects);

        for (; n_objs < 1 + 2; n_objs++) {
            objects[n_objs] = new (pool) Object;
            CHECK(objects[n_objs]);
        }

        LONGS_EQUAL(2, allocator.num_allocations());
        LONGS_EQUAL(1 + 2, Object::n_objects);

        for (; n_objs < 1 + 2 + 4; n_objs++) {
            objects[n_objs] = new (pool) Object;
            CHECK(objects[n_objs]);
        }

        LONGS_EQUAL(3, allocator.num_allocations());
        LONGS_EQUAL(1 + 2 + 4, Object::n_objects);

        for (size_t n = 0; n < n_objs; n++) {
            objects[n]->~Object();
            pool.deallocate(objects[n]);
        }

        LONGS_EQUAL(3, allocator.num_allocations());
        LONGS_EQUAL(0, Object::n_objects);
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

} // namespace core
} // namespace roc