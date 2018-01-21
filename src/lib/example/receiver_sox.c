/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/* Roc receiver example.
 *
 * This example receives an audio stream and plays it using SoX.
 * Receiver address and ports and other parameters are hardcoded.
 *
 * Building:
 *   gcc receiver_sox.c -lroc -lsox
 *
 * Running:
 *   ./a.out
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sox.h>

#include <roc/address.h>
#include <roc/context.h>
#include <roc/log.h>
#include <roc/receiver.h>

/* Receiver parameters. */
#define RECEIVER_IP "0.0.0.0"
#define RECEIVER_SOURCE_PORT 10001
#define RECEIVER_REPAIR_PORT 10002

/* Sender parameters */
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2

/* Player parameters. */
#define OUTPUT_DEVICE "default"
#define OUTPUT_TYPE "alsa"
#define BUFFER_SIZE 1000

#define oops(msg)                                                                        \
    do {                                                                                 \
        fprintf(stderr, "oops: %s\n", msg);                                              \
        exit(1);                                                                         \
    } while (0)

int main() {
    /* Initialize SoX. */
    if (sox_init() != SOX_SUCCESS) {
        oops("sox_init");
    }

    /* Enable debug logging. */
    roc_log_set_level(ROC_LOG_DEBUG);

    /* Initialize context config.
     * We use default values. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* Create context.
     * Context contains global state like memory pools and the network loop thread.
     * We need a context to create a receiver. */
    roc_context* context = roc_context_open(&context_config);
    if (!context) {
        oops("roc_context_open");
    }

    /* Start context thread. */
    if (roc_context_start(context) != 0) {
        oops("roc_context_start");
    }

    /* Initialize receiver config.
     * We use default values. */
    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    /* Create receiver. */
    roc_receiver* receiver = roc_receiver_open(context, &receiver_config);
    if (!receiver) {
        oops("roc_receiver_open");
    }

    /* Bind receiver to the source (audio) packets port.
     * The receiver will expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on this port. */
    roc_address recv_source_addr;
    if (roc_address_init(&recv_source_addr, ROC_AF_AUTO, RECEIVER_IP,
                         RECEIVER_SOURCE_PORT)
        != 0) {
        oops("roc_address_init");
    }
    if (roc_receiver_bind(receiver, ROC_PROTO_RTP_RSM8_SOURCE, &recv_source_addr) != 0) {
        oops("roc_receiver_bind");
    }

    /* Bind receiver to the repair (FEC) packets port.
     * The receiver will expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on this port. */
    roc_address recv_repair_addr;
    if (roc_address_init(&recv_repair_addr, ROC_AF_AUTO, RECEIVER_IP,
                         RECEIVER_REPAIR_PORT)
        != 0) {
        oops("roc_address_init");
    }
    if (roc_receiver_bind(receiver, ROC_PROTO_RSM8_REPAIR, &recv_repair_addr) != 0) {
        oops("roc_receiver_bind");
    }

    /* Initialize SoX parameters. */
    sox_signalinfo_t signal_info;
    memset(&signal_info, 0, sizeof(signal_info));
    signal_info.rate = SAMPLE_RATE;
    signal_info.channels = NUM_CHANNELS;
    signal_info.precision = SOX_SAMPLE_PRECISION;

    /* Open SoX output device. */
    sox_format_t* output =
        sox_open_write(OUTPUT_DEVICE, &signal_info, NULL, OUTPUT_TYPE, NULL, NULL);
    if (!output) {
        oops("sox_open_write");
    }

    /* Receive and play samples. */
    for (;;) {
        /* Read samples from receiver.
         * If not enough samples are received, receiver will pad buffer with zeros. */
        float recv_samples[BUFFER_SIZE];

        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = recv_samples;
        frame.num_samples = BUFFER_SIZE;

        if (roc_receiver_read(receiver, &frame) != 0) {
            break;
        }

        /* Convert samples to SoX format. */
        SOX_SAMPLE_LOCALS;

        size_t clips = 0;
        sox_sample_t out_samples[BUFFER_SIZE];

        ssize_t n;
        for (n = 0; n < BUFFER_SIZE; n++) {
            out_samples[n] = SOX_FLOAT_32BIT_TO_SAMPLE(recv_samples[n], clips);
        }

        /* Play samples.
         * SoX will block us until the output device is ready to accept new samples. */
        if (sox_write(output, out_samples, BUFFER_SIZE) != BUFFER_SIZE) {
            oops("sox_write");
        }
    }

    if (sox_close(output) != SOX_SUCCESS) {
        oops("sox_close");
    }

    /* Destroy receiver. */
    if (roc_receiver_close(receiver) != 0) {
        oops("roc_receiver_close");
    }

    /* Stop the context thread. */
    if (roc_context_stop(context) != 0) {
        oops("roc_context_stop");
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops("roc_context_close");
    }

    return 0;
}
