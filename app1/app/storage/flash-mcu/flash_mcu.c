#include "flash_mcu.h"
#include "drv_mcu_flash.h"
#include <string.h>

#define ANGLE_MAGIC    0x5AA55AA5UL
#define ANGLE_BLK_SIZE sizeof(priv_data_t)

/* ==================== 私有方法声明 ==================== */
static int32_t storage_load(storage_t *self);
static int32_t storage_save(storage_t *self);

/* ==================== 公开方法声明 ==================== */
static int32_t storage_init(storage_t *self);

static int32_t storage_write_model(storage_t *self, Work_Config_t work_config);
static Work_Config_t storage_read_model(storage_t *self);

static int32_t storage_write_vn(storage_t *self, uint16_t vn);
static uint16_t storage_read_vn(storage_t *self);

static int32_t storage_write_calib(storage_t *self, axis_num_t index, uint32_t flag);
static uint32_t storage_read_calib(storage_t *self, axis_num_t index);

static int32_t storage_write_pos_separate(storage_t *self, axis_num_t index, float pos);
static float storage_read_pos_separate(storage_t *self, axis_num_t index);

static float storage_read_pos_combine(storage_t *self, axis_num_t index);
static priv_data_t storage_read_all(storage_t *self);

/* ==================== 函数指针绑定 ==================== */
static storage_ops_t storage_ops =
{
    .init               = storage_init,
    .write_model        = storage_write_model,
    .read_model         = storage_read_model,

    .write_vn           = storage_write_vn,
    .read_vn            = storage_read_vn,

    .write_calib        = storage_write_calib,
    .read_calib         = storage_read_calib,

    .write_pos_separate = storage_write_pos_separate,
    .read_pos_separate  = storage_read_pos_separate,
    .read_pos_combine   = storage_read_pos_combine,

    .read_all           = storage_read_all,
};

static storage_priv_ops_t storage_priv_ops =
{
    .load = storage_load,
    .save = storage_save,
};

/* 全局单例 */
storage_t storage_dev =
{
    .data = {0},
    .ops = &storage_ops,
    .priv_ops = &storage_priv_ops,
};

/* ==================== 公有接口实现 ==================== */
static int32_t storage_init(storage_t *self)
{
    flash_store.ops->init(&flash_store, ANGLE_BLK_SIZE);
    return self->priv_ops->load(self);
}

/* 写入完整机型配置，对接work_config模块 */
static int32_t storage_write_model(storage_t *self, Work_Config_t work_config)
{
    self->data.work_config = work_config;
    return self->priv_ops->save(self);
}

static Work_Config_t storage_read_model(storage_t *self)
{
    return self->data.work_config;
}

/* 软件版本读写 */
static int32_t storage_write_vn(storage_t *self, uint16_t vn)
{
    self->data.soft_vn = vn;
    return self->priv_ops->save(self);
}

static uint16_t storage_read_vn(storage_t *self)
{
    return self->data.soft_vn;
}

/* 标定标志读写 */
static int32_t storage_write_calib(storage_t *self, axis_num_t index, uint32_t flag)
{
    if(index >= AXIS_NUM_MAX)
    {
        return -1;
    }
    self->data.axis_data[index].calib_flag = flag;
    return self->priv_ops->save(self);
}

static uint32_t storage_read_calib(storage_t *self, axis_num_t index)
{
    if(index >= AXIS_NUM_MAX)
    {
        return 0U;
    }
    return self->data.axis_data[index].calib_flag;
}

/* 写入分离点：float类型 + 浮点量程校验 */
static int32_t storage_write_pos_separate(storage_t *self, axis_num_t index, float pos)
{
    if(index >= AXIS_NUM_MAX)
    {
        return -1;
    }
    // 浮点范围校验，引用work_config全局宏
    if(pos < (float)AXIS_POS_MIN || pos > (float)AXIS_POS_MAX)
    {
        return -2;
    }
    self->data.axis_data[index].position_separae = pos;

    Work_Config_t cfg = self->data.work_config;
    Logic_t logic = cfg.bits.axis[index].val;
    float sep_pos = self->data.axis_data[index].position_separae;
    float combine_pos = sep_pos;

    switch(logic)
    {
        case LOGIC_A:
            // LOGIC_A：结合=伸出 → 结合点 = 分离点 +20mm
            combine_pos = sep_pos + (float)AXIS_COMBINE_OFFSET_MM;
            break;
        case LOGIC_B:
            // LOGIC_B：结合=缩回 → 结合点 = 分离点 -20mm
            combine_pos = sep_pos - (float)AXIS_COMBINE_OFFSET_MM;
            break;
        case LOGIC_NONE:
        default:
            combine_pos = sep_pos;
            break;
    }

    // 浮点范围校验，引用work_config全局宏
    if(combine_pos < (float)AXIS_POS_MIN || combine_pos > (float)AXIS_POS_MAX)
    {
        return -2;
    }

    self->data.axis_data[index].position_combine = combine_pos;
    return self->priv_ops->save(self);

}

