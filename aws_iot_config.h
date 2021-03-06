//
// Created by Zhi Yan Liu on 2019-05-20.
//

/**
 * @file aws_iot_config.h
 * @brief AWS IoT specific configuration file
 */

#ifndef AWS_IOT_CONFIG_H_
#define AWS_IOT_CONFIG_H_

// Get from console
//#define AWS_IOT_MQTT_HOST            "a2xa0z3a2e61rw.iot.cn-north-1.amazonaws.com.cn" ///< Customer specific MQTT HOST. The same will be used for Thing Shadow
#define AWS_IOT_MQTT_HOST            "azffej9vrka1f-ats.iot.ap-northeast-1.amazonaws.com" ///< Customer specific MQTT HOST. The same will be used for Thing Shadow
#define AWS_IOT_MQTT_PORT            443 ///< default port for MQTT/S
#define AWS_IOT_MQTT_CLIENT_ID       "nw-app-ota-demo-dev" ///< MQTT client ID should be unique for every device
#define AWS_IOT_MY_THING_NAME        "nw-app-ota-demo-dev" ///< Thing Name of the Shadow this device is associated with
#define AWS_IOT_ROOT_CA_FILENAME     "root-ca.crt" ///< Root CA file name
#define AWS_IOT_CERTIFICATE_FILENAME "cert.pem" ///< device signed certificate file name
#define AWS_IOT_PRIVATE_KEY_FILENAME "private.key" ///< Device private key filename

// MQTT PubSub
#define AWS_IOT_MQTT_RX_BUF_LEN 102400 ///< Any message that comes into the device should be less than this buffer size. If a received message is bigger than this buffer size the message will be dropped.
#define AWS_IOT_MQTT_TX_BUF_LEN 102400 ///< Any time a message is sent out through the MQTT layer. The message is copied into this buffer anytime a publish is done. This will also be used in the case of Thing Shadow
#define AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS 5 ///< Maximum number of topic filters the MQTT client can handle at any given time. This should be increased appropriately when using Thing Shadow

// Shadow and Job common configs
#define MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES 80  ///< Maximum size of the Unique Client Id. For More info on the Client Id refer \ref response "Acknowledgments"
#define MAX_SIZE_CLIENT_ID_WITH_SEQUENCE MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES + 10    ///< This is size of the extra sequence number that will be appended to the Unique client Id
#define MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE MAX_SIZE_CLIENT_ID_WITH_SEQUENCE + 20 ///< This is size of the the total clientToken key and value pair in the JSON
#define MAX_SIZE_OF_THING_NAME 30 ///< The Thing Name should not be bigger than this value. Modify this if the Thing Name needs to be bigger

// Job specific configs
#define MAX_SIZE_OF_JOB_ID 64
#define MAX_JOB_JSON_TOKEN_EXPECTED 120
#define MAX_SIZE_OF_JOB_REQUEST AWS_IOT_MQTT_TX_BUF_LEN
#define MAX_JOB_TOPIC_LENGTH_WITHOUT_JOB_ID_OR_THING_NAME 40
#define MAX_JOB_TOPIC_LENGTH_BYTES MAX_JOB_TOPIC_LENGTH_WITHOUT_JOB_ID_OR_THING_NAME + MAX_SIZE_OF_THING_NAME + MAX_SIZE_OF_JOB_ID + 2

// Auto Reconnect specific config
#define AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL 1000 ///< Minimum time before the First reconnect attempt is made as part of the exponential back-off algorithm
#define AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL 128000 ///< Maximum time interval after which exponential back-off will stop attempting to reconnect.

// Job and thing Shadow specific configs
#define MAX_JSON_TOKEN_EXPECTED 120 ///< These are the max tokens that is expected to be in the Shadow JSON document. Include the metadata that gets published

// Thing Shadow specific configs
#define SHADOW_MAX_SIZE_OF_RX_BUFFER 512 ///< Maximum size of the SHADOW buffer to store the received Shadow message
#define MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME 10 ///< At Any given time we will wait for this many responses. This will correlate to the rate at which the shadow actions are requested
#define MAX_THINGNAME_HANDLED_AT_ANY_GIVEN_TIME 10 ///< We could perform shadow action on any thing Name and this is maximum Thing Names we can act on at any given time
#define MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME 60 ///< All shadow actions have to be published or subscribed to a topic which is of the format $aws/things/{thingName}/shadow/update/accepted. This refers to the size of the topic without the Thing Name
#define MAX_SHADOW_TOPIC_LENGTH_BYTES MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME + MAX_SIZE_OF_THING_NAME ///< This size includes the length of topic with Thing Name

#define DISABLE_METRICS false ///< Disable the collection of metrics by setting this to true
#define MAX_S3_DOWNLOAD_TIME 60 ///< Maximum time in seconds that you allow the job package transfer from S3 to take.

#endif /* AWS_IOT_CONFIG_H_ */
