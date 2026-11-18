# Điều khiển hệ thống tưới

Mỗi lần gọi `IrrigationSystem::tick()`, hệ thống đọc một bộ số đo, tính lệnh bằng `Controller::update()`, rồi gửi lệnh đến driver cơ cấu chấp hành.

```text
IrrigationSystem::tick()
  SensorDriver::read() → Controller::update() → ActuatorDriver::apply()
```

## Gắn driver cho bo mạch

Driver công tắc đầy/cạn, cảm biến đất và relay cùng khung tích hợp ESP32/STM32 nằm trong [HARDWARE.md](HARDWARE.md). Khi dùng chúng, bật `useTankSwitches`. Driver có trạng thái `Pending` trong lúc chờ van mở trước khi bật P2.

Driver HX711, INA219 và ví dụ ghép các cảm biến nằm trong [SENSOR_MODULES.md](SENSOR_MODULES.md). Thư viện tương ứng cần được cài khi biên dịch hai driver này.

1. Tạo lớp kế thừa `irrigation::SensorDriver` và cài đặt `bool read(irrigation::Inputs& readings)`. Điền đầy đủ độ ẩm (%), mức nước (L), mức pin (%) và công suất khả dụng (W) trong một chu kỳ. Trả về `false` nếu thiếu số đo, đọc lỗi hoặc dữ liệu đã quá hạn. Đặt `backupAvailableW = 0` khi nguồn dự phòng chưa sẵn sàng.
2. Tạo lớp kế thừa `irrigation::ActuatorDriver` và cài đặt `bool apply(const irrigation::Outputs& commands)`. Ánh xạ `p1`, `p2`, `vg`, `vp` và `requestBackup` sang phần cứng. Driver chịu trách nhiệm về mức kích hoạt của relay và trình tự chuyển van, bơm. Trả về `false` nếu ghi lệnh thất bại.
3. Khởi tạo driver và chân GPIO về trạng thái an toàn khi khởi động. Tạo một đối tượng hệ thống dùng suốt chương trình, truyền hai driver đã khởi tạo vào constructor và gọi `tick()` theo chu kỳ. Hai driver cần tồn tại ít nhất bằng thời gian sống của hệ thống.

```cpp
BoardSensors sensors;
BoardActuators actuators;
irrigation::IrrigationSystem irrigationSystem(sensors, actuators);

// Gọi hàm này trong vòng lặp khi đến kỳ đọc và cập nhật.
irrigation::CycleResult runIrrigationCycle() {
    return irrigationSystem.tick();
}
```

`result.requested` chứa lệnh của bộ điều khiển và cờ cảnh báo. Nếu đọc lỗi, lệnh yêu cầu tắt hai bơm, đóng hai van và ngừng yêu cầu nguồn dự phòng. Nếu ghi lệnh lỗi, hệ thống thử gửi lại một lần với trạng thái tắt an toàn. `result.actuation` cho biết driver đã áp dụng lệnh gốc (`Applied`), đã áp dụng lệnh tắt an toàn (`SafeStateApplied`) hay đã báo lỗi cả hai lần (`OutputFailed`).

## Dùng cân để chọn đường tưới

Bật `config.useTankMass = true` khi driver đọc được tổng khối lượng bể và nước qua `readings.upperTankMassKg`. Trong chế độ này, driver có thể bỏ qua `readings.upperTankL`. Bộ điều khiển tự lấy khối lượng tổng trừ khối lượng bể rỗng để ước tính lượng nước (1 kg nước ≈ 1 L).

| Thông số cấu hình | Cách xác định |
| --- | --- |
| `emptyTankMassKg` | Cân bể rỗng khi đã lắp đặt, cùng các bộ phận mà cảm biến cân đỡ |
| `tankLitersPerMeter` | Chia lượng nước đổ thêm (L) cho độ tăng mực nước (m). Dùng cho bể có tiết diện ngang không đổi |
| `tankBottomToOutletM` | Độ cao đáy bể trừ độ cao đầu tưới (m). Có thể âm |
| `minGravityHeadM` | Ngưỡng dừng tưới trọng lực. Tắt P2, mở VG và đo lưu lượng để xác định cột nước tối thiểu đủ tưới |
| `gravityStartHeadM` | Ngưỡng bắt đầu ưu tiên trọng lực. Đặt cao hơn `minGravityHeadM` để giảm đóng ngắt quanh ngưỡng |

Cột nước bằng `tankBottomToOutletM + số lít / tankLitersPerMeter`. Khi cột nước đạt ngưỡng bắt đầu, bộ điều khiển ưu tiên VG và giữ P2 tắt. Khi cột nước giảm tới ngưỡng dừng, bộ điều khiển xét VP và P2 nếu cây còn cần tưới, bể còn đủ nước và đủ công suất. Lượng nước từ cân cũng dùng để xét nạp bể và dừng tưới khi nước quá ít.

Chế độ dùng cân mặc định tắt để các driver đo trực tiếp bằng lít tiếp tục hoạt động. Khi bật chế độ này, nếu cân mất số đo, giá trị vượt dung tích bể hoặc cấu hình hiệu chuẩn sai, chương trình báo `inputFault` và gửi lệnh tắt.

## Chạy ví dụ trên máy tính

Từ thư mục gốc của repo, biên dịch hai file lõi cùng ví dụ:

```sh
g++ -std=c++11 -I. IrrigationController.cpp IrrigationSystem.cpp examples/SimulatedDrivers.cpp -o irrigation_demo
./irrigation_demo
```

Ví dụ lần lượt mô phỏng tưới trọng lực, vừa nạp vừa tưới bằng bơm và mất tín hiệu cảm biến. Bản build firmware dùng hai file lõi cùng driver của bo mạch và vòng lặp riêng.
