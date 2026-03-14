#include "mqtt_filter.h"
#include <math.h> 

void kalman_init(kalman_state_t *kf, float mea_e, float est_e, float q) {
    kf->err_measure = mea_e;
    kf->err_estimate = est_e;
    kf->q = q;
    kf->last_estimate = 0.0f;
    kf->current_estimate = 0.0f;
}

float kalman_apply(kalman_state_t *kf, float raw_smoke) {
    float kalman_gain = kf->err_estimate / (kf->err_estimate + kf->err_measure);
    
    kf->current_estimate = kf->last_estimate + kalman_gain * (raw_smoke - kf->last_estimate);
    kf->err_estimate = (1.0f - kalman_gain) * kf->err_estimate + fabs(kf->last_estimate - kf->current_estimate) * kf->q;
    kf->last_estimate = kf->current_estimate;
    
    return kf->current_estimate;
}

void debounce_init(debounce_state_t *db, uint32_t threshold) {
    db->lastFlameTime = 0;
    db->flameThreshold = threshold;
    db->flameState = false;
}

bool debounce_check(debounce_state_t *db, uint8_t raw_flame, uint32_t current_millis) {
    //TH có lửa
    if (raw_flame) { 
        if (current_millis - db->lastFlameTime > db->flameThreshold) {
            db->flameState = true; // Xác nhận có lửa nếu đã vượt qua ngưỡng thời gian
        }
    } else {
        db->lastFlameTime = current_millis; // Reset đồng hồ nếu không có lửa
        db->flameState = false; // Không có lửa
    }
    return db->flameState;
}

void ror_init(ror_state_t *ror, float threshold) {
    ror->last_temp = 0.0f;
    ror->last_time = 0; // Đặt bằng 0 để đánh dấu đây là lần chạy đầu tiên
    ror->threshold_per_min = threshold;
}

bool ror_check(ror_state_t *ror, float current_temp, uint32_t current_millis) {
    //TRƯỜNG HỢP LẦN ĐẦU TIÊN BẬT MÁY
    if (ror->last_time == 0) {
        ror->last_temp = current_temp;   // Lưu nhiệt độ hiện tại làm cột mốc
        ror->last_time = current_millis; // Lưu thời gian hiện tại làm cột mốc
        return false;                    // Chưa có khoảng thời gian trôi qua nên chưa thể tính tốc độ
    }

    //TÍNH TOÁN KHOẢNG CÁCH (DELTA)
    float delta_temp = current_temp - ror->last_temp;        // Nhiệt độ đã tăng bao nhiêu?
    uint32_t delta_time_ms = current_millis - ror->last_time; // Đã bao nhiêu mili giây trôi qua?

    //CẬP NHẬT TỐC ĐỘ (Chỉ tính sau mỗi 5 giây để tránh cảm biến bị nhiễu cục bộ)
    if (delta_time_ms >= 5000) {
        
        //Công thức tính vận tốc tăng nhiệt (Độ C / Phút)
        float rate_per_min = (delta_temp / (float)delta_time_ms) * 60000.0f;
        
        //Dịch chuyển cột mốc thời gian và nhiệt độ cho chu kỳ tính toán tiếp theo
        ror->last_temp = current_temp;
        ror->last_time = current_millis;

        //KIỂM TRA NGƯỠNG
        if (rate_per_min >= ror->threshold_per_min) {
            return true; // Tốc độ tăng vượt ngưỡng -> Phát hiện cháy!
        }
    }

    return false; 
}