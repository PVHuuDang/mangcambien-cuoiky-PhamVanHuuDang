# Điều khiển quạt điện bằng giọng nói sử dụng Edge Impulse và ESP32-S3

## 1. Giới thiệu đề tài

Đây là repository lưu trữ mã nguồn và tài liệu cho báo cáo cuối kỳ môn **Mạng cảm biến**.

Đề tài thực hiện hệ thống **điều khiển quạt điện bằng giọng nói** dựa trên bài toán **Audio Keyword Spotting**. Mô hình nhận diện lệnh được huấn luyện trên nền tảng **Edge Impulse**, sau đó được export dưới dạng **Arduino Library** để triển khai lên **ESP32-S3**.

Hệ thống sử dụng microphone **INMP441** để thu âm thanh, ESP32-S3 để chạy mô hình nhận diện lệnh và MOSFET để điều khiển quạt DC 5V bằng tín hiệu PWM.

---

## 2. Thông tin sinh viên

| Nội dung      | Thông tin                           |
| ------------- | ----------------------------------- |
| Họ và tên     | Phạm Văn Hữu Đang                   |
| MSSV          | N23DCCI010                          |
| Lớp           | D23CQCI01-N                         |
| Học phần      | Mạng cảm biến                       |
| Tên đề tài    | Điều khiển quạt điện bằng giọng nói |
| Nền tảng AI   | Edge Impulse                        |
| Vi điều khiển | ESP32-S3                            |

---

## 3. Chức năng chính

Hệ thống có các chức năng chính sau:

* Thu âm lệnh giọng nói bằng microphone INMP441.
* Nhận diện 5 nhãn âm thanh:

  * `huu dang`
  * `quat len`
  * `quat xuong`
  * `tat quat`
  * `noise`
* Dùng lệnh `huu dang` để kích hoạt hệ thống.
* Dùng lệnh `quat len` để tăng tốc độ quạt.
* Dùng lệnh `quat xuong` để giảm tốc độ quạt.
* Dùng lệnh `tat quat` để tắt quạt.
* Bỏ qua nhãn `noise` để tránh điều khiển nhầm do âm thanh nền.
* Điều khiển quạt DC 5V thông qua MOSFET bằng tín hiệu PWM.

---

## 4. Cấu trúc thư mục đề xuất

```text
.
├── README.md
├── report/
│   └── Bao_cao_Audio_KWS_ESP32_Pham_Van_Huu_Dang.pdf
├── arduino/
│   └── Voice_Fan_Control_ESP32S3.ino
├── edge_impulse/
│   └── ei-dieu_khien_quat_bang_giong_noi-arduino-1.0.3-impulse.zip

```


---

## 5. File trong repository

### 5.1. File Edge Impulse Arduino Library

File `.zip` trong thư mục `edge_impulse/` là thư viện Arduino được export từ Edge Impulse sau khi huấn luyện mô hình.

File này chứa:

* Mô hình TensorFlow Lite.
* Khối xử lý đặc trưng MFCC.
* Danh sách label.
* Các file cấu hình Edge Impulse.
* Code mẫu triển khai trên vi điều khiển.

Thông tin mô hình:

| Nội dung        | Giá trị                        |
| --------------- | ------------------------------ |
| Project name    | Dieu_Khien_Quat_Bang_Giong_Noi |
| Project ID      | 1020777                        |
| Library version | 1.0.3                          |
| Kiểu cảm biến   | Microphone                     |
| Engine          | TensorFlow Lite                |
| Sample rate     | 16000 Hz                       |
| Window size     | 1000 ms                        |
| Stride          | 500 ms                         |
| MFCC            | 13 hệ số                       |
| Số label        | 5                              |

---

### 5.2. File Arduino `.ino`

File `.ino` trong thư mục `arduino/` là chương trình triển khai trên ESP32-S3.

Chương trình thực hiện các nhiệm vụ:

