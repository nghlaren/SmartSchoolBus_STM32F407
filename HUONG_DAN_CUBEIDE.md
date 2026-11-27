# Smart School Bus — Attendance and Child Check System

Tên project và firmware: **SmartSchoolBus_STM32F407**. MCU STM32F407VET6.
Bản nộp gồm toàn bộ mã nguồn, HAL/CMSIS, FreeRTOS, FatFs, cấu hình CubeIDE,
HEX/ELF đã build và danh sách học sinh SD mẫu. Giao diện tiếng Anh, OLED 128×32,
SELECT / OK / CHECKED, module còi kích thấp.

## 1. Chuẩn bị và nạp

1. Giải nén ZIP vào đường dẫn đơn giản, ví dụ `D:\Projects\SmartSchoolBus`.
2. CubeIDE → File → Import → General → Existing Projects into Workspace.
3. Select root directory → chọn thư mục con `SmartSchoolBus_STM32F407`
   chứa `.project` và `.cproject`. Chọn project rồi Finish.
4. Chọn project → Project → Build Configurations → Set Active → Debug.
5. Project → Clean, rồi Build Project. Kết quả `Debug/SmartSchoolBus_STM32F407.elf`.
6. Run → Debug Configurations → STM32 C/C++ Application → SmartSchoolBus_STM32F407.
   Main: project cùng tên, C/C++ Application `Debug/SmartSchoolBus_STM32F407.elf`.
   Debugger: ST-LINK GDB server, SWD 100 kHz, Connect under reset.
   Không bật chọn ST-LINK theo serial nếu chưa xác định serial hợp lệ.
7. Debug, dừng tại main rồi F8 chạy. F6 bước qua, F5 đi vào hàm.

Project cấu hình phần cứng bằng mã trong `board.c`, không cần Generate Code từ CubeMX.
Các file `.c/.h` được biên dịch chung thành ELF/HEX; chỉ nạp file firmware kết quả.
Nếu workspace có project cùng tên ở vị trí cũ, dùng workspace mới hoặc bỏ project cũ
khỏi workspace, bỏ chọn Delete project contents on disk.

Có thể nạp `SmartSchoolBus_STM32F407.hex` bằng CubeProgrammer: chọn ST-LINK/SWD,
Connect, Open file, Download và Verify, sau đó Reset. Đóng phiên debug CubeIDE trước
khi dùng Programmer. HEX chứa địa chỉ Flash, không nhập lại địa chỉ của file BIN.
Gói phần mềm không xác nhận khả năng kết nối hiện tại của đầu nạp.

## 2. Nút và trình tự sử dụng

| Nút | Chân | Thao tác |
|---|---|---|
| SELECT | PD9 | Chuyển lựa chọn; tăng giá trị khi chỉnh đồng hồ |
| OK | PD10 | Mở mục, xác nhận; sang trường ngày giờ kế tiếp |
| CHECKED | PD11 | Nút cuối xe, xác nhận đã đi kiểm tra ghế |

Nút nối xuống GND, dùng pull-up. OK được xử lý khi nhả nút.
Nội dung chính dùng chữ 12×16; thông tin và lựa chọn dùng 6×8.

1. READY → SELECT chọn SET CLOCK → OK để đặt ngày giờ lần đầu.
2. READY → MANAGE CARDS → chọn tên → OK → quét thẻ để đăng ký.
3. READY → START TRIP → OK. Quét thẻ khi học sinh lên, số ON BUS tăng.
4. OK → START DROPPING OFF → CONFIRM. Quét thẻ khi học sinh xuống.
5. Khi đã trả hết: OK CHECK BUS, đi kiểm tra ghế rồi nhấn CHECKED cuối xe.
6. ALL DONE → OK HOME. Chuyến tiếp theo bắt đầu bằng START TRIP.

Kết thúc khi còn học sinh sẽ chuyển cảnh báo MISSING. Khi CHECK BUS quá 60 giây
chưa nhấn nút cuối xe sẽ báo OVERDUE. Khởi động lại giữa bước kiểm tra sẽ yêu cầu
kiểm tra lại ngay. Giữ sẵn nút cuối xe trước khi vào CHECK BUS không được chấp nhận.

## 3. Xuống xe thủ công khi quên hoặc mất thẻ

Chỉ có trong giai đoạn DROP OFF hoặc cảnh báo còn người trên xe.

1. SELECT tới MANUAL EXIT → OK.
2. SELECT chọn học sinh đang trên xe → OK. Màn hình hiện CONFIRM CHILD IS OUT.
3. Mặc định là CANCEL. Sau khi đã xác nhận đúng học sinh xuống xe, SELECT tới
   CONFIRM EXIT → OK.
