// Copyright (C) 2012  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <cc/data.h>

#include <auth/datasrc_clients_mgr.h>
#include "test_datasrc_clients_mgr.h"

#include <gtest/gtest.h>

#include <boost/function.hpp>

using namespace isc::auth;
using namespace isc::auth::datasrc_clientmgr_internal;

namespace {
void
shutdownCheck() {
    // Check for common points on shutdown.  The manager should have acquired
    // the lock, put a SHUTDOWN command to the queue, and should have signaled
    // the builder.
    EXPECT_EQ(1, FakeDataSrcClientsBuilder::queue_mutex->lock_count);
    EXPECT_EQ(1, FakeDataSrcClientsBuilder::cond->signal_count);
    EXPECT_EQ(1, FakeDataSrcClientsBuilder::command_queue->size());
    const Command& cmd = FakeDataSrcClientsBuilder::command_queue->front();
    EXPECT_EQ(SHUTDOWN, cmd.first);
    EXPECT_FALSE(cmd.second);   // no argument

    // Finally, the manager should wait for the thread to terminate.
    EXPECT_TRUE(FakeDataSrcClientsBuilder::thread_waited);
}

TEST(DataSrcClientsMgrTest, start) {
    // When we create a manager, builder's run() method should be called.
    FakeDataSrcClientsBuilder::started = false;
    {
        TestDataSrcClientsMgr mgr;
        EXPECT_TRUE(FakeDataSrcClientsBuilder::started);
        EXPECT_TRUE(FakeDataSrcClientsBuilder::command_queue->empty());

        // Check pre-destroy conditions
        EXPECT_EQ(0, FakeDataSrcClientsBuilder::cond->signal_count);
        EXPECT_FALSE(FakeDataSrcClientsBuilder::thread_waited);
    } // mgr and builder have been destroyed by this point.

    // We stopped the manager implicitly (without shutdown()).  The manager
    // will internally notify it
    shutdownCheck();
}

TEST(DataSrcClientsMgrTest, shutdownWithUncaughtException) {
    // Emulating the case when the builder exists on exception.  shutdown()
    // will encounter UncaughtException exception and catch it.
    EXPECT_NO_THROW({
            TestDataSrcClientsMgr mgr;
            FakeDataSrcClientsBuilder::thread_throw_on_wait =
                FakeDataSrcClientsBuilder::THROW_UNCAUGHT_EX;
        });
}

TEST(DataSrcClientsMgrTest, shutdownWithException) {
    EXPECT_NO_THROW({
            TestDataSrcClientsMgr mgr;
            FakeDataSrcClientsBuilder::thread_throw_on_wait =
                FakeDataSrcClientsBuilder::THROW_OTHER;
        });
}

TEST(DataSrcClientsMgrTest, realThread) {
    // Using the non-test definition with a real thread.  Just checking
    // no disruption happens.
    DataSrcClientsMgr mgr;
}

} // unnamed namespace