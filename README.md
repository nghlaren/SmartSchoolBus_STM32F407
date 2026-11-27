# Smart School Bus

Hệ thống điểm danh và hỗ trợ chống bỏ quên học sinh trên xe đưa đón, sử dụng **STM32F407VET6**, **FreeRTOS**, RC522, OLED SSD1306 và microSD.

## Chức năng

- Đăng ký, thay thẻ RFID và xóa toàn bộ liên kết thẻ hiện tại.
- Điểm danh lên/xuống xe, chống đếm trùng và xác nhận xuống xe thủ công.
- Cảnh báo còn học sinh; yêu cầu kiểm tra cuối xe bằng nút CHECKED.
- Giao diện tiếng Anh; ba nút SELECT, OK và CHECKED.
- Lưu nhật ký có RTC và CRC32 trên SD; khôi phục trạng thái từ sự kiện đã lưu.
- Phần mềm phân lớp, bốn task ứng dụng và hàng đợi FreeRTOS.

## Mở và build

1. Clone hoặc tải repository vào đường dẫn không dấu, ví dụ `D:/Projects/SmartSchoolBus`.
2. STM32CubeIDE → File → Import → Existing Projects into Workspace.
3. Chọn thư mục con `SmartSchoolBus_STM32F407` có cả `.project` và `.cproject`.
4. Chọn cấu hình Debug → Clean → Build Project.
5. Debug dùng `Debug/SmartSchoolBus_STM32F407.elf`, ST-LINK, SWD 100 kHz và Connect under reset.

Phần cứng được cấu hình bằng mã trong `board.c`; không cần Generate Code từ CubeMX.
Nếu cấu hình launch có ánh xạ đường dẫn nguồn từ máy cũ, bỏ ánh xạ đó và build lại tại đường dẫn mới.

## Cấu trúc

| Đường dẫn | Nội dung |
|---|---|
| `SmartSchoolBus_STM32F407/Core` | Mã ứng dụng, nghiệp vụ và driver |
| `SmartSchoolBus_STM32F407/Drivers` | HAL và CMSIS |
| `SmartSchoolBus_STM32F407/Middlewares` | FreeRTOS và FatFs |
| `SmartSchoolBus_STM32F407/Tests` | Kiểm thử phần mềm |
| `SD_CARD/STUDENTS.CSV` | Danh sách học sinh mẫu |
| `HUONG_DAN_CUBEIDE.md` | Cách nối dây, vận hành, nạp và debug |
| `SmartSchoolBus_STM32F407.hex` | Firmware có sẵn để nạp bằng CubeProgrammer |
| `docs/` | Tài liệu tổng hợp Word phục vụ viết báo cáo |

Sao chép `STUDENTS.CSV` vào thư mục gốc thẻ SD. Không dùng nhật ký của chuyến thật làm dữ liệu mẫu.


