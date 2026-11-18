# Driver cảm biến và cơ cấu chấp hành

## Cảm biến mức logic và vị trí lắp

Quy ước dưới đây dùng trạng thái đã chuẩn hóa: `true` nghĩa là nước chạm cảm biến. Driver đổi mức HIGH/LOW bằng trường `wetHigh`.

| Công tắc thấp | Công tắc đầy | Thông tin xác định được |
| --- | --- | --- |
| Khô | Khô | Nước dưới vị trí công tắc thấp |
| Ướt | Khô | Nước giữa công tắc thấp và công tắc đầy |
| Ướt | Ướt | Nước đạt hoặc cao hơn vị trí công tắc đầy |
| Khô | Ướt | Tín hiệu mâu thuẫn, báo lỗi đầu vào |

Đo độ cao từng công tắc từ đáy bể giúp xác định ranh giới của các vùng. Với bể có tiết diện ngang không đổi, số lít tại ngưỡng bằng `tankLitersPerMeter * độ cao công tắc (m)`. Khi nước giữa hai công tắc, chỉ biết khoảng mực nước. Một giá trị offset cố định không cung cấp thêm phép đo trong khoảng đó.

Ví dụ công tắc thấp đặt tại 0,05 m và công tắc đầy tại 0,40 m. Khi thấp ướt, đầy khô, nước nằm trong khoảng 0,05 đến dưới 0,40 m. Muốn tính độ cao liên tục, dùng cân đã trừ bì và biết hình dạng bể, hoặc một cảm biến đo liên tục.

Lắp công tắc thấp trên mức cần che đầu hút P2 để dừng tưới trước khi bơm hút cạn. Công tắc nguồn dưới xác nhận nước che đầu hút P1. Công tắc đầy đặt tại mức dừng nạp. Chọn vị trí và ngưỡng tưới trọng lực theo lưu lượng đo thực tế ở đầu tưới.

## Phần control nhận gì?

Bật `Config::useTankSwitches = true` khi dùng `HardwareSensors`.

- Công tắc thấp khô khóa tưới, công tắc nguồn khô khóa P1, công tắc đầy ướt khóa P1.
- Khi `useTankMass = false`, control dùng vùng nước từ công tắc. Chỉ ưu tiên VG khi công tắc đầy đang ướt. Vùng giữa hai công tắc dùng nhánh VP + P2 nếu có nhu cầu và đủ điện. Driver giữ các trường số lít ở 0 và control bỏ qua chúng trong chế độ này.
- Khi `useTankMass = true`, cân cung cấp `upperTankMassKg` và control tính cột nước theo hướng dẫn trong README. Công tắc thấp và đầy vẫn có quyền khóa tưới/nạp. Cột nước là chênh lệch độ cao giữa mặt nước và đầu tưới, trong bể thông với khí trời.
- Trường hợp đầy ướt, thấp khô hoặc đọc cảm biến lỗi sẽ gửi lệnh tắt. Trong thời gian chờ chống dội, driver cũng báo chưa có dữ liệu ổn định. Gọi `tick()` đều đặn để nhận lại dữ liệu sau khi tín hiệu ổn định.

## Cấu trúc driver

```text
ESP32 Arduino core / STM32duino / adapter STM32 HAL
                         ↓ BoardIo
Công tắc mức ───┐
NumberSensor ───┼─ HardwareSensors ─ Inputs ─ Controller
PowerSensor ───┘                                  ↓ Outputs
                                       RelayActuators ─ BoardIo ─ GPIO
```

`HardwareDrivers.h/.cpp` chạy độc lập với SDK của bo mạch. `ArduinoBoardIo.h` cài đặt GPIO bằng Arduino core cho ESP32 hoặc STM32duino. Nếu dùng STM32CubeIDE/HAL, cài đặt lớp kế thừa `BoardIo` bằng hàm GPIO và đồng hồ của HAL. Có thể dùng lại hai driver còn lại.

`AnalogSoilSensor` nhận một chân ADC và hai số ADC đã đo thực tế (đất khô, đất đủ ẩm), đổi tuyến tính sang 0–100%. `CallbackNumberSensor` nhận hàm đọc số thực để ghép thư viện cân. `BufferedPowerSensor` nhận bộ số đo điện từ module của bạn và từ chối dữ liệu đã quá hạn. Mọi hàm đọc số đo điện/cân phải trả lỗi khi thiết bị mất kết nối hoặc dữ liệu quá hạn.

