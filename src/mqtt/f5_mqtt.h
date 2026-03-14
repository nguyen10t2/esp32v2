#ifndef F5_MQTT_H
#define F5_MQTT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
    #define DEFAULT_KALMAN_ERROR 10.0f
    #define DEFAULT_KALMAN_NOISE 0.1f
    #define DEFAULT_DEBOUNCE_DELAY 500 // milliseconds
    #define DEFAULT_ROR_THRESHOLD 8.0f // Ngưỡng mặc định: Tăng 8 độ/phút là báo cháy

    /**
     * @brief Hàm khởi tạo MQTT, thiết lập kết nối và cấu hình ban đầu cho MQTT client.
     */
    void f5_mqtt_init(void);

    /**
     * @brief Hàm cập nhật dữ liệu cảm biến và gửi payload lên MQTT server.
     * Hàm này sẽ được gọi định kỳ để gửi dữ liệu mới nhất từ node cảm biến.
     * @param raw_temp Nhiệt độ thô từ cảm biến (chưa được lọc)
     * @param raw_smoke Mức độ khói thô từ cảm biến (chưa được lọc)
     * @param raw_flame Trạng thái lửa thô từ cảm biến (chưa được lọc)
     * @param battery Mức pin còn lại của node cảm biến tính bằng phần trăm
     * @param current_millis Thời điểm hiện tại tính bằng milliseconds since epoch,
     * được sử dụng để đánh dấu thời gian ghi nhận dữ liệu
     * @return Trả về true nếu dữ liệu đã được gửi thành công lên MQTT server,
     * ngược lại trả về false nếu có lỗi xảy ra trong quá trình gửi dữ liệu.
     */
    bool f5_mqtt_update(float raw_temp, float raw_smoke, bool raw_flame, uint8_t battery, uint32_t current_millis);

#ifdef __cplusplus
}
#endif

#endif