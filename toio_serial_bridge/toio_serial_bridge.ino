/*
シリアル通信によるコマンド

【モータ制御】
  m [left_speed] [right_speed]
  m [left_speed] [right_speed] [time_x100ms]

  例：
    m 100 100     左輪の速度100、右輪の速度100で移動
    m 100 100 10  左輪の速度100、右輪の速度100で1000ms間移動
    m 0 0         停止

【目標位置へ移動】
  d [x] [y] [max_speed]
  d [x] [y] [max_speed] [angle] [move_type] [speed_type] [angle_type]

  [move_type]   0:回転しながら移動
                1:回転しながら移動（後退なし）
                2:回転してから移動

  [speed_type]  0:速度一定
                1:目標地点まで徐々に加速
                2:目標地点まで徐々に減速, 
                3:中間地点まで徐々に加速し、そこから目標地点まで減速
  
  [angle_type]  0:絶対角度/回転量が少ない方向
                1:絶対角度/正方向
                2:絶対角度/負方向
                3:相対角度/正方向
                4:相対角度/負方向
                5:角度指定なし/回転しない
                6:書き込み操作時と同じ（動作開始時と同じ角度）/回転量が少ない方向
                
  例：
    d 500 150 100           目標位置(500, 150)に速度100で移動
    d 500 150 100 45 0 3 0  目標位置(500, 150)に速度100で移動、角度45度、回転しながら移動、加減速、絶対角度/回転量が少ない方向
  
【サウンド再生】
  s [id] [volume]

  例：
    s 1 200   音1を音量200で再生

*/


// This code is based on:
// https://github.com/Katsushun89/M5Stack_BLE_toio_controller

#include <M5StickC.h>
#include <BLEDevice.h>

const byte CONTROL_CONTINUOUS = 0x01;         // モータ制御（連続動作）
const byte CONTROL_TIME_SPECIFIED = 0x02;     // 時間指定付きモータ制御
const byte CONTROL_POSITION_SPECIFIED = 0x03; // 目標指定付きモータ制御

const byte MOVE_WITH_ROTATION = 0x00;          // 回転しながら移動
const byte MOVE_WITH_ROTATION_NO_BACK = 0x01;  // 回転しながら移動（後退なし）
const byte MOVE_AFTER_ROTATION = 0x02;         // 回転してから移動

const byte SPEED_CONSTANT = 0x00;    // 速度一定
const byte SPEED_ACCELERATED = 0x01; // 目標地点まで徐々に加速
const byte SPEED_DECELERAED = 0x02;  // 目標地点まで徐々に減速
const byte SPEED_ACC_AND_DEC = 0x03; // 中間地点まで徐々に加速し、そこから目標地点まで減速

const byte DEST_ANGLE_ABS = 0x00;         // 絶対角度  回転量が少ない方向
const byte DEST_ANGLE_ABS_POS = 0x01;     // 絶対角度  正方向
const byte DEST_ANGLE_ABS_NEG = 0x02;     // 絶対角度  負方向
const byte DEST_ANGLE_REL_POS = 0x03;     // 相対角度  正方向
const byte DEST_ANGLE_REL_NEG = 0x04;     // 相対角度  負方向
const byte DEST_ANGLE_NO_ROTATION = 0x05; // 角度指定なし  回転しない
const byte DEST_ANGLE_SAME_AS_START = 0x06;  // 書き込み操作時と同じ（動作開始時と同じ角度）  回転量が少ない方向

const byte MOTOR_ID_LEFT = 0x01;
const byte MOTOR_ID_RIGHT = 0x02;
const byte MOTOR_DIR_FRONT = 0x01;
const byte MOTOR_DIR_BACK = 0x02;

const byte PLAY_SOUND = 0x02;
const byte NONE = 0x00;

