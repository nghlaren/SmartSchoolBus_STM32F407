# Smart School Bus

Tôi xây dựng hệ thống điểm danh và hỗ trợ kiểm tra học sinh trên xe đưa đón bằng **STM32F407VET6** và **FreeRTOS**. Thẻ RFID ghi nhận lượt lên, xuống xe; trước khi kết thúc chuyến, người phụ trách phải xác nhận đã kiểm tra cuối xe bằng nút **CHECKED**. Trạng thái chuyến đi được lưu trên microSD để khôi phục sau khi khởi động lại.

## Chức năng chính

- Đăng ký, thay thẻ RFID và quản lý danh sách học sinh từ `STUDENTS.CSV`.
- Điểm danh lên/xuống xe, chống quét trùng và xác nhận xuống xe thủ công khi mất thẻ.
- Cảnh báo khi còn học sinh trên xe; yêu cầu kiểm tra cuối xe trước khi hoàn tất chuyến.
- Hiển thị trạng thái trên OLED SSD1306, phát cảnh báo bằng còi và thao tác bằng ba nút SELECT, OK, CHECKED.
- Ghi sự kiện kèm thời gian RTC và CRC32 vào thẻ SD; đọc lại nhật ký để khôi phục trạng thái.

## Kiến trúc

| Thành phần | Vai trò |
| --- | --- |
| RC522 | Đọc UID thẻ RFID qua SPI |
| STM32F407VET6 | Điều phối giao diện, nghiệp vụ điểm danh và ngoại vi |
| FreeRTOS | Bốn task ứng dụng: Trip, Keys, RFID, Sound |
| OLED SSD1306 và còi | Hiển thị trạng thái, phát cảnh báo |
| microSD và FatFs | Lưu danh sách học sinh, nhật ký sự kiện |
| RTC | Gắn thời gian cho bản ghi |

Luồng nghiệp vụ đi từ **READY → PICKUP → DROP OFF → CHECK BUS → ALL DONE**. Nếu còn học sinh hoặc chưa xác nhận kiểm tra cuối xe, hệ thống giữ trạng thái cảnh báo. Chi tiết thao tác, kết nối chân và định dạng file SD nằm trong [hướng dẫn CubeIDE](HUONG_DAN_CUBEIDE.md).

## Cài đặt và nạp firmware

1. Clone repo vào đường dẫn không dấu, ví dụ `D:/Projects/SmartSchoolBus`.
2. Trong STM32CubeIDE, chọn **File → Import → Existing Projects into Workspace** và trỏ tới thư mục `SmartSchoolBus_STM32F407` có `.project` và `.cproject`.
3. Chọn cấu hình **Debug**, rồi **Clean** và **Build Project**.
4. Nạp file [`SmartSchoolBus_STM32F407.hex`](SmartSchoolBus_STM32F407.hex) bằng STM32CubeProgrammer hoặc debug file ELF vừa build qua ST-LINK.
5. Chép [`SD_CARD/STUDENTS.CSV`](SD_CARD/STUDENTS.CSV) vào thư mục gốc của thẻ SD trước khi vận hành.

Cấu hình phần cứng nằm trong `board.c`; project không cần tạo lại mã bằng CubeMX.

## Kiểm thử

Bộ kiểm thử phần mềm trong [`Tests/`](SmartSchoolBus_STM32F407/Tests/) tập trung vào chuyển trạng thái chuyến đi, quản lý thẻ, danh sách CSV và nhật ký SD. Có thể chạy trên PC với Python 3 và GCC:

```bash
python SmartSchoolBus_STM32F407/Tests/run_tests.py
```

## Cấu trúc repo

| Đường dẫn | Nội dung |
| --- | --- |
| [`SmartSchoolBus_STM32F407/Core/`](SmartSchoolBus_STM32F407/Core/) | Nghiệp vụ, giao diện và driver phần cứng |
| [`SmartSchoolBus_STM32F407/Drivers/`](SmartSchoolBus_STM32F407/Drivers/) | STM32 HAL và CMSIS |
| [`SmartSchoolBus_STM32F407/Middlewares/`](SmartSchoolBus_STM32F407/Middlewares/) | FreeRTOS và FatFs |
| [`SmartSchoolBus_STM32F407/Tests/`](SmartSchoolBus_STM32F407/Tests/) | Kiểm thử phần mềm |
| [`SD_CARD/`](SD_CARD/) | Danh sách học sinh mẫu |
| [`docs/images/`](docs/images/) | Ảnh thực tế của mô hình |
