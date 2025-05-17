#include "device_service.h"

static const char *TAG = "DEVICE_SERVICE";

cJSON *get_device_info(void)
{
    cJSON *data = cJSON_CreateObject();
    if (data == NULL) {
        goto get_device_info_end;
    }

    cJSON_AddStringToObject(data, "compile_time", __TIMESTAMP__);
    cJSON_AddStringToObject(data, "git_commit_id", "GIT_COMMIT_SHA1");
    cJSON_AddStringToObject(data, "firmware_version", SW_VERSION);
    cJSON_AddNumberToObject(data, "package_version", efuse_ll_get_chip_ver_pkg());
    cJSON_AddNumberToObject(data, "chip_version", efuse_hal_chip_revision());
    uint32_t cpu_freq_value;
    ESP_ERROR_CHECK(esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &cpu_freq_value));
    cJSON_AddNumberToObject(data, "cpu_freq", cpu_freq_value);
    char idf_version_str[16];
    sprintf(idf_version_str, "%d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    cJSON_AddStringToObject(data, "idf_version", idf_version_str);

get_device_info_end:
    return data;
}

cJSON * get_task_state(void)
{
    cJSON *data      = cJSON_CreateObject();
    cJSON *task_list = cJSON_CreateArray();
    cJSON *task      = NULL;
    if (data == NULL || task_list == NULL) {
        goto get_state_info_end;
    }

    uint8_t task_count              = uxTaskGetNumberOfTasks(); // 获取当前任务数量
    TaskStatus_t *task_status_array = malloc(sizeof(TaskStatus_t) * task_count);

    if (task_status_array != NULL) {
        // 获取所有任务的状态信息
        task_count = uxTaskGetSystemState(task_status_array, task_count, NULL);
        cJSON_AddNumberToObject(data, "task_count", task_count);

        // 打印任务状态信息
        for (uint8_t i = 0; i < task_count; i++) {
            const char *pcTaskName        = task_status_array[i].pcTaskName;
            uint8_t xTaskNumber           = task_status_array[i].xTaskNumber;
            uint8_t eCurrentState         = task_status_array[i].eCurrentState;
            uint16_t usStackHighWaterMark = task_status_array[i].usStackHighWaterMark;

            task = cJSON_CreateObject();
            if (task == NULL) {
                goto get_state_info_end;
            }
            cJSON_AddStringToObject(task, "pcTaskName", pcTaskName);
            cJSON_AddNumberToObject(task, "xTaskNumber", xTaskNumber);
            cJSON_AddNumberToObject(task, "eCurrentState", eCurrentState);
            cJSON_AddNumberToObject(task, "usStackHighWaterMark", usStackHighWaterMark);
            cJSON_AddItemToArray(task_list, task);

            // ESP_LOGI(TAG, "Task %s (ID: %u) is in state %u and water mark %uB.", pcTaskName, xTaskNumber, eCurrentState, usStackHighWaterMark);
        }

        free(task_status_array); // 释放分配的内存
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory for task state information.");
    }

    cJSON_AddItemToObject(data, "task_list", task_list);

    // 定义一个 heap_caps_info_t 结构体来存储内存信息
    multi_heap_info_t heap_info;
    // 获取堆内存的信息
    heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);

    uint32_t total_free_bytes      = heap_info.total_free_bytes;
    uint32_t total_allocated_bytes = heap_info.total_allocated_bytes;
    uint32_t largest_free_block    = heap_info.largest_free_block;
    uint32_t minimum_free_bytes    = heap_info.minimum_free_bytes;

    cJSON_AddNumberToObject(data, "total_free_bytes", total_free_bytes);
    cJSON_AddNumberToObject(data, "total_allocated_bytes", total_allocated_bytes);
    cJSON_AddNumberToObject(data, "largest_free_block", largest_free_block);
    cJSON_AddNumberToObject(data, "minimum_free_bytes", minimum_free_bytes);

    // ESP_LOGI(TAG, "Heap Info: Total free: %lu, Total allocated: %lu, Largest free block: %lu, Minimum free: %lu", total_free_bytes, total_allocated_bytes, largest_free_block, minimum_free_bytes);
    // ESP_LOGI(TAG, "data size: %d", data_index);

get_state_info_end:
    return data;
}