static BLEUUID service_UUID("10B20100-5B3B-4571-9508-CF3EFCD7BBAE");// toio service
static BLEUUID read_char_UUID("10B20101-5B3B-4571-9508-CF3EFCD7BBAE");// read sensor characteristic
static BLEUUID motor_char_UUID("10B20102-5B3B-4571-9508-CF3EFCD7BBAE");// motor characteristic
static BLEUUID sound_char_UUID("10B20104-5B3B-4571-9508-CF3EFCD7BBAE");// sound characteristic

static BLERemoteCharacteristic* read_characteristic;
static BLERemoteCharacteristic* motor_characteristic;
static BLERemoteCharacteristic* sound_characteristic;
static BLEAdvertisedDevice* my_device;

static boolean connected = false;
//static boolean do_connect = false;
//static boolean do_scan = false;

typedef struct {
  uint16_t x_cube_center;
  uint16_t y_cube_center;
  uint16_t angle_cube_center;
  uint16_t x_read_sensor;
  uint16_t y_read_sensor;
  uint16_t angle_read_sensor;
} position_id_t;

typedef struct {
  uint32_t standard_id;
  uint16_t angle_cube;
} standard_id_t;

static position_id_t position_id = {0};
static standard_id_t standard_id = {0};
static bool is_missed_position_id = false;
static bool is_missed_standard_id = false;

static void readPositionID(uint8_t *data, size_t length) {
  memcpy(&position_id, &data[1], sizeof(position_id));
}

static void readStandardID(uint8_t *data, size_t length) {
  memcpy(&standard_id, &data[1], sizeof(standard_id));
}

static void readPositionIDMissed(uint8_t *data, size_t length) {
  is_missed_position_id = true;
  position_id.x_cube_center = 0;
  position_id.y_cube_center = 0;
  position_id.angle_cube_center = 0;
  position_id.x_read_sensor = 0;
  position_id.y_read_sensor = 0;
  position_id.angle_read_sensor = 0;
}

static void readStandardIDMissed(uint8_t *data, size_t length) {
  is_missed_standard_id = true;
  standard_id.standard_id = 0;
  standard_id.angle_cube = 0;
}

void (*readFunctionTable[])(uint8_t *data, size_t length) = {
  &readPositionID,
  &readStandardID,
  &readPositionIDMissed,
  &readStandardIDMissed
};

static void selectReadFunction(uint8_t *data, size_t length) {
  uint8_t id = data[0];
  // Serial.printf("read id:%d\n", id);
  // need to size check
  readFunctionTable[id - 1](data, length);
}

static void notifyReadCallback( BLERemoteCharacteristic* ble_remote_characterisstic,
                                uint8_t* data,
                                size_t length,
                                bool is_notify) {

  selectReadFunction(data, length);
  Serial.printf("%d,", position_id.x_cube_center);
  Serial.printf("%d,", position_id.y_cube_center);
  Serial.printf("%d,", position_id.angle_cube_center);
  Serial.printf("%d,", standard_id.standard_id);
  Serial.printf("%d\n", standard_id.angle_cube);
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* client) {
      connected = true;
      Serial.println("onConnect");
    }
    void onDisconnect(BLEClient* client) {
      connected = false;
      Serial.println("onDisconnect");
    }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(my_device->getAddress().toString().c_str());

  BLEClient*  client  = BLEDevice::createClient();
  Serial.println(" - Created client");

  client->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  client->connect(my_device);  // if you pass BLEadvertised_device instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* remote_service = client->getService(service_UUID);
  if (remote_service == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(service_UUID.toString().c_str());
    client->disconnect();
    return false;
  }
  Serial.println(" - Found toio service");

  //read
  read_characteristic = remote_service->getCharacteristic(read_char_UUID);
  if (read_characteristic == nullptr) {
    Serial.print("Failed to find read characteristic UUID: ");
    Serial.println(read_char_UUID.toString().c_str());
    client->disconnect();
    return false;
  }
  Serial.println(" - Found read characteristic");

  // Read the value of the characteristic.
  if (read_characteristic->canRead()) {
    std::string value = read_characteristic->readValue();
    Serial.print("read characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (read_characteristic->canNotify())
    read_characteristic->registerForNotify(notifyReadCallback);

  //motor
  motor_characteristic = remote_service->getCharacteristic(motor_char_UUID);
  if (motor_characteristic == nullptr) {
    Serial.print("Failed to find motor characteristic UUID: ");
    Serial.println(motor_char_UUID.toString().c_str());
    client->disconnect();
    return false;
  }
  Serial.println(" - Found motor characteristic");

  //sound
  sound_characteristic = remote_service->getCharacteristic(sound_char_UUID);
  if (sound_characteristic == nullptr) {
    Serial.print("Failed to find sound characteristic UUID: ");
    Serial.println(sound_char_UUID.toString().c_str());
    client->disconnect();
    return false;
  }
  Serial.println(" - Found sound characteristic");
}

//Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    // Called for each advertising BLE server.
    void onResult(BLEAdvertisedDevice advertised_device) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertised_device.toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertised_device.haveServiceUUID() && advertised_device.isAdvertisingService(service_UUID)) {

        BLEDevice::getScan()->stop();
        my_device = new BLEAdvertisedDevice(advertised_device);
        // do_connect = true;
        // do_scan = true;

      } // Found our server
    } // onResult
}; // Myadvertised_deviceCallbacks


