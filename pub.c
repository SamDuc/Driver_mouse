#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <mosquitto.h>
#include <string.h>

#define MQTT_HOST "5.196.78.28"
#define MQTT_PORT 1883
#define MQTT_TOPIC_CLICKS "DUCANH/HOME/CLICK_COUNT"
#define MQTT_TOPIC_POSITION "DUCANH/HOME/MOUSE_POSITION"
#define MOUSE_DEVICE "/dev/input/mice"

// Function to publish messages to a specific topic
void publish_message(struct mosquitto *mosq, const char *topic, const char *message) {
    int ret = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 0, false);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error publishing to topic %s: %s\n", topic, mosquitto_strerror(ret));
    }
}

int main() {
    int fd;
    unsigned char data[3];
    struct mosquitto *mosq;
    int left_clicks = 0;
    int x = 0, y = 0;

    // Open the mouse device
    fd = open(MOUSE_DEVICE, O_RDONLY);
    if (fd == -1) {
        perror("Opening mouse device");
        return EXIT_FAILURE;
    }

    // Initialize the Mosquitto library
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "Error creating Mosquitto client\n");
        return EXIT_FAILURE;
    }

    // Connect to the MQTT broker
    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Unable to connect to MQTT broker\n");
        return EXIT_FAILURE;
    }

    // Start the Mosquitto network loop
    mosquitto_loop_start(mosq);

    printf("Publishing mouse data. Press Ctrl+C to exit.\n");

    while (read(fd, data, sizeof(data)) > 0) {
        char position_message[64];
        char click_message[64];

        // Extract mouse data
        char left = data[0] & 0x1;
        char dx = data[1];
        char dy = data[2];

        x += dx;
        y -= dy;

        if (left) {
            left_clicks++;
            snprintf(click_message, sizeof(click_message), "left_clicks=%d", left_clicks);
            publish_message(mosq, MQTT_TOPIC_CLICKS, click_message);
        }

        snprintf(position_message, sizeof(position_message), "x=%d,y=%d", x, y);
        publish_message(mosq, MQTT_TOPIC_POSITION, position_message);

        usleep(10000);
    }

    close(fd);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return EXIT_SUCCESS;
}