* Khởi tạo microphone INMP441 qua giao tiếp I2S.
* Thu âm thanh liên tục.
* Chạy mô hình Edge Impulse trên ESP32-S3.
* Lấy label có score cao nhất.
* So sánh label với các lệnh đã định nghĩa.
* Xuất PWM điều khiển MOSFET.
* Điều khiển tốc độ quạt DC 5V.

---

## 6. Phần cứng sử dụng

| STT | Linh kiện           | Chức năng                                                  |
| --- | ------------------- | ---------------------------------------------------------- |
| 1   | ESP32-S3            | Vi điều khiển trung tâm, chạy mô hình AI và điều khiển PWM |
| 2   | INMP441             | Microphone I2S dùng để thu âm thanh                        |
| 3   | MOSFET IRLZ44N      | Điều khiển dòng cấp cho quạt                               |
| 4   | Quạt DC 5V          | Tải được điều khiển                                        |
| 5   | Nguồn ngoài 5V      | Cấp nguồn riêng cho quạt                                   |
| 6   | Điện trở            | Hạn dòng Gate và kéo xuống Gate MOSFET                     |
| 7   | Breadboard, dây nối | Lắp ráp mạch thử nghiệm                                    |

---

## 7. Bảng kết nối linh kiện

| STT | Linh kiện      | Chân linh kiện | Kết nối đến                              | Ghi chú                       |
| --- | -------------- | -------------- | ---------------------------------------- | ----------------------------- |
| 1   | ESP32-S3       | 3V3            | VCC/VDD INMP441                          | Cấp nguồn 3.3V cho microphone |
| 2   | ESP32-S3       | GND            | GND INMP441, GND nguồn 5V, Source MOSFET | Mass chung hệ thống           |
| 3   | ESP32-S3       | GPIO 26        | SCK/BCLK INMP441                         | Clock I2S                     |
| 4   | ESP32-S3       | GPIO 32        | WS/LRCL INMP441                          | Word Select I2S               |
| 5   | ESP32-S3       | GPIO 33        | SD/DOUT INMP441                          | Dữ liệu âm thanh              |
| 6   | ESP32-S3       | GPIO 25        | Gate MOSFET qua điện trở                 | Xuất PWM điều khiển quạt      |
| 7   | MOSFET IRLZ44N | Gate           | GPIO PWM ESP32-S3                        | Nhận tín hiệu điều khiển      |
| 8   | MOSFET IRLZ44N | Drain          | Cực âm quạt DC                           | Đường dòng tải                |
| 9   | MOSFET IRLZ44N | Source         | GND chung                                | Mass mạch công suất           |
| 10  | Quạt DC 5V     | Cực dương      | +5V nguồn ngoài                          | Nguồn cấp cho quạt            |
| 11  | Quạt DC 5V     | Cực âm         | Drain MOSFET                             | Điều khiển qua MOSFET         |
| 12  | Nguồn ngoài 5V | GND            | GND ESP32-S3                             | Bắt buộc nối chung mass       |

Lưu ý: Quạt DC không được cấp nguồn trực tiếp từ GPIO của ESP32-S3. GPIO chỉ dùng để điều khiển Gate MOSFET.

---

## 8. Cấu hình mô hình Edge Impulse

Mô hình được huấn luyện trên Edge Impulse theo bài toán **Audio Keyword Spotting**.

| Thông số                    | Giá trị        |
| --------------------------- | -------------- |
| Input                       | Audio          |
| Sample rate                 | 16000 Hz       |
| Window size                 | 1000 ms        |
| Stride                      | 500 ms         |
| Processing block            | MFCC           |
| Number of MFCC coefficients | 13             |
| Learning block              | Classification |
| Output labels               | 5              |

Danh sách label trong Arduino Library:

```cpp
huu dang
noise
quat len
quat xuong
tat quat
```

---

## 9. Cài đặt thư viện Edge Impulse vào Arduino IDE

