#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//Base Station MAC address: 30:76:F5:B9:E2:94 (COM13)
//UGV1 MAC address (ESP32-S3): A4:CB:8F:D9:2D:D8 (COM5)
//UGV2 MAC address (ESP32-S3): 30:30:F9:16:39:40 (COM4)
//UGV3 MAC address (ESP32-S3): 84:FC:E6:65:30:30 (COM6)


struct packet{
  uint8_t mac_addr[6];
  uint8_t command_type;
  uint8_t battery_life;
  uint8_t roll_angle;
  uint8_t pitch_angle;
  uint8_t yaw_angle;
};


void Send_Command_Task(void *pvParams);
void Display_Telemetry_Task(void *pvParams);
void Mode_Check_Task(void *pvParams);
void Telemetry_Received_Handler(const uint8_t* mac_address, const uint8_t* data, int len);

TaskHandle_t Send_Command_Task_ID = nullptr;
TaskHandle_t Display_Telemetry_Task_ID = nullptr;
TaskHandle_t Mode_Check_ID = nullptr;

enum Mode{
COMMAND_MODE = 'c',
TELEMETRY_MODE = 't',
EXIT = 'e'
};

enum Command{
START_COMMAND = 's',
HALT_COMMAND = 'h'
};

enum Packet_Type{
COMMAND,
TELEMETRY
};

bool TELEM_MODE = false;

packet start_command = {};
packet halt_command = {};
packet command_sent = {};
packet telemetry_received = {};
packet UGV1_telem = {};
packet UGV2_telem = {};
packet UGV3_telem = {};


uint8_t UGV1_MAC_ADDR[6] = {0xA4, 0xCB, 0x8F, 0xD9, 0x2D, 0xD8};
uint8_t UGV2_MAC_ADDR[6] = {0x30, 0x30, 0xF9, 0x16, 0x39, 0x40};
uint8_t UGV3_MAC_ADDR[6] = {0x84, 0xFC, 0xE6, 0x65, 0x30, 0x30};

QueueHandle_t Command_Queue_ID = xQueueCreate(10 , sizeof(packet));
QueueHandle_t Telemetry_Queue_ID = xQueueCreate(10, sizeof(packet));

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_now_init();

  esp_now_register_recv_cb(Telemetry_Received_Handler);
  
  esp_now_peer_info_t peerInfo_UGV1 = {};
  memcpy(peerInfo_UGV1.peer_addr, UGV1_MAC_ADDR, 6);
  peerInfo_UGV1.channel = 0;
  peerInfo_UGV1.encrypt = false;
  peerInfo_UGV1.ifidx = WIFI_IF_STA;

  esp_now_add_peer(&peerInfo_UGV1);
  

  esp_now_peer_info_t peerInfo_UGV2 = {};
  memcpy(peerInfo_UGV2.peer_addr, UGV2_MAC_ADDR, 6);
  peerInfo_UGV2.channel = 0;
  peerInfo_UGV2.encrypt = false;
  peerInfo_UGV2.ifidx = WIFI_IF_STA;

  esp_now_add_peer(&peerInfo_UGV2);


  esp_now_peer_info_t peerInfo_UGV3 = {};
  memcpy(peerInfo_UGV3.peer_addr, UGV3_MAC_ADDR, 6);
  peerInfo_UGV3.channel = 0;
  peerInfo_UGV3.encrypt = false;
  peerInfo_UGV3.ifidx = WIFI_IF_STA;

  esp_now_add_peer(&peerInfo_UGV3);





  Serial.println("=====================");
  Serial.println("Base Station MAC Address"); 
  Serial.println(WiFi.macAddress());
  Serial.println("=====================");
  
  
  constexpr configSTACK_DEPTH_TYPE STACK_SIZE_BYTES = 4096;
  constexpr void *NO_TASK_PARAMS = nullptr;
  constexpr UBaseType_t SEND_COMMAND_TASK_PRIORITY = 5;
  constexpr UBaseType_t DISPLAY_TELEMETRY_TASK_PRIORITY = 5;
  constexpr UBaseType_t MODE_CHECK_TASK_PRIORITY = 3;

  
  start_command.command_type = START_COMMAND;
  halt_command.command_type = HALT_COMMAND;
  
  xTaskCreate(
    Send_Command_Task, 
    "Send Command Task",
    STACK_SIZE_BYTES,
    NO_TASK_PARAMS,
    SEND_COMMAND_TASK_PRIORITY,
    &Send_Command_Task_ID
  );

  xTaskCreate(
    Display_Telemetry_Task,
    "Display Telemetry Task",
    STACK_SIZE_BYTES,
    NO_TASK_PARAMS,
    DISPLAY_TELEMETRY_TASK_PRIORITY,
    &Display_Telemetry_Task_ID
  );

  xTaskCreate(
    Mode_Check_Task,
    "Mode Check Task",
    STACK_SIZE_BYTES,
    NO_TASK_PARAMS,
    MODE_CHECK_TASK_PRIORITY,
    &Mode_Check_ID
  );

}

