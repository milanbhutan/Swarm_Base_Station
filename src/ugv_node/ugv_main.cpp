#include <Arduino.h>
#include <esp_now.h>
#include <Wifi.h>
#include <esp_wifi.h>
#include <freeRTOS/FreeRTOS.h>
#include <freeRTOS/task.h>

void Command_Task(void *pvParams);
void Telemetry_Task(void *pvParams);
void Motor_Task(void *pvParams);
void Command_Received_Handler(const uint8_t *mac_addr, const uint8_t *data, int len);

constexpr int MOTOR_PIN_1 = 4;
constexpr int MOTOR_PIN_1_chan = 0;
constexpr int MOTOR_PIN_1_freq = 2000; //Hz


TaskHandle_t Command_Task_ID;
TaskHandle_t Telemetry_Task_ID;
TaskHandle_t Motor_Task_ID;

struct packet{
  uint8_t packet_type = 0;
  uint8_t command_type = 0;
};

enum Command{
  START_COMMAND = 's',
  HALT_COMMAND = 'h'
};

enum Packet_Type{
  COMMAND,
  TELEMETRY
};


packet telemetry_structure;
packet telemetry_sent;
packet command_received;

uint8_t BASE_STATION_MAC_ADDR[6] = {0x30,0x76,0xF5,0xB9,0xE2,0x94};

QueueHandle_t Packet_Queue_ID = xQueueCreate(10, sizeof(command_received));

uint8_t motor_status = START_COMMAND;

void setup() {

Serial.begin(115200);

//Wifi setup
WiFi.mode(WIFI_STA);
esp_now_init();

esp_now_register_recv_cb(Command_Received_Handler);

esp_now_peer_info_t peerInfo = {};
memcpy(peerInfo.peer_addr, BASE_STATION_MAC_ADDR, 6);
peerInfo.encrypt = false;
peerInfo.channel = 0;
peerInfo.ifidx = WIFI_IF_STA;

esp_now_add_peer(&peerInfo);


Serial.println("=================");
Serial.println("UGV MAC Address: ");
Serial.println(WiFi.macAddress());
Serial.println("================");

//Timer setup

Serial.println(ledcSetup(MOTOR_PIN_1_chan, MOTOR_PIN_1_freq, 8));
ledcAttachPin(MOTOR_PIN_1, MOTOR_PIN_1_chan);
ledcWrite(MOTOR_PIN_1_chan, 127);



//Task setup
constexpr configSTACK_DEPTH_TYPE STACK_SIZE_BYTES = 4096;

constexpr UBaseType_t COMMAND_TASK_PRIORITY = 5; 
constexpr UBaseType_t TELEMETRY_TASK_PRIORITY = 5;
constexpr UBaseType_t MOTOR_TASK_PRIORITY = 8;

  
xTaskCreate(
  Command_Task,
  "Command Task",
  STACK_SIZE_BYTES,
  nullptr,
  COMMAND_TASK_PRIORITY,
  &Command_Task_ID
  );

xTaskCreate(
  Telemetry_Task,
  "Telemetry Task",
  STACK_SIZE_BYTES,
  nullptr,
  TELEMETRY_TASK_PRIORITY,
  &Telemetry_Task_ID
);

xTaskCreate(
  Motor_Task,
  "Motor Task",
  STACK_SIZE_BYTES,
  nullptr,
  MOTOR_TASK_PRIORITY,
  &Motor_Task_ID
);

}



void loop() {
 vTaskDelay(pdMS_TO_TICKS(1000));
}


void Command_Task(void *pvParams){
    for(;;){
      xQueueReceive(Packet_Queue_ID, &command_received, portMAX_DELAY);
      motor_status = command_received.command_type;
      vTaskDelay(pdMS_TO_TICKS(25));
    }

}

void Telemetry_Task(void *pvParams){
    for(;;){
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Motor_Task(void *pvParams){
    for(;;){
      switch(motor_status){
        case START_COMMAND:
          ledcAttachPin(MOTOR_PIN_1, MOTOR_PIN_1_chan);
          ledcWrite(MOTOR_PIN_1_chan, 127);
          break;

        case HALT_COMMAND:
          ledcWrite(MOTOR_PIN_1, 0);
          ledcDetachPin(MOTOR_PIN_1);
          break;

        default:
          ledcWrite(MOTOR_PIN_1, 0);

      }
      vTaskDelay(pdMS_TO_TICKS(25));
    }

}

void Command_Received_Handler(const uint8_t *mac_addr, const uint8_t* data, int len){

if (len!=sizeof(command_received)){
  return;
}

memcpy(&command_received, data, len);
xQueueSend(Packet_Queue_ID, &command_received, 0);

}