## Cắm vào IrrigationSystem

Chọn chân theo sơ đồ đúng model bo. `SensorWiring` có thứ tự: công tắc thấp, công tắc đầy, công tắc nguồn, thời gian chống dội (ms). Mỗi công tắc có `{pin, wetHigh, pullup}`. `ActuatorWiring` có thứ tự P1, VG, VP, P2, nguồn dự phòng, thời gian chờ VP mở (ms). Mỗi relay có `{pin, activeHigh}`. Mỗi thiết bị dùng chân riêng.

Ví dụ, `{pin, false, true}` là đầu vào kích LOW với điện trở kéo lên. Relay `{pin, false}` bật khi chân LOW. Giá trị này cần khớp thiết bị thực tế. GPIO đọc được mức logic ổn định vẫn có thể không phát hiện dây đứt hoặc phao kẹt; `Applied` biểu thị driver ghi lệnh thành công, phản hồi cơ khí cần cảm biến riêng.

`examples/BoardIntegration/BoardIntegration.ino` là khung ghép driver. Các chân khởi tạo bằng -1 và hai mốc ADC bằng 0 để thể hiện thông số còn thiếu. Điền chúng trước khi sử dụng. Biên dịch cùng ba file `IrrigationController.cpp`, `IrrigationSystem.cpp` và `HardwareDrivers.cpp`. Với Arduino IDE, chép các file .cpp và .h của repo vào cùng thư mục sketch `BoardIntegration`.

Trong `loop()`, module đo điện của bạn cập nhật theo mẫu:

```cpp
PowerReadings readings;
readings.batteryPct = batteryPercent;
readings.pvAvailableW = availableSolarWatts;
readings.batteryAvailableW = allowedBatteryWatts;
readings.backupAvailableW = readyBackupWatts;
power.publish(readings);
```

Các biến bên phải là số đo do module điện/BMS cung cấp. Ví dụ mẫu chưa cài đặt module điện hay thư viện cân cụ thể vì thiết bị và sơ đồ chân chưa xác định. Công suất nguồn dự phòng chỉ cập nhật sau khi xác nhận nguồn đã sẵn sàng. Thiếu bộ số đo điện mới làm hệ thống trả lệnh tắt.

Để cắm cân, viết hàm `bool readTankMassKg(float& kg)` dùng thư viện của module cân, rồi tạo `CallbackNumberSensor mass(readTankMassKg)` và truyền `&mass` vào tham số cuối của `HardwareSensors`. Bật `useTankMass` và điền các thông số hiệu chuẩn. Driver trả tổng khối lượng bể cùng nước theo kg.

## Trình tự cơ cấu chấp hành

`actuators.begin()` thiết lập các đầu ra OFF. Khi chọn VP, driver dừng P2, đóng VG, mở VP, sau đó chờ `valveSettleMs` trước khi bật P2. Việc chờ dùng đồng hồ, tiếp tục gọi `tick()` để tiến trình chạy. Khi đang chờ, kết quả là `ActuationStatus::Pending`. Nếu yêu cầu dừng hoặc đổi sang VG, driver dừng P2 trước khi đóng VP.

Khi ghi đầu ra lỗi, driver thử tắt mọi đầu ra, kể cả nếu một chân đã báo lỗi. `IrrigationSystem` cũng thử gửi bộ lệnh tắt và báo kết quả. Thời gian chờ VP mở cần đo theo van thực tế. Driver này dành cho van có lệnh ON để mở, OFF để đóng; van chốt hoặc van cần đảo chiều cần driver riêng.

## Kiểm thử trên máy tính

```sh
g++ -std=c++11 -Wall -Wextra -I. IrrigationController.cpp IrrigationSystem.cpp HardwareDrivers.cpp tests/HardwareDriversTest.cpp -o hardware_test
./hardware_test
```

Kiểm thử mô phỏng tín hiệu HIGH/LOW, chống dội, bể đầy/cạn, nguồn dưới cạn, mất số đo, chờ van, lỗi ghi, chuyển đường tưới, đồng hồ tràn và khóa nạp/tưới theo công tắc khi dùng cân.

API GPIO của adapter Arduino tham chiếu tài liệu chính thức của [ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html) và [STM32duino](https://github.com/stm32duino/Arduino_Core_STM32/wiki/API).
