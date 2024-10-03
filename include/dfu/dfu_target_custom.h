
#ifndef DFU_TARGET_CUSTOM_H__
#define DFU_TARGET_CUSTOM_H__

#include <dfu/dfu_target.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool dfu_target_custom_identify(const void *const buf);
int dfu_target_custom_init(size_t file_size, dfu_target_callback_t cb);
int dfu_target_custom_offset_get(size_t *offset);
int dfu_target_custom_write(const void *const buf, size_t len);
int dfu_target_custom_done(bool successful);
int dfu_target_custom_schedule_update(int img_num);
int dfu_target_custom_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_SUIT_H__ */