4. Ghi SD thành công mới giảm ON BUS. Nhật ký ghi MANUAL_EXIT, mã HS và UID đã lưu.

Danh sách này chỉ chứa người đang trên xe và mục BACK. Việc xác nhận thủ công vẫn
phải kết thúc bằng CHECK BUS và nút CHECKED cuối xe. Nếu ghi SD lỗi, hệ thống STOP
và không cập nhật trạng thái trong RAM. Kiểm tra thẻ SD rồi khởi động lại.

## 4. Ngày giờ RTC

READY → SET CLOCK → chỉnh lần lượt YEAR, MONTH, DAY, HOUR, MINUTE.
SELECT tăng và quay vòng; OK sang trường tiếp theo. Màn SAVE TIME cho phép SAVE
hoặc CANCEL. Giây được đặt 00 khi lưu. Dùng giờ địa phương, ví dụ giờ Việt Nam.
Chỉ chỉnh khi đang READY; chuyến mới yêu cầu đồng hồ đã được đặt hợp lệ.

RTC dùng đồng hồ nội của STM32. Firmware ưu tiên LSE 32.768 kHz, tự chuyển sang LSI
nếu LSE không khởi động. Lần đầu có thể chờ vài giây. Khi lưu giờ bằng LSI,
OLED báo LSI CHECK TIME DAILY. LSI có sai số lớn hơn và cần đặt lại sau mất nguồn.
LSE chỉ giữ giờ khi tắt nguồn chính nếu kit có nguồn VBAT/pin dự phòng hoạt động.
Chưa đo độ chính xác hoặc xác nhận pin VBAT trên kit của bạn.

READY hiển thị ngày giờ, UNSET nếu chưa có giờ hợp lệ. Nếu đang khôi phục một chuyến
mà mất giờ, vẫn cho phép trả học sinh và kiểm tra cuối xe; các bản ghi khi đó mang
UNSET. Sau khi kết thúc chuyến, đặt lại giờ trước chuyến tiếp theo.

## 5. Danh sách học sinh trên SD

File ở thư mục gốc SD: `STUDENTS.CSV`. Mẫu trong `SD_CARD/STUDENTS.CSV`.
Nếu file chưa tồn tại, firmware tạo mẫu AN/BINH/CHI. Nếu SD đã có danh sách cần giữ,
chỉnh file hiện có, không chép đè mẫu. Tắt nguồn trước khi tháo SD, lắp lại và khởi động
để đọc danh sách mới.

```csv
id,name
HS01,AN
HS02,BINH
HS03,CHI
HS04,MINH
```

- Tối đa 32 học sinh. Mã duy nhất từ HS01 đến HS32, luôn đủ hai chữ số.
- Tên hiển thị 1–10 ký tự, chữ IN HOA không dấu, số, dấu cách hoặc dấu gạch ngang.
- Dùng dấu phẩy, tiêu đề chính xác `id,name`; UTF-8 có/không BOM đều được hỗ trợ.
- Thứ tự dòng là thứ tự menu. Đổi tên hoặc đảo dòng không làm chuyển thẻ sang người khác.
- HS01 luôn ứng với slot 0, HS02 với slot 1, ... HS32 với slot 31 trong log.
- Giữ nguyên mã của cùng học sinh. Đổi mã là đổi định danh; không tái dùng mã cũ
  cho người khác khi chưa xử lý thẻ của người cũ.
- Không bỏ người đã có điểm danh trong chuyến chưa kết thúc; firmware sẽ báo
  ROSTER CSV ERROR để yêu cầu khôi phục dòng bị thiếu.
- Bỏ một người khỏi CSV lúc READY chỉ ẩn người đó. UID đã đăng ký vẫn được giữ trong
  lịch sử và không thể đăng ký cho người khác. Thêm lại cùng HS để dùng lại, hoặc dùng
  RESET CARDS để xóa toàn bộ liên kết hiện hành rồi đăng ký lại.
- Đọc CSV lỗi, trùng mã, mã ngoài giới hạn, tên không hợp lệ: STOP / ROSTER CSV ERROR.
  Sửa file rồi khởi động lại. Firmware không ghi đè file lỗi bằng danh sách mẫu.

## 6. RESET CARDS và nhật ký

READY → MANAGE CARDS → SELECT qua danh sách đến RESET CARDS → OK.
Mặc định CANCEL; SELECT tới DELETE ALL → OK xác nhận.
Sau khi lưu thành công, tất cả UID và trạng thái điểm danh hiện hành được xóa.
Danh sách tên/mã trong STUDENTS.CSV vẫn giữ để đăng ký lại.
Chỉ thực hiện được khi chuyến đã kết thúc; không bỏ qua việc kiểm tra cuối xe.

