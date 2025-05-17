#include "http_service.h"

static const char *TAG = "HTTP_SERVICE";

httpd_handle_t server = NULL;
int current_ws_fd     = -1;
extern MessageBufferHandle_t xMessageBufferReqRecv;
extern user_config_t user_config;

static esp_err_t get_whoami_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    const char *response = "0721esp32";
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {

        char query[256];
        char token[128];
        char expected_token[128];

        if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
            if (httpd_query_key_value(query, "token", token, sizeof(token)) == ESP_OK) {
                ESP_LOGI(TAG, "Token received: %s", token);
                snprintf(expected_token, sizeof(expected_token), "%s:%s", user_config.username, user_config.password);
                if (strcmp(token, expected_token) != 0) {
                    ESP_LOGW(TAG, "Unauthorized access: %s", token);
                    httpd_resp_set_status(req, "401 Unauthorized");
                    httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
                    return ESP_FAIL;
                }
            } else {
                ESP_LOGW(TAG, "Authorization token missing");
                httpd_resp_set_status(req, "401 Unauthorized");
                httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
                return ESP_FAIL;
            }
        } else {
            ESP_LOGW(TAG, "Authorization token missing");
            httpd_resp_set_status(req, "401 Unauthorized");
            httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }

        if (httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)) == HTTPD_WS_CLIENT_WEBSOCKET) {
            ESP_LOGI(TAG, "Client connected");
        }

        return ESP_OK;
    }

    current_ws_fd = httpd_req_to_sockfd(req);

    uint8_t *ws_recv_buf = NULL;
    ws_recv_buf          = malloc(user_config.ws_recv_buf_size);

    // 处理 WebSocket 数据帧
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = ws_recv_buf;
    httpd_ws_recv_frame(req, &ws_pkt, user_config.ws_recv_buf_size);
    ws_pkt.payload[ws_pkt.len] = 0;
    ESP_LOGI(TAG, "Receive ws size: %d, data: %s", ws_pkt.len, ws_pkt.payload);

    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        current_ws_fd = -1;
        ESP_LOGI(TAG, "Ws client disconnected");
    } else {
        xMessageBufferSend(xMessageBufferReqRecv, ws_pkt.payload, ws_pkt.len, portMAX_DELAY);
    }

    free(ws_recv_buf);
    return ESP_OK;
}

// HTTP 服务器启动
httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t whoami_uri = {
        .uri     = "/whoami",
        .method  = HTTP_GET,
        .handler = get_whoami_handler,
    };
    httpd_register_uri_handler(server, &whoami_uri);

    httpd_uri_t ws_uri = {
        .uri          = "/esp-ws",
        .method       = HTTP_GET,
        .handler      = websocket_handler,
        .is_websocket = true};
    httpd_register_uri_handler(server, &ws_uri);

    ESP_LOGI(TAG, "start_webserver");

    return server;
}
