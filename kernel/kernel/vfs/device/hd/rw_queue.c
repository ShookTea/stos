#include "../../rw_queue/rw_queue.h"
#include "./hd.h"
#include <stdlib.h>

rw_queue_t hd_rw_queue = RW_QUEUE_INIT;

void hd_rw_ready(void* ptr)
{
    hd_wakeup_data_t* data = ptr;
    size_t id = data->wait_idx;
    if (!rwq_set_ready(&hd_rw_queue, id)) {
        abort();
    }
    wait_wake_up(data->hd_metadata->wait_obj);
}

bool hd_rw_wait_for_ready(void* ptr)
{
    hd_wakeup_data_t* data = ptr;
    size_t id = data->wait_idx;
    return rwq_is_ready(&hd_rw_queue, id);
}
