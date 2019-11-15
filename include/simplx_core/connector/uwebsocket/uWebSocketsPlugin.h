/* The loop is lazily created per-thread and run with uWS::run() */
#pragma once

#include "App.h"
extern "C" {
#include "internal/internal.h"
}

namespace uWSPlugin{
    
void integrate(us_loop_t* loop){
    ::us_loop_integrate(loop);
}

void singleLoopRun(us_loop_t* loop){

    /* if we have non-fallthrough polls we shouldn't fall through */
    if (loop->num_polls) {
        /* Emit pre callback */
        ::us_internal_loop_pre(loop);

        /* Fetch ready polls */
        loop->num_ready_polls = ::epoll_wait(loop->fd, loop->ready_polls, 1024, 0);

        /* Iterate ready polls, dispatching them by type */
        for (loop->current_ready_poll = 0; loop->current_ready_poll < loop->num_ready_polls; loop->current_ready_poll++) {
            struct us_poll_t *poll = (struct us_poll_t *) loop->ready_polls[loop->current_ready_poll].data.ptr;
            /* Any ready poll marked with nullptr will be ignored */
            if (poll) {

                int events = loop->ready_polls[loop->current_ready_poll].events;
                int error = loop->ready_polls[loop->current_ready_poll].events & (EPOLLERR | EPOLLHUP);
                /* Always filter all polls by what they actually poll for (callback polls always poll for readable) */
                events &= ::us_poll_events(poll);
                if (events || error) {
                    ::us_internal_dispatch_ready_poll(poll, error, events);
                }
            }
        }
        /* Emit post callback */
        ::us_internal_loop_post(loop);
    }
}

}