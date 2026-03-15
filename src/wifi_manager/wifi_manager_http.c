#include "wifi_manager_http.h"

#include <string.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "portal_html.h"
#include "wifi_manager.h"

static const char *TAG = "WM_HTTP";
static httpd_handle_t s_server = NULL;

/* ── GET / — Phục vụ trang HTML captive portal ── */
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WIFI_PORTAL_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* ── GET /scan — Quét và trả danh sách Wi-Fi dạng JSON ── */
static esp_err_t scan_get_handler(httpd_req_t *req) {
    /* Quét Wi-Fi (blocking) */
    wifi_scan_config_t scan_cfg = {
        .show_hidden = false,
    };
    esp_wifi_scan_start(&scan_cfg, true);

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count > 20)
        ap_count = 20; /* giới hạn */

    wifi_ap_record_t *ap_records = calloc(ap_count, sizeof(wifi_ap_record_t));
    esp_wifi_scan_get_ap_records(&ap_count, ap_records);

    /* Tạo JSON */
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < ap_count; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "ssid", (const char *)ap_records[i].ssid);
        cJSON_AddNumberToObject(obj, "rssi", ap_records[i].rssi);
        cJSON_AddNumberToObject(obj, "auth", ap_records[i].authmode);
        cJSON_AddItemToArray(arr, obj);
    }
    free(ap_records);

    char *json_str = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    free(json_str);
    return ESP_OK;
}

/* ── Hàm helper: URL-decode ── */
static void url_decode(char *dst, const char *src, size_t dst_size) {
    size_t di = 0;
    for (size_t si = 0; src[si] && di < dst_size - 1; si++) {
        if (src[si] == '%' && src[si + 1] && src[si + 2]) {
            char hex[3] = {src[si + 1], src[si + 2], 0};
            dst[di++] = (char)strtol(hex, NULL, 16);
            si += 2;
        } else if (src[si] == '+') {
            dst[di++] = ' ';
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

/* ── Hàm helper: lấy giá trị param từ body URL-encoded ── */
static bool get_form_param(const char *body, const char *key, char *out, size_t out_size) {
    size_t key_len = strlen(key);
    const char *p = body;
    while ((p = strstr(p, key)) != NULL) {
        /* Đảm bảo đây là đầu param (đầu chuỗi hoặc sau '&') */
        if (p != body && *(p - 1) != '&') {
            p += key_len;
            continue;
        }
        if (p[key_len] != '=') {
            p += key_len;
            continue;
        }
        const char *val_start = p + key_len + 1;
        const char *val_end = strchr(val_start, '&');
        size_t val_len = val_end ? (size_t)(val_end - val_start) : strlen(val_start);

        char *raw = calloc(val_len + 1, 1);
        memcpy(raw, val_start, val_len);
        url_decode(out, raw, out_size);
        free(raw);
        return true;
    }
    return false;
}

/* ── POST /connect — Nhận SSID & password, thực hiện kết nối ── */
static esp_err_t connect_post_handler(httpd_req_t *req) {
    char body[256] = {0};
    int received = httpd_req_recv(req, body, sizeof(body) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }
    body[received] = '\0';
    ESP_LOGI(TAG, "Body: %s", body);

    char ssid[33] = {0};
    char password[65] = {0};

    if (!get_form_param(body, "ssid", ssid, sizeof(ssid))) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Thieu SSID\"}");
        return ESP_OK;
    }
    get_form_param(body, "password", password, sizeof(password));

    ESP_LOGI(TAG, "Yêu cầu kết nối -> SSID='%s'", ssid);

    /* Lưu vào NVS và bắt đầu kết nối STA */
    esp_err_t ret = wifi_manager_connect(ssid, password);

    cJSON *resp = cJSON_CreateObject();
    if (ret == ESP_OK) {
        /* Lấy IP */
        esp_netif_ip_info_t ip_info;
        esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (sta && esp_netif_get_ip_info(sta, &ip_info) == ESP_OK) {
            char ip_str[16];
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
            cJSON_AddStringToObject(resp, "status", "ok");
            cJSON_AddStringToObject(resp, "ip", ip_str);
        } else {
            cJSON_AddStringToObject(resp, "status", "ok");
            cJSON_AddStringToObject(resp, "ip", "N/A");
        }
    } else {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Khong the ket noi. Kiem tra lai SSID/password.");
    }

    char *json_str = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    free(json_str);
    return ESP_OK;
}

/* ── Khởi động HTTP server ── */
esp_err_t wifi_manager_http_start(void) {
    if (s_server) {
        ESP_LOGW(TAG, "HTTP server đã chạy");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Không thể khởi động HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Đăng ký các URI handler */
    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
    };
    const httpd_uri_t scan = {
        .uri = "/scan",
        .method = HTTP_GET,
        .handler = scan_get_handler,
    };
    const httpd_uri_t connect_uri = {
        .uri = "/connect",
        .method = HTTP_POST,
        .handler = connect_post_handler,
    };

    httpd_register_uri_handler(s_server, &root);
    httpd_register_uri_handler(s_server, &scan);
    httpd_register_uri_handler(s_server, &connect_uri);

    ESP_LOGI(TAG, "HTTP server khởi động tại http://192.168.4.1/");
    return ESP_OK;
}

/* ── Dừng HTTP server ── */
void wifi_manager_http_stop(void) {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "HTTP server đã dừng");
    }
}
