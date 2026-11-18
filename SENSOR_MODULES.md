# Đọc độ ẩm đất, load cell và điện áp/dòng điện

## Bộ module cho mô hình DIY

| Đại lượng | Module được hỗ trợ | Driver | Giá trị trả về |
| --- | --- | --- | --- |
| Độ ẩm đất | Cảm biến điện dung analog dạng Capacitive Soil Moisture v1.2 hoặc tương đương | `AnalogSoilSensor` | Chỉ số 0–100% theo hai điểm hiệu chuẩn |
| Khối lượng bể | Load cell dạng thanh và module HX711 | `HX711TankSensor` | Tổng khối lượng bể và nước, kg |
| Điện áp/dòng điện DC | INA219 có shunt 0,1 ohm | `INA219ElectricalSensor` | Điện áp bus V, dòng có dấu A, công suất đo W |
| Mức đầy/cạn | Công tắc phao hoặc module mức có ngõ ra số | `HardwareSensors` | Ba trạng thái vùng nước như HARDWARE.md |

Chọn tải cân lớn hơn tổng khối lượng bể cùng lượng nước tối đa. Bể 5 L có khoảng 5 kg nước, cần cộng thêm khối lượng bể và phần gá được cân đỡ. Cấp nguồn và nối tín hiệu theo đúng module với mức GPIO 3,3 V của bo. Giữ phần điện tử của cảm biến đất ngoài nước. Driver INA219 ở đây dùng hiệu chuẩn mặc định của Adafruit cho shunt 0,1 ohm và giới hạn phần mềm tối đa 2 A, bus tối đa 26 V; kiểm tra dòng khởi động bơm trước khi đặt module trên đường tải.

## Cài thư viện và ghép driver

Arduino Library Manager cài `HX711 Arduino Library` của Bogdan Necula/bogde và `Adafruit INA219` cùng phụ thuộc `Adafruit BusIO`. Chọn Arduino core cho ESP32 hoặc STM32duino.

`examples/AllSensors/AllSensors.ino` ghép đầy đủ driver vào hệ thống. Chép các file .h/.cpp ở gốc repo vào thư mục sketch AllSensors, cài thư viện và điền chân/hiệu chuẩn. Chọn chân I2C cho đúng bo. Với STM32CubeIDE/HAL, các giao diện NumberSensor/ElectricalSensor vẫn dùng được nhưng cần cài đặt đọc ngoại vi bằng HAL.

```cpp
AnalogSoilSensor soil(soilPin, dryAdc, wetAdc);
HX711TankSensor mass(io, scaleConfig);
INA219ElectricalSensor electrical(0x40);
BufferedPowerSensor power(io, 2000);
HardwareSensors sensors(io, sensorWiring, soil, power, &mass, &electrical);
RelayActuators actuators(io, actuatorWiring);
IrrigationSystem irrigationSystem(sensors, actuators, config);
```

Gọi `mass.begin()`, `electrical.begin(Wire)`, `sensors.begin()` và `actuators.begin()` khi khởi động. Sau đó gọi `tick()` đều đặn. Cấu hình `useTankMass = true` và `useTankSwitches = true` cùng thông số hình học bể. Khi driver đã gắn nhưng đọc thất bại, HardwareSensors trả lỗi và hệ thống yêu cầu tắt.

## Hiệu chuẩn cảm biến đất

Đo ADC tại trạng thái đất khô để lấy `dryAdc`. Đo ở trạng thái đất đủ ẩm mà bạn chọn làm mốc 100% để lấy `wetAdc`. Driver tính `(ADC - dryAdc) * 100 / (wetAdc - dryAdc)` rồi giới hạn về 0–100. Đây là chỉ số tương đối theo hai mốc cho loại đất/cảm biến đang dùng. Cần chỉnh ngưỡng soilStartPct và soilStopPct theo phép thử tưới thực tế.

## Hiệu chuẩn HX711

1. Đọc ADC thô HX711 khi bàn cân không đỡ bể để lấy `zeroRaw`.
2. Đặt khối lượng chuẩn đã biết lên cân. Tính `countsPerKg = (raw có tải - zeroRaw) / số kg chuẩn`. Giá trị có thể âm tùy chiều dây tín hiệu.
3. Lắp bể rỗng, đọc khối lượng kg rồi đặt giá trị đó vào `Config::emptyTankMassKg`.
4. Điền hình học bể và ngưỡng cột nước như README. Driver trả tổng khối lượng bể/nước; Controller trừ khối lượng bể rỗng.

Driver đọc một mẫu khi `is_ready()` rồi dùng lại mẫu gần nhất trong thời gian `maxAgeMs` (ví dụ 500 ms). Không tự tare lúc khởi động vì bể có thể đang chứa nước. Chưa có mẫu, mẫu quá hạn, ADC chạm biên hoặc số kg âm sẽ báo lỗi. Sau lỗi đầu vào, control xóa trạng thái và xét lại ngưỡng ở lượt hợp lệ tiếp theo.

## INA219 và các biến trong Inputs

| Biến | Vai trò |
| --- | --- |
| `hasElectricalReading` | Có driver điện đã đọc thành công |
| `busVoltageV` | Điện áp bus tại phía tải của shunt |
| `busCurrentA` | Dòng điện có dấu theo chiều đấu IN+/IN- |
| `busPowerW` | `busVoltageV * busCurrentA`, giữ dấu dòng điện |

Gắn một module vào đường cần quan sát, ví dụ bus DC chung. Muốn đo riêng PV, pin và tải, cần các module ở những đường tương ứng, đổi địa chỉ I2C và tạo đối tượng driver riêng. Không đặt nhiều module cùng địa chỉ 0x40 trên cùng bus.

Phép đo trên tải đang chạy cho biết mức tiêu thụ hiện tại. Để quyết định có bật thêm bơm hay không, các trường `pvAvailableW`, `batteryAvailableW`, `batteryPct` và `backupAvailableW` cần dữ liệu từ bộ quản lý nguồn/BMS hoặc giới hạn nguồn đã được xác định trong module quản lý điện. INA219 riêng lẻ chưa cung cấp SOC pin và dự trữ công suất của PV.

Module quản lý nguồn cập nhật `BufferedPowerSensor::publish()` theo mẫu trong HARDWARE.md. Khi chưa có bản cập nhật mới, chương trình giữ lệnh tắt. Khung AllSensors thể hiện chỗ nối module đó.

## Kiểm tra bằng mô phỏng

```sh
g++ -std=c++11 -Wall -Wextra -Itests/fakes -I. IrrigationController.cpp IrrigationSystem.cpp HardwareDrivers.cpp HX711TankSensor.cpp INA219ElectricalSensor.cpp tests/SensorModulesTest.cpp -o sensor_modules_test
./sensor_modules_test
```

Thư mục `tests/fakes` cung cấp thiết bị giả cho kiểm thử trên máy tính. Kiểm thử kiểm tra hiệu chuẩn cân, dữ liệu quá hạn, lỗi I2C, đổi mA sang A, chiều dòng âm và luồng dữ liệu qua HardwareSensors. Mã chưa được thử trên bo thật.

API thư viện tham chiếu [HX711 của bogde](https://github.com/bogde/HX711) và [Adafruit INA219](https://github.com/adafruit/Adafruit_INA219). Giới hạn bus 26 V tham chiếu [TI INA219](https://www.ti.com/product/INA219).