void loop(){
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void Send_Command_Task(void *pvParams){
  for(;;){
    xQueueReceive(Command_Queue_ID, &command_sent, portMAX_DELAY);
    esp_now_send(NULL, (uint8_t *)&command_sent, sizeof(packet));
    vTaskDelay(pdMS_TO_TICKS(10));
  }

}

void Display_Telemetry_Task(void *pvParams){
  for(;;){
    for(int num_packets = 0; num_packets < 3; num_packets++){
      xQueueReceive(Telemetry_Queue_ID, &telemetry_received, portMAX_DELAY);
    
      if(memcmp(telemetry_received.mac_addr, UGV1_MAC_ADDR, 6) == 0){
        memcpy(&UGV1_telem, &telemetry_received, sizeof(packet));
      }

      if(memcmp(telemetry_received.mac_addr, UGV2_MAC_ADDR, 6 ) == 0){
        memcpy(&UGV2_telem, &telemetry_received, sizeof(packet));
      }

      if(memcmp(telemetry_received.mac_addr, UGV3_MAC_ADDR, 6) == 0){
        memcpy(&UGV3_telem, &telemetry_received, sizeof(packet));
      }

  }

    if(TELEM_MODE == true){
      Serial.print("\n");
      Serial.print("UGV 1 Telemetry: \n");
      Serial.print("Battery Life: ");
      Serial.print(UGV1_telem.battery_life);
      Serial.print("%\n");
      Serial.print("Roll Angle: ");
      Serial.print(UGV1_telem.roll_angle);
      Serial.print("°\n");
      Serial.print("Pitch Angle: ");
      Serial.print(UGV1_telem.pitch_angle);
      Serial.print("°\n");
      Serial.print("Yaw Angle: ");
      Serial.print(UGV1_telem.yaw_angle);
      Serial.print("°\n");
      
      Serial.print("\n");
      Serial.print("UGV 2 Telemetry: \n");
      Serial.print("Battery Life: ");
      Serial.print(UGV2_telem.battery_life);
      Serial.print("%\n");
      Serial.print("Roll Angle: ");
      Serial.print(UGV2_telem.roll_angle);
      Serial.print("°\n");
      Serial.print("Pitch Angle: ");
      Serial.print(UGV2_telem.pitch_angle);
      Serial.print("°\n");
      Serial.print("Yaw Angle: ");
      Serial.print(UGV2_telem.yaw_angle);
      Serial.print("°\n");

      Serial.print("\n");
      Serial.print("UGV 3 Telemetry: \n");
      Serial.print("Battery Life: ");
      Serial.print(UGV3_telem.battery_life);
      Serial.print("%\n");
      Serial.print("Roll Angle: ");
      Serial.print(UGV3_telem.roll_angle);
      Serial.print("°\n");
      Serial.print("Pitch Angle: ");
      Serial.print(UGV3_telem.pitch_angle);
      Serial.print("°\n");
      Serial.print("Yaw Angle: ");
      Serial.print(UGV3_telem.yaw_angle);
      Serial.print("°\n");
      Serial.print("\n");
      Serial.print("===============================");
      Serial.print("\n");
    }


    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void Mode_Check_Task(void *pvParams){
  for(;;){
    if(Serial.available()>0){
      switch(Serial.read()){
        case COMMAND_MODE:{
          TELEM_MODE = false;
          Serial.println("Command Mode");
          int exit_status=false;
          while(!exit_status){
            if(Serial.available()>0){  
              switch(Serial.read()){
                case START_COMMAND:{
                  Serial.println("Sending Start Command");
                  xQueueSend(Command_Queue_ID, &start_command, portMAX_DELAY);
                  break;
                }
                case HALT_COMMAND:{
                  Serial.println("Sending Halt Command");
                  xQueueSend(Command_Queue_ID, &halt_command, portMAX_DELAY);
                  break;
                }
                case EXIT:{
                  Serial.println("Exiting Command Mode");
                  exit_status=true;
                  break;
                }
                default:{
                  Serial.println("Invalid Command");
                }
              }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
          }
          break;
        }
        case TELEMETRY_MODE:{
          Serial.println("Telemetry Mode");
          TELEM_MODE = true;
          break;
        }
        default:{
          Serial.println("Invalid Mode");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

}

void Telemetry_Received_Handler(const uint8_t* mac_address, const uint8_t* data, int len){

if(len!=sizeof(telemetry_received)){
  return;
}

memcpy(&telemetry_received, data, sizeof(packet));
memcpy(telemetry_received.mac_addr, mac_address, 6);
xQueueSend(Telemetry_Queue_ID, &telemetry_received, 0);

}

