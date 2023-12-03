#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include<math.h>

#define TARGET_DEVICE "XX:XX:XX:XX:XX:XX" // Replace with your BLE beacon MAC address

void parse_accelerometer_packet(unsigned char *data) {
    // Parse accelerometer data based on the provided frame definition
    // Extract X, Y, Z axis values, battery level, and MAC address
    short x_axis = data[13] | (data[14] << 8);
    short y_axis = data[15] | (data[16] << 8);
    short z_axis = data[17] | (data[18] << 8);
    unsigned char battery_level = data[12];
    char mac_address[18];
    sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", data[24], data[23], data[22], data[21], data[20], data[19]);

    // Add your logic to determine if the tag is moving or stationary
    // For example, check if the acceleration values exceed a certain threshold
    double acceleration_threshold = 1.0; // Adjust this threshold as needed

    if (fabs(x_axis) > acceleration_threshold || fabs(y_axis) > acceleration_threshold || fabs(z_axis) > acceleration_threshold) {
        printf("Moving - X: %d, Y: %d, Z: %d, Battery: %d%%, MAC: %s\n", x_axis, y_axis, z_axis, battery_level, mac_address);
    } else {
        printf("Stationary - X: %d, Y: %d, Z: %d, Battery: %d%%, MAC: %s\n", x_axis, y_axis, z_axis, battery_level, mac_address);
    }
}

int main() {
    inquiry_info *devices = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev(dev_id);
    if (dev_id < 0 || sock < 0) {
        perror("Error opening socket");
        exit(1);
    }

    len = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    devices = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &devices, flags);
    if (num_rsp < 0) {
        perror("Error during inquiry");
        exit(1);
    }

    printf("Found BLE Devices:\n");
    for (i = 0; i < num_rsp; i++) {
        char addr[19] = { 0 };
        ba2str(&(devices+i)->bdaddr, addr);
        printf("%s\n", addr);
    }

    close(sock);

    struct sockaddr_rc addr = { 0 };
    int s, status;

    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t)1;
    str2ba(TARGET_DEVICE, &addr.rc_bdaddr);

    status = connect(s, (struct sockaddr*)&addr, sizeof(addr));
    if (status < 0) {
        perror("Error connecting to device");
        exit(1);
    }

    unsigned char data[1024];
    while (1) {
        // Read accelerometer data
        ssize_t bytes = read(s, data, sizeof(data));
        if (bytes >= 30 && data[0] == 0x02 && data[2] == 0x06 && data[3] == 26) {
            parse_accelerometer_packet(data);
        }
    }

    close(s);
    free(devices);

    return 0;
}