static float storage_read_pos_separate(storage_t *self, axis_num_t index)
{
    if(index >= AXIS_NUM_MAX)
    {
        return 0.0f;
    }
    return self->data.axis_data[index].position_separae;
}

/* 核心：根据当前机型逻辑自动计算结合点，float输出 */
static float storage_read_pos_combine(storage_t *self, axis_num_t index)
{
    if(index >= AXIS_NUM_MAX)
    {
        return 0.0f;
    }
    return self->data.axis_data[index].position_combine;
}

static priv_data_t storage_read_all(storage_t *self)
{
    return self->data;
}

/* ==================== 底层load/save私有函数 ==================== */
static int32_t storage_load(storage_t *self)
{
    flash_store.ops->read(&flash_store, flash_store.ops->get_last_addr(&flash_store),
                          (uint8_t*)&self->data, ANGLE_BLK_SIZE);
    if (self->data.magic != ANGLE_MAGIC)
    {
        // Magic失效，完整初始化全部默认值
        memset(&self->data, 0, sizeof(priv_data_t));
        self->data.magic  = ANGLE_MAGIC;
        self->data.soft_vn = 1000U;
        storage_save(self);
    }
    return 0;
}

static int32_t storage_save(storage_t *self)
{
    uint8_t last_data[ANGLE_BLK_SIZE];
    flash_store.ops->read(&flash_store, flash_store.ops->get_last_addr(&flash_store),
                          last_data, ANGLE_BLK_SIZE);
    // 数据无变化直接跳过Flash写入，减少擦写
    if (memcmp(&self->data, last_data, ANGLE_BLK_SIZE) == 0)
    {
        return 0;
    }
    self->data.magic = ANGLE_MAGIC;
    uint32_t addr = flash_store.ops->get_next_addr(&flash_store);
    flash_store.ops->write(&flash_store, addr, (uint8_t*)&self->data, ANGLE_BLK_SIZE);
    flash_store.priv.last_valid_addr = addr;
    flash_store.priv.next_write_addr = addr + ANGLE_BLK_SIZE;
    return 0;
}

/* ==================== BSP上层封装接口 ==================== */
void bsp_storeage_init(void)
{
    storage_dev.ops->init(&storage_dev);
}

int32_t bsp_write_model(Work_Config_t cfg)
{
    return storage_dev.ops->write_model(&storage_dev, cfg);
}

Work_Config_t bsp_read_model(void)
{
    return storage_dev.ops->read_model(&storage_dev);
}

int32_t bsp_write_vn(uint16_t vn)
{
    return storage_dev.ops->write_vn(&storage_dev, vn);
}

uint16_t bsp_read_vn(void)
{
    return storage_dev.ops->read_vn(&storage_dev);
}

int32_t bsp_write_cali(axis_num_t index ,uint32_t flag)
{
    return storage_dev.ops->write_calib(&storage_dev, index, flag);
}

uint32_t bsp_read_cali(axis_num_t index)
{
    return storage_dev.ops->read_calib(&storage_dev, index);
}

// 分离点 float 读写接口
int32_t bsp_write_pos_separate(axis_num_t index, float pos)
{
    return storage_dev.ops->write_pos_separate(&storage_dev, index, pos);
}

float bsp_read_pos_separate(axis_num_t index)
{
    return storage_dev.ops->read_pos_separate(&storage_dev, index);
}

// 结合点 float 读取接口
float bsp_read_pos_combine(axis_num_t index)
{
    return storage_dev.ops->read_pos_combine(&storage_dev, index);
}

priv_data_t bsp_read_all(void)
{
    return storage_dev.ops->read_all(&storage_dev);
}

#if 0
void bsp_storage_test(void)
{
    bsp_storeage_init();
    Work_Config_t cfg = bsp_read_model();
    float sep = bsp_read_pos_separate(AXIS_NUM_0);
    float com = bsp_read_pos_combine(AXIS_NUM_0);
    int32_t ret = bsp_write_pos_separate(AXIS_NUM_0, 120.5f);
}
#endif
