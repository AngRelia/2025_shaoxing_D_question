#ifndef ICODE_ENCODER_H_
#define ICODE_ENCODER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== 编码器参数（按你的电机/减速箱实际参数修改） ===== */
#define ENCODER_PPR              (13.0f)     /* 电机轴每圈脉冲数（单沿） */
#define ENCODER_GEAR_RATIO       (20.0f)     /* 减速比 */
#define ENCODER_EDGE_FACTOR      (2.0f)      /* 仅A相上下沿计数 -> 2X，B相预留 */
#define ENCODER_WHEEL_DIAMETER_M (0.065f)    /* 轮径（m） */
#define ENCODER_SAMPLE_PERIOD_S  (0.020f)    /* 统计窗口（s），当前是20ms */

/* 方向符号校正（若正反颠倒，将对应值改为 -1.0f） */
#define ENCODER_LEFT_DIR_SIGN    (1.0f)
#define ENCODER_RIGHT_DIR_SIGN   (-1.0f)

void encoder_init(void);

float encoder_get_left_speed_mps(void);
float encoder_get_right_speed_mps(void);
float encoder_get_left_rpm(void);
float encoder_get_right_rpm(void);

uint8_t encoder_is_updated(void);
void encoder_clear_update_flag(void);

#ifdef __cplusplus
}
#endif

#endif /* ICODE_ENCODER_H_ */
