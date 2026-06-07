#define EIDSP_QUANTIZE_FILTERBANK 0

#include <Dieu_Khien_Quat_Bang_Giong_Noi_inferencing.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"

// ================== CẤU HÌNH CHÂN KẾT NỐI ==================
// Đổi các chân này theo đúng mạch thực tế nếu bạn đấu khác.

#define I2S_BCLK_PIN      26      // INMP441 SCK/BCLK
#define I2S_WS_PIN        32      // INMP441 WS/LRCL
#define I2S_DIN_PIN       33      // INMP441 SD/DOUT

#define FAN_PWM_PIN       25      // GPIO xuất PWM đến Gate MOSFET
#define FAN_PWM_CHANNEL   0
#define FAN_PWM_FREQ      5000
#define FAN_PWM_RES       8       // 8 bit: giá trị PWM từ 0 đến 255

// ================== CẤU HÌNH NHẬN DIỆN ==================

#define COMMAND_THRESHOLD 0.80    // Ngưỡng nhận lệnh
#define WAKE_TIMEOUT_MS   8000    // Sau khi nói "hữu đang", hệ thống chờ lệnh trong 8 giây
#define COMMAND_DELAY_MS  1500    // Chống nhận lặp lại cùng một lệnh quá nhanh

// ================== BIẾN ĐIỀU KHIỂN QUẠT ==================

bool wakeActive = false;
unsigned long wakeStartTime = 0;
unsigned long lastCommandTime = 0;

int fanLevel = 0;
int fanPWM[4] = {0, 85, 170, 255};   // 0%, 33%, 67%, 100%

// ================== BIẾN XỬ LÝ ÂM THANH EDGE IMPULSE ==================

typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];

static bool debug_nn = false;
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
static bool record_status = true;

// ================== HÀM ĐIỀU KHIỂN PWM ==================

void setupFanPWM() {
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RES);
    ledcAttachPin(FAN_PWM_PIN, FAN_PWM_CHANNEL);
    ledcWrite(FAN_PWM_CHANNEL, 0);
}

void setFanLevel(int level) {
    if (level < 0) level = 0;
    if (level > 3) level = 3;

    fanLevel = level;
    ledcWrite(FAN_PWM_CHANNEL, fanPWM[fanLevel]);

    Serial.print("Muc quat hien tai: ");
    Serial.print(fanLevel);
    Serial.print(" | PWM = ");
    Serial.println(fanPWM[fanLevel]);
}

// ================== XỬ LÝ LỆNH GIỌNG NÓI ==================

void handleVoiceCommand(const char *label, float score) {
    unsigned long now = millis();

    if (score < COMMAND_THRESHOLD) {
        return;
    }

    if (strcmp(label, "noise") == 0) {
        return;
    }

    if (now - lastCommandTime < COMMAND_DELAY_MS) {
        return;
    }

    Serial.print("Lenh nhan dien: ");
    Serial.print(label);
    Serial.print(" | Score: ");
    Serial.println(score, 3);

    // Lệnh kích hoạt hệ thống
    if (strcmp(label, "huu dang") == 0) {
        wakeActive = true;
        wakeStartTime = now;
        lastCommandTime = now;

        Serial.println("He thong da duoc kich hoat. Hay noi lenh dieu khien quat.");
        return;
    }

    // Nếu hết thời gian chờ thì tắt trạng thái kích hoạt
    if (wakeActive && (now - wakeStartTime > WAKE_TIMEOUT_MS)) {
        wakeActive = false;
        Serial.println("Het thoi gian cho lenh. Can noi lai 'huu dang'.");
    }

    // Chưa kích hoạt thì bỏ qua lệnh điều khiển
    if (!wakeActive) {
        Serial.println("Chua kich hoat he thong. Hay noi 'huu dang' truoc.");
        lastCommandTime = now;
        return;
    }

    // Lệnh tăng tốc quạt
    if (strcmp(label, "quat len") == 0) {
        setFanLevel(fanLevel + 1);
        Serial.println("Da tang toc do quat.");
        wakeStartTime = now;
        lastCommandTime = now;
        return;
    }

    // Lệnh giảm tốc quạt
    if (strcmp(label, "quat xuong") == 0) {
        setFanLevel(fanLevel - 1);
        Serial.println("Da giam toc do quat.");
        wakeStartTime = now;
        lastCommandTime = now;
        return;
    }

    // Lệnh tắt quạt
    if (strcmp(label, "tat quat") == 0) {
        setFanLevel(0);
        wakeActive = false;
        Serial.println("Da tat quat.");
        lastCommandTime = now;
        return;
    }
}