// モータ制御（連続動作）
void driveMotor(int left_speed, int right_speed) {
  uint8_t data[7];

  data[0] = CONTROL_CONTINUOUS;
  data[1] = MOTOR_ID_LEFT;
  data[2] = ( left_speed >= 0 ) ? MOTOR_DIR_FRONT : MOTOR_DIR_BACK;
  data[3] = (byte)constrain(abs(left_speed), 0, 255);
  data[4] = MOTOR_ID_RIGHT;
  data[5] = ( right_speed >= 0 ) ? MOTOR_DIR_FRONT : MOTOR_DIR_BACK;
  data[6] = (byte)constrain(abs(right_speed), 0, 255);

  motor_characteristic->writeValue(data, sizeof(data));
}

// 時間指定付きモータ制御
void driveMotor(int left_speed, int right_speed, int time_x100ms) {
  uint8_t data[8];

  data[0] = CONTROL_TIME_SPECIFIED;
  data[1] = MOTOR_ID_LEFT;
  data[2] = ( left_speed >= 0 ) ? MOTOR_DIR_FRONT : MOTOR_DIR_BACK;
  data[3] = (byte)constrain(abs(left_speed), 0, 255);
  data[4] = MOTOR_ID_RIGHT;
  data[5] = ( right_speed >= 0 ) ? MOTOR_DIR_FRONT : MOTOR_DIR_BACK;
  data[6] = (byte)constrain(abs(right_speed), 0, 255);
  data[7] = (byte)constrain(time_x100ms, 0, 255);

  motor_characteristic->writeValue(data, sizeof(data));
}

// 目標指定付きモータ制御
void gotoDestination(int x, int y, int max_speed) {
  gotoDestination( x, y, max_speed, 0, MOVE_WITH_ROTATION_NO_BACK, SPEED_CONSTANT, DEST_ANGLE_NO_ROTATION );
}

// 目標指定付きモータ制御
void gotoDestination(int x, int y, int max_speed, int angle, int move_type, int speed_type, int angle_type ) {
  uint8_t data[13];
  int timeout = 5; // 秒
  int control_id_value = 0; // 制御識別値

  data[0] = CONTROL_POSITION_SPECIFIED;
  data[1] = (byte)control_id_value;
  data[2] = (byte)timeout;
  data[3] = (byte)move_type;
  data[4] = (byte)max_speed;
  data[5] = (byte)speed_type;
  data[6] = NONE;
  data[7] = lowByte(x);
  data[8] = highByte(x);
  data[9] = lowByte(y);
  data[10] = highByte(y);
  data[11] = lowByte(angle);
  data[12] = (byte)(highByte(angle) + ((byte)angle_type << 5));

  motor_characteristic->writeValue(data, sizeof(data));
}

