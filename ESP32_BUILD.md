# Cấu hình ESP32 mẫu cho hệ tưới điện mặt trời

## Phần cứng được chọn

- ESP32 DevKit dùng ESP32-WROOM-32.
- Bể cao 5 L, load cell có tải định mức lớn hơn tổng khối lượng bể và nước, module HX711.
- Cảm biến độ ẩm đất điện dung analog, ba công tắc mức (bể cao thấp/đầy, nguồn dưới còn nước).
- Hai INA219 shunt 0,1 ohm, địa chỉ 0x40 và 0x41.
- Hệ DC 12 V, ắc quy chì kín khí 12 V (ví dụ 7 Ah), bộ sạc năng lượng mặt trời phù hợp loại ắc quy. Tấm PV khoảng 10 W, điện áp hở mạch tối đa phải nằm trong giới hạn bus INA219 26 V.
- Bơm và van DC 12 V có công suất thực đo phù hợp ngân sách mẫu 15 W, dòng trên mỗi INA219 trong dải đã cấu hình (tối đa 2 A). Tính cả dòng khởi động khi chọn module và tải.
- Module relay tương thích đầu vào 3,3 V, kích LOW, van thường đóng mở bằng cấp điện. Bộ hạ áp 12 V xuống 5 V cấp chân nguồn phù hợp của DevKit. Không nối 12 V vào GPIO.

Đây là lựa chọn thiết kế cho ví dụ. Các ngưỡng điện áp là điểm khởi đầu để hiệu chỉnh cho ắc quy chì 12 V và tải cụ thể. Nếu thay sang lithium, cần profile ngưỡng, bộ sạc và BMS phù hợp loại pin.

## Sơ đồ chân

| Chức năng | GPIO |
| --- | --- |
| Cảm biến đất AO | 34 |
| HX711 DT / SCK | 32 / 33 |
| INA219 SDA / SCL chung | 21 / 22 |
| Công tắc bể thấp / đầy | 16 / 17 |
| Công tắc nguồn dưới | 18 |
| Relay P1 / VG / VP / P2 | 19 / 23 / 25 / 26 |
| Relay yêu cầu nguồn dự phòng | 27 |

Công tắc mẫu đóng xuống GND khi nước chạm, INPUT_PULLUP. Nguồn logic cảm biến và điện trở kéo I2C dùng mức phù hợp GPIO 3,3 V. Chung GND theo thiết kế bộ nguồn. Đấu tiếp điểm tải theo sơ đồ module, có bảo vệ tải cảm ứng phù hợp. GPIO16/17 trong bảng dành cho WROOM; kiểm tra lại nếu dùng bo khác có PSRAM hoặc chân đã được dùng.

## Vị trí hai cảm biến điện

```text
PV+ → IN+ INA219 0x40 → IN- → đầu PV+ bộ sạc

Ắc quy+ (qua bảo vệ phù hợp)
  → IN+ INA219 0x41 → IN- → nút BAT+/tải của hệ quản lý nguồn
```

INA219 pin có chiều dương khi pin xả ra hệ thống, âm khi pin được sạc. Tất cả đường sạc và xả của pin cần đi qua cùng shunt để đọc dòng tổng. Nếu dùng BMS hoặc bộ sạc có các cổng riêng, xác định lại điểm đo theo sơ đồ của thiết bị. Không dùng đường đo để đi vòng bảo vệ BMS.

Đổi địa chỉ module pin thành 0x41 theo sơ đồ jumper A0/A1 của module, xác nhận bằng quét I2C. Chọn vị trí nối tải sao cho giới hạn nguồn của ví dụ phản ánh đường cấp điện thực tế.

## Ngân sách điện trong chương trình

`DualSourcePowerSensor` đọc hai module mỗi lượt, truyền cả hai bộ V/A/W qua `HardwareSensors` vào `Inputs`. Lỗi một trong hai phép đo làm lượt điều khiển trả lỗi đầu vào.

- Phần PV được dự toán bằng công suất PV đo đang cấp nhân hiệu suất mẫu 0,8, giới hạn tối đa 10 W và ngân sách tổng.
- Ngân sách xả pin lấy điện áp pin nhân dòng xả cho phép đã cấu hình 1,2 A. Tổng PV và pin được chặn ở 15 W.
- Đây là ngân sách theo giới hạn thiết kế và phép đo hiện tại. Thay đổi thời tiết, đáp ứng bộ sạc, sụt áp và dòng khởi động cần được kiểm thử trên hệ thật.
- `useBatteryVoltage = true` dùng ngưỡng dừng 11,8 V và cho phép lại 12,4 V. Mức 13,5 V là điều kiện điện áp để xét tận dụng PV nạp bể khi đất đã đủ ẩm. Ngưỡng này không xác nhận ắc quy đã đầy.
- INA219 chưa cung cấp SOC. `batteryPct` trong bộ đọc hai nguồn để 0 và được bỏ qua khi chọn ngưỡng pin ở chế độ điện áp. Mức pin phần trăm có thể tích hợp từ BMS/fuel gauge sau.
- Ví dụ có relay yêu cầu nguồn dự phòng nhưng chưa lắp cảm biến xác nhận nguồn dự phòng. `backupAvailableW` bằng 0. Relay yêu cầu sẽ không tự tạo ra công suất để bật P1.

## Hiệu chuẩn và biên dịch

1. Mở `examples/ESP32SolarIrrigation/ESP32SolarIrrigation.ino`. Chép các .cpp/.h ở gốc repo vào cùng thư mục sketch, giữ `BoardConfig.h` cạnh file .ino.
2. Cài Arduino core ESP32, HX711 của bogde và Adafruit INA219 cùng Adafruit BusIO. Chọn bo ESP32 Dev Module tương ứng DevKit.
3. Đo và điền hai mốc ADC của cảm biến đất, zeroRaw/countsPerKg của HX711, khối lượng bể rỗng, số lít trên mỗi mét và cao độ đầu tưới trong `BoardConfig.h`. Các giá trị hiệu chuẩn để 0 thể hiện phép đo còn thiếu.
4. Đo lưu lượng khi tắt P2 và mở VG ở nhiều mực nước để chọn hai ngưỡng cột nước. Đo công suất bơm/van và cập nhật `Config` thay cho số công suất mẫu.
5. Nạp chương trình, mở Serial Monitor 115200. Theo dõi hai bộ V/A/W, các lệnh bơm/van, inputFault và mã ActuationStatus.

Ví dụ không yêu cầu gửi `power.publish()` trong loop vì driver hai nguồn đọc trực tiếp. Ngưỡng điện áp có hai mức để hạn chế đóng ngắt khi pin dao động. Chương trình vẫn dừng khi công tắc cạn báo thiếu nước, khóa nạp khi công tắc đầy ướt, và chờ VP mở trước khi chạy P2.

Đã có kiểm thử trên máy tính cho bộ đọc hai nguồn và tích hợp controller. Cần hiệu chuẩn và thử thực tế trước khi vận hành tự động.

Có thể nạp riêng `examples/CalibrateSensors/CalibrateSensors.ino` để xem ADC đất và ADC thô HX711 trên Serial Monitor. Ghi nhiều mẫu ổn định và lấy trung bình. Với HX711, `zeroRaw` đo khi chưa đặt bể, `countsPerKg = (ADC có khối lượng chuẩn - zeroRaw) / số kg chuẩn`. Khối lượng bể rỗng nhập riêng vào `emptyTankMassKg` sau khi đã quy đổi ADC sang kg.

Tham chiếu API [ESP32 I2C](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html) và giới hạn bus [TI INA219](https://www.ti.com/product/INA219).
