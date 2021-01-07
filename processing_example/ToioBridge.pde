import processing.serial.*;

public class ToioBridge {
  PApplet parent;
  Serial serial;
  SerialProxy serialProxy;
  int x, y, angle, std_id, std_angle;

  // コンストラクタ
  public ToioBridge(PApplet parent, String serial_name, int serial_rate) {
    this.parent = parent;
    this.serialProxy = new SerialProxy();
    this.serial = new Serial(serialProxy, serial_name, serial_rate);
    parent.registerMethod("dispose", this);
  }

  // モータ制御（連続動作）
  void driveMotor(int left_speed, int right_speed ) {
    serial.write("m " + left_speed + " " + right_speed + "\n" );
  }

  // モータ制御（時間指定）
  void driveMotor(int left_speed, int right_speed, int time_x100ms ) {
    serial.write("m " + left_speed + " " + right_speed + " " + time_x100ms + "\n" );
  }

  // 目標位置への移動
  void gotoDestination(int x, int y, int max_speed) {
    serial.write("d " + x + " " + y + " " + max_speed + "\n");
  }

  // サウンド再生
  void playSound(int sound_id, int volume) {
    serial.write("s " + sound_id + " " + volume + "\n");
  }

  // データ受信
  public class SerialProxy extends PApplet {
    public SerialProxy() {
    }
    public void serialEvent(Serial port) {
      if ( port.available() > 0 ) {
        String line = port.readStringUntil('\n');
        if ( line != null ) {
          // print(line);
          String[] value = split(line, ',');
          if ( value.length == 5 ) {
            x = int(trim(value[0]));
            y = int(trim(value[1]));
            angle = int(trim(value[2]));
            std_id = int(trim(value[3]));
            std_angle = int(trim(value[4]));
            // println( x, y, angle, std_id, std_angle );
          }
        }
      }
    }
  }

  // 終了時の処理
  public void dispose() {
    this.serial.dispose();
  }
}
