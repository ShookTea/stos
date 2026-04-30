#include "./hd.h"
#include "../../rw_queue/rw_queue.h"
#include "kernel/drivers/ata.h"

// Shared wait_obj for blocking MBR reads done during readdir/finddir.
wait_obj_t* hd_partition_wait_obj = NULL;

void hd_read_mbr_sync(uint8_t disk_id, uint16_t* mbr_buf)
{
    hd_metadata_t fake_meta = {
        .disk_id = disk_id,
        .is_partition = false,
        .wait_obj = hd_partition_wait_obj,
    };
    size_t wait_idx = rwq_allocate_pos(&hd_rw_queue);
    hd_wakeup_data_t wd = {
        .wait_idx = wait_idx,
        .hd_metadata = &fake_meta,
    };
    ata_read(disk_id, 0, 1, mbr_buf, hd_rw_ready, &wd);
    wait_on_condition(hd_partition_wait_obj, hd_rw_wait_for_ready, &wd);
    rwq_deallocate_pos(&hd_rw_queue, wait_idx);
}