// ================== SETUP ==================

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("He thong dieu khien quat bang giong noi");
    Serial.println("Dang khoi dong...");

    setupFanPWM();

    ei_printf("Thong so mo hinh Edge Impulse:\n");
    ei_printf("\tTan so lay mau: %d Hz\n", EI_CLASSIFIER_FREQUENCY);
    ei_printf("\tSo mau dau vao: %d\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    ei_printf("\tSo label: %d\n", EI_CLASSIFIER_LABEL_COUNT);

    run_classifier_init();

    ei_printf("Bat dau nhan dien lien tuc sau 2 giay...\n");
    ei_sleep(2000);

    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Khong cap phat duoc bo nho cho audio buffer.\n");
        return;
    }

    ei_printf("Dang ghi am va nhan dien...\n");
}

// ================== LOOP ==================

void loop() {
    bool m = microphone_inference_record();

    if (!m) {
        ei_printf("ERR: Loi ghi am thanh.\n");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;

    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);

    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Khong chay duoc classifier (%d)\n", r);
        return;
    }

    if (++print_results >= EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW) {
        const char *bestLabel = "";
        float bestScore = 0.0;

        Serial.println("========== Ket qua nhan dien ==========");

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            const char *label = result.classification[ix].label;
            float score = result.classification[ix].value;

            Serial.print(label);
            Serial.print(": ");
            Serial.println(score, 3);

            if (score > bestScore) {
                bestScore = score;
                bestLabel = label;
            }
        }

        Serial.print("Label cao nhat: ");
        Serial.print(bestLabel);
        Serial.print(" | Score: ");
        Serial.println(bestScore, 3);

        handleVoiceCommand(bestLabel, bestScore);

        Serial.println("=======================================");
        print_results = 0;
    }
}

// ================== CALLBACK ÂM THANH ==================

static void audio_inference_callback(uint32_t n_bytes) {
    for (int i = 0; i < n_bytes >> 1; i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

        if (inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void *arg) {
    const int32_t i2s_bytes_to_read = (uint32_t)arg;
    size_t bytes_read = i2s_bytes_to_read;

    while (record_status) {
        i2s_read((i2s_port_t)1, (void *)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

        if (bytes_read <= 0) {
            ei_printf("Loi doc I2S: %d\n", bytes_read);
        } else {
            if (bytes_read < i2s_bytes_to_read) {
                ei_printf("Doc I2S khong du du lieu.\n");
            }

            // Khuếch đại tín hiệu âm thanh
            for (int x = 0; x < i2s_bytes_to_read / 2; x++) {
                sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
            }

            if (record_status) {
                audio_inference_callback(i2s_bytes_to_read);
            } else {
                break;
            }
        }
    }

    vTaskDelete(NULL);
}

// ================== KHỞI TẠO MICROPHONE ==================

static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[1] == NULL) {
        ei_free(inference.buffers[0]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    if (i2s_init(EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Khong khoi dong duoc I2S.\n");
    }

    ei_sleep(100);

    record_status = true;

    xTaskCreate(
        capture_samples,
        "CaptureSamples",
        1024 * 32,
        (void *)sample_buffer_size,
        10,
        NULL
    );

    return true;
}

// ================== GHI ÂM ==================

static bool microphone_inference_record(void) {
    bool ret = true;

    if (inference.buf_ready == 1) {
        ei_printf("Loi: sample buffer bi tran.\n");
        ret = false;
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;
    return ret;
}

// ================== LẤY DỮ LIỆU ÂM THANH CHO EDGE IMPULSE ==================

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(
        &inference.buffers[inference.buf_select ^ 1][offset],
        out_ptr,
        length
    );

    return 0;
}

// ================== GIẢI PHÓNG BỘ NHỚ ==================

static void microphone_inference_end(void) {
    i2s_deinit();
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
}

// ================== CẤU HÌNH I2S CHO INMP441 ==================

static int i2s_init(uint32_t sampling_rate) {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
        .sample_rate = sampling_rate,
        .bits_per_sample = (i2s_bits_per_sample_t)16,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = -1,
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = -1,
        .data_in_num = I2S_DIN_PIN,
    };

    esp_err_t ret = ESP_OK;

    ret = i2s_driver_install((i2s_port_t)1, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ei_printf("Loi i2s_driver_install.\n");
    }

    ret = i2s_set_pin((i2s_port_t)1, &pin_config);
    if (ret != ESP_OK) {
        ei_printf("Loi i2s_set_pin.\n");
    }

    ret = i2s_zero_dma_buffer((i2s_port_t)1);
    if (ret != ESP_OK) {
        ei_printf("Loi i2s_zero_dma_buffer.\n");
    }

    return int(ret);
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)1);
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Model Edge Impulse hien tai khong phai model microphone."
#endif