# Nhom6-K02
Register-level I2C driver, SSD1306 OLED and sensor integration for STM32F103C8T6
#sv3_oled
-Dành cho SSD1306 128×64, dùng charge pump nội.
-Địa chỉ mặc định 0x3C, chưa dịch trái.
-SV4 khởi tạo nguồn/reset và I2C trước, rồi gọi OLED_Init(&handle).
-handle phải tồn tại suốt thời gian dùng OLED; nên là biến global hoặc static.
-OLED_Init() xóa buffer, nên vẽ sau khi Init thành công.
-Các hàm vẽ chỉ sửa RAM; gọi OLED_Update() để gửi ảnh.
-OLED_OK = 0; SV4 phải kiểm tra kết quả Init/Update.
-SV4 ghép driver SV1 và xác nhận hiển thị thực tế; mày sửa lỗi thuộc module OLED nếu xuất hiện.