`TRIP_LOG.CSV` được ghi và đồng bộ sau mỗi sự kiện, có CRC32 để phát hiện bản ghi lỗi.
Khởi động đọc lại log để khôi phục đăng ký thẻ, chuyến và trạng thái từng học sinh.
Các cột mới: seq, uptime_ms, trip, event, slot, uid, onboard, datetime, student_id, crc32.
Ngày giờ dạng YYYY-MM-DD HH:MM:SS; uptime_ms vẫn giữ để đối chiếu trong mỗi lần chạy.

Log firmware cũ được đọc theo định dạng cũ. Firmware thêm một dòng tiêu đề mới vào
cuối phần lịch sử, sau đó ghi định dạng có ngày giờ. Không sửa hoặc xóa các dòng cũ.
Vì vậy log nâng cấp có hai phần với hai tiêu đề; file mới chỉ có một tiêu đề mới.
Các bản ghi lịch sử không tự được bổ sung ngày giờ đã thiếu.
Không sửa TRIP_LOG.CSV bằng Excel hoặc lưu lại bằng trình soạn thảo; CRC/định dạng
không còn đúng sẽ làm hệ thống STOP. Sao chép file để xem và giữ file gốc trên SD.
Sau khi nâng cấp, không dùng firmware cũ đọc log mới.
RESET CARDS xóa liên kết đang sử dụng, vẫn giữ lịch sử UID cũ trong log.

## 7. Phần cứng và kiến trúc

| Thiết bị | Kết nối |
|---|---|
| RC522 | SPI2 PB13 SCK, PB14 MISO, PB15 MOSI, PB12 CS, PD8 RST, nguồn 3.3 V |
| OLED SSD1306 128×32 | I2C1 PB6 SCL, PB7 SDA, địa chỉ tự chọn 0x3C/0x3D |
| microSD trên kit | SDIO PC8–PC12, PD2; đang chạy 1-bit |
| Còi | PD12, BUZZER_ACTIVE_LOW=1 |
| Nút | PD9 SELECT, PD10 OK, PD11 CHECKED |

| Lớp | File chính |
|---|---|
| Giao diện và điều phối | main.c — task, menu, xác nhận và thông báo |
| Nghiệp vụ | trip.c — điều kiện chuyển trạng thái; calendar.c — lịch và ngày nhuận |
| Dịch vụ dữ liệu | roster.c — CSV; journal.c — log, CRC, khôi phục |
| Truy cập phần cứng | board.c, clock.c, oled.c, reader.c, disk.c |
| Nền tảng | STM32 HAL/CMSIS, FreeRTOS, FatFs |

Bốn task ứng dụng: Trip, Keys, RFID, Sound. Chỉ Trip dùng FatFs; chỉ Sound điều khiển
còi sau khởi tạo. Ghi SD không kéo dài một tiếng báo ngắn. Thẻ RC522 hỗ trợ UID 4 byte.

## 8. Debug và kiểm chứng

Đặt breakpoint tại handle, commit, transition, journal_append hoặc render.
Xem trip.phase, trip.status, trip.uid, trip.trip, view, choice, slot, roster_count,
roster_order, names, fault, roster_ok, reader_ok, screen_ok, sound_level, sd.ErrorCode.
Khi chạy bình thường sau khởi tạo: fault=false, roster_ok/reader_ok/screen_ok=true.
Khi dừng CPU bằng debugger, tác vụ còi cũng dừng; nếu dừng đúng lúc phát, còi có thể
giữ mức đang phát đến khi Resume. Đặt breakpoint lúc READY để dễ quan sát.

Đã clean build Debug bằng CubeIDE 2.2.0: 0 lỗi, 0 cảnh báo. Đã kiểm thử trên PC:
- Đăng ký, lên xe, quét trùng, trả bằng thẻ, trả thủ công, hủy xác nhận.
- Không giảm số người nếu ghi SD thất bại; không reset hoặc trả thủ công lúc PICKUP.
- Chỉ hoàn tất sau nút cuối xe hợp lệ; mất giờ không khóa việc kết thúc chuyến.
- Ngày nhuận, sửa tháng/năm tự giới hạn ngày, lưu thời gian.
- Reset, khởi động lại, đổi thứ tự/tên giữ đúng UID, từ chối CSV lỗi.
- Đọc log cũ và nối log mới; phát hiện CRC sai/bản ghi bị cắt.

Chạy lại bộ kiểm thử bằng Python 3 và GCC trên PC: `python Tests/run_tests.py`.
File Build_log.txt là log build của bản bàn giao.
**Bản bổ sung này chưa nạp thử trên kit.** Cần thử trên mạch luồng trả thủ công,
ngắt/bật nguồn để kiểm tra RTC và khôi phục SD, rồi đi đủ quy trình CHECKED.
