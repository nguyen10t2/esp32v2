# Module Actuator (Thiết bị chấp hành)

Thư mục này chứa mã nguồn quản lý các thiết bị đầu ra (Output) của hệ thống, bao gồm còi báo động và màn hình hiển thị hướng sơ tán.

## 1. Cấu trúc thư mục
- `buzzer.c / .h`: Quản lý còi hú báo động.
- `led_matrix.c / .h`: Quản lý hiển thị hướng sơ tán trên Ma trận LED.

## 2. Chi tiết các thành phần và thay đổi

### 📢 Module Buzzer (Còi hú)
- **Vị trí chân:** Mặc định cấu hình tại `GPIO_NUM_5`.
- **Chức năng:** - `buzzer_init()`: Cấu hình GPIO, đảm bảo còi tắt khi khởi động hệ thống.
    - `buzzer_set_state(bool state)`: Nhận lệnh từ module MQTT để bật/tắt còi theo thời gian thực.
- **Cập nhật:** Đã xử lý logic chống nhiễu lệnh và đảm bảo không gây delay cho hệ thống chính.

### 🧭 Module LED Matrix (Chỉ hướng)
- **Chức năng:** Hiển thị mũi tên chỉ đường dựa trên dữ liệu điều hướng từ Server.
- **Các trạng thái hỗ trợ:**
    - `"N"`: Đi lên (North)
    - `"S"`: Đi xuống (South)
    - `"E"`: Sang phải (East)
    - `"W"`: Sang trái (West)
    - `"OFF"`: Tắt màn hình.
- **Cập nhật:** Xây dựng khung giao tiếp hướng sự kiện (Event-driven), sẵn sàng tích hợp các chuẩn giao tiếp SPI/I2C.

## 3. Hướng dẫn tích hợp cho Team
Để sử dụng các module này trong `main.c`, chỉ cần:
1. Include file header tương ứng.
2. Gọi hàm `init` trong phần setup.
3. Module MQTT sẽ tự động gọi các hàm `set_state` hoặc `draw_direction` khi nhận được gói tin JSON phù hợp từ server.

*Lưu ý: Nếu thay đổi chân GPIO, vui lòng cập nhật trong file `.c` tương ứng.*