Thực hiện các bước sau:

1. Mở Arduino IDE.
2. Chọn `Sketch`.
3. Chọn `Include Library`.
4. Chọn `Add .ZIP Library...`.
5. Chọn file `.zip` trong thư mục `edge_impulse/`.
6. Chờ Arduino IDE cài đặt thư viện.
7. Mở file `.ino` trong thư mục `arduino/`.
8. Chọn đúng board ESP32-S3.
9. Compile và upload chương trình lên ESP32-S3.

---

## 10. Cấu hình chân trong code

Trong file `.ino`, các chân kết nối chính được khai báo như sau:

```cpp
#define I2S_BCLK_PIN      26
#define I2S_WS_PIN        32
#define I2S_DIN_PIN       33

#define FAN_PWM_PIN       25
#define FAN_PWM_CHANNEL   0
#define FAN_PWM_FREQ      5000
#define FAN_PWM_RES       8
```

Nếu đấu dây khác với bảng kết nối, cần sửa lại các giá trị GPIO trên cho đúng với mạch thực tế.

---

## 11. Nguyên lý hoạt động

Quy trình hoạt động của hệ thống:

1. INMP441 thu âm thanh từ môi trường.
2. ESP32-S3 đọc dữ liệu âm thanh qua giao tiếp I2S.
3. Dữ liệu âm thanh được đưa vào mô hình Edge Impulse.
4. Mô hình phân loại âm thanh thành một trong 5 label.
5. Nếu label có score lớn hơn ngưỡng, chương trình xử lý lệnh.
6. Lệnh `huu dang` kích hoạt hệ thống.
7. Lệnh `quat len` tăng mức PWM.
8. Lệnh `quat xuong` giảm mức PWM.
9. Lệnh `tat quat` đưa PWM về 0 để tắt quạt.
10. Label `noise` bị bỏ qua.

---

## 12. Bảng ánh xạ lệnh và hành động

| Lệnh nhận diện | Điều kiện                  | Hành động          |
| -------------- | -------------------------- | ------------------ |
| `huu dang`     | Score > 0.80               | Kích hoạt hệ thống |
| `quat len`     | Đã kích hoạt, Score > 0.80 | Tăng tốc độ quạt   |
| `quat xuong`   | Đã kích hoạt, Score > 0.80 | Giảm tốc độ quạt   |
| `tat quat`     | Score > 0.80               | Tắt quạt           |
| `noise`        | Âm thanh nền               | Không điều khiển   |

---

## 13. Bảng mức PWM điều khiển quạt

| Mức tốc độ | Giá trị PWM | Duty cycle xấp xỉ | Trạng thái quạt      |
| ---------- | ----------: | ----------------: | -------------------- |
| Mức 0      |           0 |                0% | Quạt tắt             |
| Mức 1      |          85 |               33% | Quạt quay chậm       |
| Mức 2      |         170 |               67% | Quạt quay trung bình |
| Mức 3      |         255 |              100% | Quạt quay nhanh      |

---

## 14. Cách chạy thử

### Bước 1: Kiểm tra phần cứng

* Kiểm tra ESP32-S3 đã được cấp nguồn.
* Kiểm tra INMP441 nối đúng chân I2S.
* Kiểm tra MOSFET nối đúng Gate, Drain, Source.
* Kiểm tra quạt dùng nguồn ngoài 5V.
* Kiểm tra GND nguồn ngoài đã nối chung với GND ESP32-S3.

### Bước 2: Upload code

* Mở file `.ino`.
* Chọn đúng board ESP32-S3.
* Chọn đúng cổng COM.
* Nhấn Upload.

### Bước 3: Mở Serial Monitor

Thiết lập baud rate:

```text
115200
```

### Bước 4: Kiểm tra lệnh

Nói lần lượt các lệnh:

```text
hữu đang
quạt lên
quạt xuống
tắt quạt
```

Kết quả mong muốn:

