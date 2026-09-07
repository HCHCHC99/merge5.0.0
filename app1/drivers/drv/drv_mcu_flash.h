#ifndef DRV_MCU_FLASH_H_
#define DRV_MCU_FLASH_H_

#include <stdint.h>

typedef struct flash flash_t;

// 公开方法
typedef struct
{
    int32_t  (*init)(flash_t *self, uint32_t blk_size);
    void      (*write)(flash_t *self, uint32_t addr, const uint8_t *data, uint32_t len);
    void      (*read)(flash_t *self, uint32_t addr, uint8_t *data, uint32_t len);
    uint32_t  (*get_last_addr)(flash_t *self);
    uint32_t  (*get_next_addr)(flash_t *self);
} flash_ops_t;

// 私有方法
typedef struct
{
    void (*unlock)(flash_t *self);
    void (*lock)(flash_t *self);
    int32_t (*erase)(flash_t *self);
    uint32_t (*find_last_valid)(flash_t *self, uint32_t blk_size);
} flash_priv_ops_t;

// Flash 类
struct flash
{
    // 私有属性
    struct
    {
        uint32_t sector_addr;
        uint32_t sector_size;
        uint32_t magic;
        uint32_t last_valid_addr;
        uint32_t next_write_addr;
        int32_t irq_level;
    } priv;

    // 公开方法
    flash_ops_t *ops;

    // 私有方法
    flash_priv_ops_t *priv_ops;
};

extern flash_t flash_store;

#endif /* DRV_MCU_FLASH_H_ */
