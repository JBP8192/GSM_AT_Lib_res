/*
 * MQTT client API example with GSM device.
 *
 * Once device is connected to network,
 * it will try to connect to mosquitto test server and start the MQTT.
 *
 * If successfully connected, it will publish data to "gsm_mqtt_topic" topic every x seconds.
 *
 * To check if data are sent, you can use mqtt-spy PC software to inspect
 * test.mosquitto.org server and subscribe to publishing topic
 */

#include "gsm/apps/gsm_mqtt_client_api.h"
#include "mqtt_client_api.h"
#include "gsm/gsm_mem.h"

/**
 * \brief           Connection information for MQTT CONNECT packet
 */
static const gsm_mqtt_client_info_t
mqtt_client_info = {    
    .keep_alive = 10,

    /* Server login data */
    .user = "8a215f70-a644-11e8-ac49-e932ed599553",
    .pass = "26aa943f702e5e780f015cd048a91e8fb54cca28",

    /* Device identifier address */
    .id = "2c3573a0-0176-11e9-a056-c5cffe7f75f9",
};

/**
 * \brief           Memory for temporary topic
 */
static char
mqtt_topic_str[256];

/**
 * \brief           MQTT client API thread
 */
void
mqtt_client_api_thread(void const* arg) {
    gsm_mqtt_client_api_p client;
    gsm_mqtt_conn_status_t conn_status;
    gsm_mqtt_client_api_buf_p buf;
    gsmr_t res;

    /* Create new MQTT API */
    client = gsm_mqtt_client_api_new(256, 128);
    if (client == NULL) {
        goto terminate;
    }

    while (1) {
        /* Make a connection */
        printf("Joining MQTT server\r\n");

        /* Try to join */
        conn_status = gsm_mqtt_client_api_connect(client, "mqtt.mydevices.com", 1883, &mqtt_client_info);
        if (conn_status == GSM_MQTT_CONN_STATUS_ACCEPTED) {
            printf("Connected and accepted!\r\n");
            printf("Client is ready to subscribe and publish to new messages\r\n");
        } else {
            printf("Connect API response: %d\r\n", (int)conn_status);
            gsm_delay(5000);
            continue;
        }

        /* Subscribe to topics */
        sprintf(mqtt_topic_str, "v1/%s/things/%s/cmd/#", mqtt_client_info.user, mqtt_client_info.id);
        if (gsm_mqtt_client_api_subscribe(client, mqtt_topic_str, GSM_MQTT_QOS_AT_LEAST_ONCE) == gsmOK) {
            printf("Subscribed to topic\r\n");
        } else {
            printf("Problem subscribing to topic!\r\n");
        }

        while (1) {
            /* Receive MQTT packet with 1000ms timeout */
            res = gsm_mqtt_client_api_receive(client, &buf, 5000);
            if (res == gsmOK) {
                if (buf != NULL) {
                    printf("Publish received!\r\n");
                    printf("Topic: %s, payload: %s\r\n", buf->topic, buf->payload);
                    gsm_mqtt_client_api_buf_free(buf);
                    buf = NULL;
                }
            } else if (res == gsmCLOSED) {
                printf("MQTT connection closed!\r\n");
                break;
            } else if (res == gsmTIMEOUT) {
                printf("Timeout on MQTT receive function. Manually publishing.\r\n");

                /* Publish data on channel 1 */
                sprintf(mqtt_topic_str, "v1/%s/things/%s/data/1", mqtt_client_info.user, mqtt_client_info.id);
                gsm_mqtt_client_api_publish(client, mqtt_topic_str, "1", 1, GSM_MQTT_QOS_AT_LEAST_ONCE, 0);
            }
        }
    }

terminate: 
    gsm_mqtt_client_api_delete(client);
    printf("MQTT client thread terminate\r\n");
    gsm_sys_thread_terminate(NULL);
}
