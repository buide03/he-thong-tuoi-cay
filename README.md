# Điều khiển hệ thống tưới

Mỗi lần gọi `IrrigationSystem::tick()`, hệ thống đọc một bộ số đo, tính lệnh bằng `Controller::update()`, rồi gửi lệnh đến driver cơ cấu chấp hành.

```text
IrrigationSystem::tick()
  SensorDriver::read() → Controller::update() → ActuatorDriver::apply()
```

## Gắn driver cho bo mạch

1. Tạo lớp kế thừa `irrigation::SensorDriver` và cài đặt `bool read(irrigation::Inputs& readings)`. Điền đầy đủ độ ẩm (%), mức nước (L), mức pin (%) và công suất khả dụng (W) trong một chu kỳ. Trả về `false` nếu thiếu số đo, đọc lỗi hoặc dữ liệu đã quá hạn. Đặt `backupAvailableW = 0` khi nguồn dự phòng chưa sẵn sàng.
2. Tạo lớp kế thừa `irrigation::ActuatorDriver` và cài đặt `bool apply(const irrigation::Outputs& commands)`. Ánh xạ `p1`, `p2`, `vg`, `vp` và `requestBackup` sang phần cứng. Driver chịu trách nhiệm về mức kích hoạt của relay và trình tự chuyển van, bơm. Trả về `false` nếu ghi lệnh thất bại.
3. Khởi tạo driver và chân GPIO về trạng thái an toàn khi khởi động. Tạo một đối tượng hệ thống dùng suốt chương trình, truyền hai driver đã khởi tạo vào constructor và gọi `tick()` theo chu kỳ. Hai driver cần tồn tại ít nhất bằng thời gian sống của hệ thống.

```cpp
BoardSensors sensors;
BoardActuators actuators;
irrigation::IrrigationSystem system(sensors, actuators);

// Gọi hàm này trong vòng lặp khi đến kỳ đọc và cập nhật.
irrigation::CycleResult runIrrigationCycle() {
    return system.tick();
}
```

`result.requested` chứa lệnh của bộ điều khiển và cờ cảnh báo. Nếu đọc lỗi, lệnh yêu cầu tắt hai bơm, đóng hai van và ngừng yêu cầu nguồn dự phòng. Nếu ghi lệnh lỗi, hệ thống thử gửi lại một lần với trạng thái tắt an toàn. `result.actuation` cho biết driver đã áp dụng lệnh gốc (`Applied`), đã áp dụng lệnh tắt an toàn (`SafeStateApplied`) hay đã báo lỗi cả hai lần (`OutputFailed`).

## Chạy ví dụ trên máy tính

Từ thư mục gốc của repo, biên dịch hai file lõi cùng ví dụ:

```sh
g++ -std=c++11 -I. IrrigationController.cpp IrrigationSystem.cpp examples/SimulatedDrivers.cpp -o irrigation_demo
./irrigation_demo
```

Ví dụ lần lượt mô phỏng tưới trọng lực, vừa nạp vừa tưới bằng bơm và mất tín hiệu cảm biến. Bản build firmware dùng hai file lõi cùng driver của bo mạch và vòng lặp riêng.
