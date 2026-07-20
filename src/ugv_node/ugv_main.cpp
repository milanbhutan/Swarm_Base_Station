#include <Arduino.h>
#include <esp_now.h>
#include <Wifi.h>
#include <esp_wifi.h>
#include <freeRTOS/FreeRTOS.h>
#include <freeRTOS/task.h>

void Execute_Command_Task(void *pvParams);
void Send_Telemetry_Task(void *pvParams);
void Motor_Task(void *pvParams);
void Get_Telem_Task(void *pvParams);
void Command_Received_Handler(const uint8_t *mac_address, const uint8_t *data, int len);

struct packet{
  uint8_t mac_addr[6];
  uint8_t command_type;
  uint8_t battery_life;
  uint8_t roll_angle;
  uint8_t pitch_angle;
  uint8_t yaw_angle;

};

enum Command{
  START_COMMAND = 's',
  HALT_COMMAND = 'h'
};


packet telemetry_sent = {};
packet command_received = {};

TaskHandle_t Execute_Command_Task_ID;
TaskHandle_t Send_Telemetry_Task_ID;
TaskHandle_t Motor_Task_ID;
TaskHandle_t Get_Telem_Task_ID;

QueueHandle_t Command_Queue_ID = xQueueCreate(10, sizeof(packet));
QueueHandle_t Telemetry_Queue_ID = xQueueCreate(10, sizeof(packet));

uint8_t BASE_STATION_MAC_ADDR[6] = {0x30, 0x76, 0xF5, 0xB9, 0xE2, 0x94};


constexpr int MOTOR_PIN_1 = 4;
constexpr int MOTOR_PIN_1_chan = 0;
constexpr int MOTOR_PIN_1_freq = 2000; //Hz
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
constexpr void *NO_TASK_PARAMS = nullptr;
constexpr UBaseType_t EXECUTE_COMMAND_TASK_PRIORITY = 8; 
constexpr UBaseType_t SEND_TELEMETRY_TASK_PRIORITY = 6;
constexpr UBaseType_t MOTOR_TASK_PRIORITY = 8;
constexpr UBaseType_t GET_TELEM_TASK_PRIORITY = 5;
  

xTaskCreate(
  Execute_Command_Task,
  "Execute Command Task",
  STACK_SIZE_BYTES,
  NO_TASK_PARAMS,
  EXECUTE_COMMAND_TASK_PRIORITY,
  &Execute_Command_Task_ID
  );

xTaskCreate(
  Send_Telemetry_Task,
  "Send Telemetry Task",
  STACK_SIZE_BYTES,
  NO_TASK_PARAMS,
  SEND_TELEMETRY_TASK_PRIORITY,
  &Send_Telemetry_Task_ID
);

xTaskCreate(
  Motor_Task,
  "Motor Task",
  STACK_SIZE_BYTES,
  NO_TASK_PARAMS,
  MOTOR_TASK_PRIORITY,
  &Motor_Task_ID
);


xTaskCreate(
  Get_Telem_Task,
  "Get Telem Task",
  STACK_SIZE_BYTES,
  NO_TASK_PARAMS,
  GET_TELEM_TASK_PRIORITY,
  &Get_Telem_Task_ID
);

}



void loop() {
 vTaskDelay(pdMS_TO_TICKS(1000));
}


void Execute_Command_Task(void *pvParams){
    for(;;){
      xQueueReceive(Command_Queue_ID, &command_received, portMAX_DELAY);
      motor_status = command_received.command_type;
      vTaskDelay(pdMS_TO_TICKS(25));
    }

}

void Send_Telemetry_Task(void *pvParams){
    for(;;){
      xQueueReceive(Telemetry_Queue_ID, &telemetry_sent, portMAX_DELAY);
      esp_now_send(BASE_STATION_MAC_ADDR,(uint8_t *)&telemetry_sent, sizeof(packet));
      vTaskDelay(pdMS_TO_TICKS(200));
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

void Get_Telem_Task(void *pvParams){
for(;;){
  telemetry_sent.battery_life += 1;
  telemetry_sent.roll_angle += 4;
  telemetry_sent.pitch_angle += 5;
  telemetry_sent.yaw_angle += 1;
  
  if(telemetry_sent.battery_life > 99){
    telemetry_sent.battery_life = 50;
  }

  if(telemetry_sent.roll_angle > 20){
    telemetry_sent.roll_angle = 0;
  }

  if(telemetry_sent.pitch_angle > 20){
    telemetry_sent.pitch_angle = 0;
  }

  if(telemetry_sent.yaw_angle > 180){
    telemetry_sent.yaw_angle = 0;
  }

  xQueueSend(Telemetry_Queue_ID, &telemetry_sent, portMAX_DELAY);
  vTaskDelay(pdMS_TO_TICKS(200));
}

}

void Command_Received_Handler(const uint8_t *mac_address, const uint8_t* data, int len){

if (len!=sizeof(packet)){
  return;
}

memcpy(&command_received, data, len);
xQueueSend(Command_Queue_ID, &command_received, 0);

}



