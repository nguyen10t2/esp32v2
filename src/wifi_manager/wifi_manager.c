#include "wifi_manager.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "wifi_manager_http.h"

/* ── Hằng số ── */
#define WM_TAG "WM"
#define WM_AP_SSID "ESP32"
#define WM_AP_PASS "" /* Mở, không mật khẩu */
#define WM_AP_MAX_CONN 4
#define WM_AP_CHANNEL 1
#define WM_NVS_NAMESPACE "wifi_mgr"
#define WM_NVS_KEY_SSID "ssid"
#define WM_NVS_KEY_PASS "pass"
#define WM_CONNECT_TIMEOUT_MS 15000 /* 15 giây chờ kết nối */

/* ── Event bits ── */
#define WM_CONNECTED_BIT BIT0
#define WM_FAIL_BIT BIT1

/* ── Biến nội bộ ── */
static EventGroupHandle_t s_wifi_event_group = NULL;
static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;
static bool s_connected = false;
static int s_retry_count = 0;
#define WM_MAX_RETRY 3

/* ══════════════════════════════════════════════════════
 *  NVS helpers – lưu / đọc / xóa SSID + password
 * ══════════════════════════════════════════════════════ */
static esp_err_t nvs_save_credentials(const char *ssid, const char *password) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(WM_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK)
        return err;

    nvs_set_str(h, WM_NVS_KEY_SSID, ssid);
    nvs_set_str(h, WM_NVS_KEY_PASS, password);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(WM_TAG, "Đã lưu thông tin Wi-Fi vào NVS");
    return ESP_OK;
}

// Trả về true nếu đã có SSID + password trong NVS, false nếu không có hoặc lỗi
static bool nvs_load_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len) {
    nvs_handle_t h;

    if (nvs_open(WM_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK)
        return false;

    size_t ssid_size = ssid_len;
    size_t pass_size = pass_len;

    esp_err_t e1 = nvs_get_str(h, WM_NVS_KEY_SSID, ssid, &ssid_size);
    esp_err_t e2 = nvs_get_str(h, WM_NVS_KEY_PASS, pass, &pass_size);

    nvs_close(h);

    return (e1 == ESP_OK && e2 == ESP_OK && strlen(ssid) > 0);
}

// Xóa thông tin Wi-Fi đã lưu trong NVS
static void nvs_clear_credentials(void) {
    nvs_handle_t h;
    if (nvs_open(WM_NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK)
        return;
    nvs_erase_all(h);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(WM_TAG, "Đã xóa thông tin Wi-Fi trong NVS");
}

/* ══════════════════════════════════════════════════════
 *  Event handler
 * ══════════════════════════════════════════════════════ */
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t event_id,
                               void *event_data) {
    if (base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(WM_TAG, "STA bắt đầu, đang kết nối...");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                s_connected = false;
                if (s_retry_count < WM_MAX_RETRY) {
                    s_retry_count++;
                    ESP_LOGW(WM_TAG, "Mất kết nối, thử lại (%d/%d)...", s_retry_count,
                             WM_MAX_RETRY);
                    esp_wifi_connect();
                } else {
                    ESP_LOGE(WM_TAG, "Không thể kết nối sau %d lần thử", WM_MAX_RETRY);
                    if (s_wifi_event_group) {
                        xEventGroupSetBits(s_wifi_event_group, WM_FAIL_BIT);
                    }
                }
                break;

            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t *ev = (wifi_event_ap_staconnected_t *)event_data;
                ESP_LOGI(WM_TAG, "Thiết bị kết nối vào AP, MAC=" MACSTR, MAC2STR(ev->mac));
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t *ev = (wifi_event_ap_stadisconnected_t *)event_data;
                ESP_LOGI(WM_TAG, "Thiết bị rời AP, MAC=" MACSTR, MAC2STR(ev->mac));
                break;
            }
            default:
                break;
        }
    } else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WM_TAG, "Đã nhận IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        s_connected = true;
        s_retry_count = 0;
        if (s_wifi_event_group) {
            xEventGroupSetBits(s_wifi_event_group, WM_CONNECTED_BIT);
        }
    }
}

/* ══════════════════════════════════════════════════════
 *  Khởi tạo hạ tầng Wi-Fi (gọi 1 lần)
 * ══════════════════════════════════════════════════════ */
static void wifi_init_infra(void) {
    esp_netif_init();
    esp_event_loop_create_default();

    /* Tạo netif cho STA và AP */
    s_sta_netif = esp_netif_create_default_wifi_sta();
    s_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    /* Đăng ký event handler */
    esp_event_handler_instance_t any_wifi, got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
                                        &any_wifi);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
                                        &got_ip);
}

/* ══════════════════════════════════════════════════════
 *  Bật AP mode để phục vụ captive portal
 * ══════════════════════════════════════════════════════ */
