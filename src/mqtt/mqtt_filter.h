#ifndef MQTT_FILTER_H
#define MQTT_FILTER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Cấu trúc lưu trạng thái của bộ lọc Kalman.
 */
typedef struct {
    float err_measure;      /**< Sai số đo lường */
    float err_estimate;     /**< Sai số ước lượng */
    float q;                /**< Nhiễu môi trường */
    float current_estimate; /**< Đầu ra cuối */
    float last_estimate;    /**< giá trị khói "sạch" ở lần đo trước */
    float kalman_gain;      /**< Hệ số Kalman */
} kalman_state_t;

/**
 * @brief Cấu trúc lưu trạng thái của bộ lọc chống dội.
 */
typedef struct {
    uint32_t lastFlameTime;  /**< Thời gian cuối cùng thấy lửa */
    uint32_t flameThreshold; /**< Ngưỡng cháy */
    uint8_t flameState;      /**< Đầu ra */
} debounce_state_t;

/**
 * @brief Cấu trúc lưu trạng thái của thuật toán Rate of Rise (RoR).
 * Thuật toán này giúp phát hiện đám cháy sớm dựa trên tốc độ tăng nhiệt độ
 * thay vì phải đợi nhiệt độ đạt đến một mức trần cố định.
 */
typedef struct {
    float last_temp;         /**< Nhiệt độ đo được ở lần gần nhất */
    uint32_t last_time;      /**< Mốc thời gian của lần đo gần nhất (mili giây) */
    float threshold_per_min; /**< Ngưỡng báo động: Số độ C tăng lên tối đa trong 1 phút */
} ror_state_t;

/**
 * @brief Khởi tạo bộ lọc Kalman và bộ lọc chống dội.
 *
 * @param kf Con trỏ đến cấu trúc trạng thái Kalman
 * @param mea_e Sai số đo lường ban đầu cho Kalman
 * @param est_e Sai số ước lượng ban đầu cho Kalman
 * @param q Nhiễu môi trường cho Kalman
 */
void kalman_init(kalman_state_t *kf, float mea_e, float est_e, float q);

/**
 * @brief Khởi tạo bộ lọc chống dội.
 *
 * @param db Con trỏ đến cấu trúc trạng thái chống dội
 * @param threshold Ngưỡng thời gian (ms) để xác định lửa ổn định
 */
void debounce_init(debounce_state_t *db, uint32_t threshold);

/**
 * @brief Khởi tạo thuật toán gia nhiệt đột ngột (RoR).
 * * @param ror Con trỏ đến cấu trúc trạng thái RoR
 * @param threshold Ngưỡng gia nhiệt (Độ C / phút)
 */
void ror_init(ror_state_t *ror, float threshold);

/**
 * @brief Áp dụng bộ lọc Kalman cho giá trị khói thô.
 *
 * @param kf Con trỏ đến cấu trúc trạng thái Kalman
 * @param raw_smoke Giá trị khói thô từ cảm biến
 * @return Giá trị khói đã được lọc
 */
float kalman_apply(kalman_state_t *kf, float raw_smoke);

/**
 * @brief Áp dụng bộ lọc chống dội cho giá trị lửa thô.
 *
 * @param db Con trỏ đến cấu trúc trạng thái chống dội
 * @param raw_flame Giá trị lửa thô từ cảm biến (0 hoặc 1)
 * @param current_millis Thời gian hiện tại (ms) để so sánh với thời gian cuối cùng thấy lửa
 * @return Trạng thái lửa đã được lọc (0 hoặc 1)
 */
bool debounce_check(debounce_state_t *db, uint8_t raw_flame, uint32_t current_millis);

/**
 * @brief Hàm kiểm tra xem nhiệt độ có đang tăng quá nhanh (gia nhiệt đột ngột) hay không.
 * * @param ror Con trỏ đến biến trạng thái RoR
 * @param current_temp Nhiệt độ thực tế vừa đọc được từ cảm biến
 * @param current_millis Mốc thời gian hiện tại (mili giây)
 * @return true nếu phát hiện cháy do nhiệt tăng sốc, false nếu nhiệt độ tăng bình thường
 */
bool ror_check(ror_state_t *ror, float current_temp, uint32_t current_millis);

#ifdef __cplusplus
}
#endif

#endif