// サウンド再生
void playSound(int sound_id, int volume) {
  uint8_t data[3];

  data[0] = PLAY_SOUND;
  data[1] = (byte)constrain(sound_id, 0, 10);
  data[2] = (byte)constrain(volume, 0, 255);

  sound_characteristic->writeValue(data, sizeof(data));
}

// センシングデータの表示
static void drawReadSensor(void) {
  int xpos = 5;
  int ypos = 0;
  int line_height = 16;

  M5.Lcd.setCursor(xpos, 0 * line_height);  M5.Lcd.printf("x : %d    ", position_id.x_cube_center);
  M5.Lcd.setCursor(xpos, 1 * line_height);  M5.Lcd.printf("y : %d    ", position_id.y_cube_center);
  M5.Lcd.setCursor(xpos, 2 * line_height);  M5.Lcd.printf("angle : %d    ", position_id.angle_cube_center);
  M5.Lcd.setCursor(xpos, 3 * line_height);  M5.Lcd.printf("standard id : %d        ", standard_id.standard_id);
  M5.Lcd.setCursor(xpos, 4 * line_height);  M5.Lcd.printf("standard angle : %d   ", standard_id.angle_cube);

  // connection status
  if ( connected ) {
    M5.Lcd.fillRect(155, 0, 5, 80, GREEN);
  } else {
    M5.Lcd.fillRect(155, 0, 5, 80, RED);
  }
}

// ボタン入力に対する処理
static void checkPushButton(void) {
  if ( connected ) {
    if ( M5.BtnA.wasPressed() ) {
      driveMotor(30, 30);
    }
    else if ( M5.BtnB.wasPressed() ) {
      driveMotor(30, -30);
    }
    else if ( M5.BtnA.wasReleased() || M5.BtnB.wasReleased() ) {
      driveMotor(0, 0);
    }
  }
}

// シリアルからの入力に対する処理
void processSerialInput() {
  if ( Serial.available() ) {
    String cmd = Serial.readStringUntil('\n');
    String key = getValue(cmd, ' ', 0);

    // モータ制御
    if ( key.equals("m") ) {
      int left_speed  = getValue(cmd, ' ', 1).toInt();
      int right_speed = getValue(cmd, ' ', 2).toInt();
      String time_x100ms = getValue(cmd, ' ', 3);

      if ( time_x100ms == "" ) {
        driveMotor(left_speed, right_speed);
      } else {
        driveMotor(left_speed, right_speed, time_x100ms.toInt());
      }
    }
    // 目標位置へ移動
    else if ( key.equals("d") ) {
      int x = getValue(cmd, ' ', 1).toInt();
      int y = getValue(cmd, ' ', 2).toInt();
      int max_speed = getValue(cmd, ' ', 3).toInt();
      String angle_s = getValue(cmd, ' ', 4);

      if ( angle_s == "" ) {
        gotoDestination( x, y, max_speed );
      } else {
        int angle = angle_s.toInt();
        int move_type = getValue(cmd, ' ', 5).toInt();
        int speed_type = getValue(cmd, ' ', 6).toInt();
        int angle_type = getValue(cmd, ' ', 7).toInt();
        gotoDestination( x, y, max_speed, angle, move_type, speed_type, angle_type );
      }
    }
    // サウンド再生
    else if ( key.equals("s") ) {
      int id = getValue(cmd, ' ', 1).toInt();
      int volume = getValue(cmd, ' ', 2).toInt();
      playSound(id, volume);
    }
  }
}

// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(0x0000);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextFont(2);  // 16px
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(0xFFFF, 0x0000);

  Serial.begin(115200);
  BLEDevice::init("");

  BLEScan* ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  ble_scan->setInterval(1349);
  ble_scan->setWindow(449);
  ble_scan->setActiveScan(true);
  // ble_scan->start(5, false);
}

void loop() {
  M5.update();

  processSerialInput();
  checkPushButton();
  drawReadSensor();

  if ( !connected ) {
    BLEDevice::getScan()->start(0);
    connectToServer();
  }

  delay(5);
}
