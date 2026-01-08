#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "cloud_client.h"
#include "utils.h"
#include "time.h"

void cloud_client_init() {
    curl_global_init(CURL_GLOBAL_ALL);
}

void cloud_client_cleanup() {
    curl_global_cleanup();
}

void upload_alert_task(void *arg) {
    AlertTaskArgs *data = (AlertTaskArgs *)arg;
    CURL *curl = curl_easy_init();
    
    if (curl) {
        // 由于node_id是uint32_t，将其转换成字符串避免出错
        char id_hex[16];
        sprintf(id_hex, "0x%X", data->node_id);
        // 转换时间戳为年月日 时分秒格式
        time_t now = time(NULL);
        struct tm tm_info;
        char time_buf[26]; // 足够容纳 "2023-10-27 10:00:00"
        // 使用线程安全版本的 localtime_r (在线程池环境中非常重要)
        localtime_r(&now, &tm_info);
        // 格式化为：年-月-日 时:分:秒
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);
        // 1. 构造 JSON
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "node_id", id_hex);
        cJSON_AddNumberToObject(root, "ldr", data->ldr_value);
        cJSON_AddNumberToObject(root, "severity", data->severity);
        cJSON_AddStringToObject(root, "time", time_buf);
        char *json_str = cJSON_PrintUnformatted(root);

        // 2. 配置 Curl
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, FIREBASE_URL);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // 5秒超时

        // 3. 发送请求
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            log_message(LOG_ERR, "[Cloud] Upload failed: %s", curl_easy_strerror(res));
        } else {
            log_message(LOG_ALERT, "[Cloud] Alert from 0x%X pushed to Firebase.", data->node_id);
        }

        // 4. 清理
        cJSON_Delete(root);
        free(json_str);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    // 释放由调用者分配的参数内存
    free(data);
}