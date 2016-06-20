/*
 * Copyright (c) 2016 UAVCAN Team
 *
 * Distributed under the MIT License, available in the file LICENSE.
 *
 */

#include "nuttxcan.h"
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <nuttx/can.h>

/**
 * Initializes the NuttXCAN instance.
 */
int nuttxcanInit(NuttXCANInstance* out_ins, const char* can_iface_name)
{
    const int fd = open(can_iface_name, O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        return -1;
    }

    out_ins->fd = fd;
    return 1;
}

/**
 * Deinitializes the NuttXCAN instance.
 */
int nuttxcanClose(NuttXCANInstance* ins)
{
    const int close_result = close(ins->fd);
    if (close_result < 0)
    {
        return -1;
    }

    return 1;
}

/**
 * Transmits a CanardCANFrame to the CAN device.
 */
int nuttxcanTransmit(NuttXCANInstance* ins, const CanardCANFrame* frame, int timeout_msec)
{
    struct pollfd fds;
    memset(&fds, 0, sizeof(fds));
    fds.fd = ins->fd;
    fds.events |= POLLOUT;

    const int poll_result = poll(&fds, 1, timeout_msec);
    if (poll_result < 0)
    {
        return -1;
    }
    if (poll_result == 0)
    {
        return 0;
    }
    if ((fds.revents & POLLOUT) == 0)
    {
        return -1;
    }

    struct can_msg_s transmit_msg;
    memset(&transmit_msg, 0, sizeof(transmit_msg));
    transmit_msg.cm_hdr.ch_id = frame->id;
    transmit_msg.cm_hdr.ch_dlc = frame->data_len;
    transmit_msg.cm_hdr.ch_extid = 1;
    memcpy(transmit_msg.cm_data, frame->data, frame->data_len);

    const size_t msg_len = CAN_MSGLEN(transmit_msg.cm_hdr.ch_dlc);
    const ssize_t nbytes = write(ins->fd, &transmit_msg, msg_len);
    if (nbytes < 0 || (size_t)nbytes != msg_len)
    {
        return -1;
    }

    return 1;
}

/**
 * Receives a CanardCANFrame from the CAN device.
 */
int nuttxcanReceive(NuttXCANInstance* ins, CanardCANFrame* out_frame, int timeout_msec)
{
    struct pollfd fds;
    memset(&fds, 0, sizeof(fds));
    fds.fd = ins->fd;
    fds.events |= POLLIN;

    const int poll_result = poll(&fds, 1, timeout_msec);
    if (poll_result < 0)
    {
        return -1;
    }
    if (poll_result == 0)
    {
        return 0;
    }
    if ((fds.revents & POLLIN) == 0)
    {
        return -1;
    }

    struct can_msg_s receive_msg;
    const ssize_t nbytes = read(ins->fd, &receive_msg, sizeof(receive_msg));
    if (nbytes < 0 || (size_t)nbytes < CAN_MSGLEN(0) || (size_t)nbytes > sizeof(receive_msg))
    {
        return -1;
    }

    out_frame->id = receive_msg.cm_hdr.ch_id;
    out_frame->data_len = receive_msg.cm_hdr.ch_dlc;
    memcpy(out_frame->data, receive_msg.cm_data, receive_msg.cm_hdr.ch_dlc);

    return 1;
}

/**
 * Returns the file descriptor of the CAN device.
 */
int nuttxcanGetDeviceFileDescriptor(const NuttXCANInstance* ins)
{
    return ins->fd;
}