static void start_ap_mode(void) {
    /* Cấu hình AP + STA (APSTA) để có thể vừa phục vụ portal vừa quét mạng */
    esp_wifi_set_mode(WIFI_MODE_APSTA);

    wifi_config_t ap_cfg = {
        .ap =
            {
                .ssid = WM_AP_SSID,
                .ssid_len = strlen(WM_AP_SSID),
                .channel = WM_AP_CHANNEL,
                .max_connection = WM_AP_MAX_CONN,
                .authmode = WIFI_AUTH_OPEN,
            },
    };

    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();

    ESP_LOGI(WM_TAG, "AP \"%s\" đã bật — Truy cập http://192.168.4.1/", WM_AP_SSID);

    /* Khởi động HTTP server */
    wifi_manager_http_start();
}

/* ══════════════════════════════════════════════════════
 *  Kết nối STA từ SSID + password
 * ══════════════════════════════════════════════════════ */
static esp_err_t do_sta_connect(const char *ssid, const char *password) {
    s_retry_count = 0;
    s_connected = false;

    if (!s_wifi_event_group) {
        s_wifi_event_group = xEventGroupCreate();
    }
    xEventGroupClearBits(s_wifi_event_group, WM_CONNECTED_BIT | WM_FAIL_BIT);

    wifi_config_t sta_cfg = {0};
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
    strncpy((char *)sta_cfg.sta.password, password, sizeof(sta_cfg.sta.password) - 1);
    sta_cfg.sta.threshold.authmode = strlen(password) > 0 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    /* Nếu đang ở APSTA thì giữ nguyên, nếu chưa start thì set STA */
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_NULL || mode == WIFI_MODE_STA) {
        esp_wifi_set_mode(WIFI_MODE_STA);
    }
    /* Nếu mode == APSTA thì giữ nguyên để portal vẫn hoạt động */

    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);

    /* Nếu Wi-Fi chưa start (STA thuần) */
    if (mode == WIFI_MODE_NULL) {
        esp_wifi_start();
    } else {
        /* Đã start (APSTA) → chỉ cần disconnect rồi connect lại */
        esp_wifi_disconnect();
        esp_wifi_connect();
    }

    ESP_LOGI(WM_TAG, "Đang kết nối đến '%s'...", ssid);

    /* Chờ kết quả */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WM_CONNECTED_BIT | WM_FAIL_BIT,
                                           pdFALSE, pdFALSE, pdMS_TO_TICKS(WM_CONNECT_TIMEOUT_MS));

    if (bits & WM_CONNECTED_BIT) {
        ESP_LOGI(WM_TAG, "Kết nối thành công đến '%s'", ssid);
        return ESP_OK;
    }

    ESP_LOGE(WM_TAG, "Kết nối thất bại đến '%s'", ssid);
    return ESP_FAIL;
}

// ---------------------------------- API công khai ---------------------------------- */

void wifi_manager_start(void) {
    wifi_init_infra();

    char ssid[33] = {0};
    char pass[65] = {0};

    /* Thử đọc thông tin Wi-Fi từ NVS */
    if (nvs_load_credentials(ssid, sizeof(ssid), pass, sizeof(pass))) {
        ESP_LOGI(WM_TAG, "Tìm thấy thông tin Wi-Fi trong NVS: SSID='%s'", ssid);

        /* Thử kết nối */
        esp_wifi_set_mode(WIFI_MODE_STA);
        if (do_sta_connect(ssid, pass) == ESP_OK) {
            ESP_LOGI(WM_TAG, "Kết nối từ NVS thành công!");
            return; /* Xong — không cần AP */
        }
        ESP_LOGW(WM_TAG, "Kết nối từ NVS thất bại, chuyển sang AP mode");
        esp_wifi_stop();
    } else {
        ESP_LOGI(WM_TAG, "Chưa có thông tin Wi-Fi trong NVS");
    }

    /* Bật AP + captive portal */
    start_ap_mode();
}

// Dừng Wi-Fi, xóa event group, reset trạng thái
static void ap_shutdown_task(void *arg) {
    vTaskDelay(pdMS_TO_TICKS(2000));

    wifi_manager_http_stop();
    esp_wifi_set_mode(WIFI_MODE_STA);

    ESP_LOGI(WM_TAG, "AP đã tắt");

    vTaskDelete(NULL);
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password) {
    if (!ssid || strlen(ssid) == 0)
        return ESP_ERR_INVALID_ARG;
    if (!password)
        password = "";

    /* Lưu vào NVS */
    nvs_save_credentials(ssid, password);

    /* Kết nối */
    esp_err_t ret = do_sta_connect(ssid, password);

    if (ret == ESP_OK) {
        /* Kết nối thành công → tắt HTTP server + AP sau 2s để client nhận response */
        xTaskCreate(ap_shutdown_task, "ap_shutdown", 4096, NULL, 5, NULL);
        ESP_LOGI(WM_TAG, "Đã tắt AP, chuyển sang STA mode");
    }

    return ret;
}

bool wifi_manager_is_connected(void) {
    return s_connected;
}

void wifi_manager_reset(void) {
    nvs_clear_credentials();
    s_connected = false;
}
