#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <mysql/mysql.h>
#include <string.h>

#define MQTT_HOST "5.196.78.28"
#define MQTT_PORT 1883
#define MQTT_TOPIC_CLICKS "DUCANH/HOME/CLICK_COUNT"
#define MQTT_TOPIC_POSITION "DUCANH/HOME/MOUSE_POSITION"

#define HOST "localhost"
#define USER "ducanh"
#define PASS "123456"
#define DB "checksystem"

MYSQL *conn;

void connect_db() {
    conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "Lỗi khởi tạo MySQL!\n");
        exit(1);
    }
    if (!mysql_real_connect(conn, HOST, USER, PASS, DB, 0, NULL, 0)) {
        fprintf(stderr, "Lỗi kết nối MySQL: %s\n", mysql_error(conn));
        exit(1);
    }
    printf("Kết nối MySQL thành công!\n");
}

void insert_click_data(int left_clicks) {
    char query[128];
    snprintf(query, sizeof(query), "INSERT INTO mouse_clicks (left_clicks) VALUES (%d)", left_clicks);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Lỗi ghi dữ liệu click: %s\n", mysql_error(conn));
    } else {
        printf("Inserted left_clicks=%d\n", left_clicks);
    }
}

void insert_mouse_position(int x, int y) {
    char query[128];
    snprintf(query, sizeof(query), "INSERT INTO mouse_position (x, y) VALUES (%d, %d)", x, y);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Lỗi ghi dữ liệu vị trí: %s\n", mysql_error(conn));
    } else {
        printf("Inserted mouse position: X=%d, Y=%d\n", x, y);
    }
}

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    if (strcmp(msg->topic, MQTT_TOPIC_CLICKS) == 0) {
        int left_clicks;
        if (sscanf(msg->payload, "left_clicks=%d", &left_clicks) == 1) {
            insert_click_data(left_clicks);
        } else {
            printf("Dữ liệu click không hợp lệ!\n");
        }
    } else if (strcmp(msg->topic, MQTT_TOPIC_POSITION) == 0) {
        int x, y;
        if (sscanf(msg->payload, "x=%d,y=%d", &x, &y) == 2) {
            insert_mouse_position(x, y);
        } else {
            printf("Dữ liệu vị trí chuột không hợp lệ!\n");
        }
    }
}

int main() {
    struct mosquitto *mosq;

    connect_db();

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "Lỗi tạo Mosquitto client\n");
        return 1;
    }

    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Không thể kết nối MQTT Broker\n");
        return 1;
    }

    mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_CLICKS, 0);
    mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_POSITION, 0);

    printf("Đang lắng nghe dữ liệu từ MQTT broker...\n");
    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    mysql_close(conn);

    return 0;
}