* Serial Monitor hiển thị label và score.
* Quạt thay đổi tốc độ tương ứng với lệnh.
* Khi có tiếng nền, hệ thống nhận là `noise` hoặc bỏ qua.

---

## 15. Kết quả mong đợi trên Serial Monitor

Ví dụ kết quả khi nhận diện đúng lệnh:

```text
========== Ket qua nhan dien ==========
huu dang: 0.912
noise: 0.015
quat len: 0.030
quat xuong: 0.018
tat quat: 0.025
Label cao nhat: huu dang | Score: 0.912
He thong da duoc kich hoat. Hay noi lenh dieu khien quat.
=======================================
```

Ví dụ khi điều khiển quạt:

```text
Lenh nhan dien: quat len | Score: 0.887
Muc quat hien tai: 1 | PWM = 85
Da tang toc do quat.
```

---

## 16. Lưu ý an toàn

* Không cấp nguồn quạt trực tiếp từ GPIO của ESP32-S3.
* Phải dùng MOSFET hoặc driver để điều khiển quạt.
* GND của nguồn ngoài 5V phải nối chung với GND ESP32-S3.
* Nên dùng diode chống ngược song song với quạt để bảo vệ MOSFET.
* Nên dùng tụ lọc nguồn nếu ESP32-S3 bị reset khi quạt khởi động.
* Trước khi chạy mô hình, nên test riêng phần PWM điều khiển quạt.

---

## 17. Một số lỗi thường gặp

| Lỗi                               | Nguyên nhân có thể                               | Cách khắc phục                                    |
| --------------------------------- | ------------------------------------------------ | ------------------------------------------------- |
| Không compile được                | Chưa cài thư viện Edge Impulse `.zip`            | Cài lại bằng Add .ZIP Library                     |
| Không nhận âm thanh               | Sai chân I2S hoặc microphone chưa cấp nguồn      | Kiểm tra BCLK, WS, SD, 3V3, GND                   |
| Serial Monitor không hiện kết quả | Sai baud rate hoặc chưa upload thành công        | Chọn baud rate 115200                             |
| Quạt không chạy                   | Sai chân PWM, MOSFET đấu sai, chưa nối GND chung | Kiểm tra mạch công suất                           |
| ESP32-S3 bị reset khi quạt chạy   | Nguồn yếu hoặc nhiễu từ quạt                     | Dùng nguồn riêng, tụ lọc, nối mass đúng           |
| Nhận diện sai lệnh                | Dữ liệu train ít hoặc nhiễu môi trường           | Thu thêm mẫu, tăng dữ liệu noise, chỉnh threshold |

---

## 18. Hướng phát triển

Một số hướng phát triển tiếp theo:

* Thu thêm dữ liệu từ nhiều người nói khác nhau.
* Bổ sung nhiều mức điều khiển tốc độ quạt hơn.
* Thêm màn hình OLED/LCD hiển thị trạng thái nhận diện.
* Thêm kết nối WiFi để giám sát trạng thái quạt.
* Tối ưu mô hình để giảm thời gian inference trên ESP32-S3.
* Thiết kế PCB thay cho breadboard để mạch ổn định hơn.

---

## 19. Tài liệu tham khảo

* Edge Impulse Documentation.
* Edge Impulse Keyword Spotting Tutorial.
* ESP32-S3 Arduino Core Documentation.
* Tài liệu microphone INMP441.
* Tài liệu MOSFET IRLZ44N.
* Tài liệu học phần Mạng cảm biến.

---

## 20. Ghi chú

Repository này được xây dựng phục vụ báo cáo cuối kỳ cá nhân cho môn Mạng cảm biến. Nội dung tập trung vào quy trình thu thập dữ liệu âm thanh, huấn luyện mô hình Audio Keyword Spotting trên Edge Impulse và triển khai mô hình lên ESP32-S3 để điều khiển quạt DC bằng giọng nói.
