
ToioBridge toio;
PVector targetPos;
boolean keyDown = false;

void setup() {
  size(800, 700);
  toio = new ToioBridge(this, Serial.list()[0], 115200);

  // ウィンドウにフォーカス
  ((java.awt.Canvas) surface.getNative()).requestFocus();
}

void draw() {
  background(50);

  textSize(20);
  fill(255);
  text("move : UP, DOWN, LEFT, RIGHT", 30, 30 );
  text("go to the destination : mouse click", 30, 55 ); 
  text("play sound : 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, - ", 30, 80 );

  fill(255, 255, 0);
  text("    x=" + toio.x + ", y="+toio.y + ", angle=" + toio.angle, 30, 120 );

  //-----------------------------------------------------------------------------
  int offset_x = 34;
  int offset_y = 35;
  int mat_width = 339 - 34;
  int mat_height = 250 - 35;

  translate( 30, 120 ); // 全体の表示位置決め
  scale(0.6);          // 表示サイズの調整

  // 現在の座標系でのマウスカーソルの座標値を取得
  targetPos = screenToLocal(mouseX, mouseY);

  stroke(255);
  strokeWeight(2);
  for ( int x=0; x<3; x++ ) {
    for (int y=0; y<4; y++) {
      pushMatrix();
      translate( x * mat_width + offset_x, y * mat_height + offset_y ); 
      fill(60);
      rect(0, 0, mat_width, mat_height );
      textSize(30);
      fill(255);
      text("#"+(x*4+y+1), 10, 35);
      popMatrix();
    }
  }

  // cube
  translate(toio.x, toio.y);
  rotate(radians(toio.angle));
  stroke(255, 255, 0);
  strokeWeight(3);
  noFill();
  rectMode(CENTER);
  rect( 0, 0, 20, 20 );
  rectMode(CORNER);
  line( 10, 0, 20, 0);
}

void mousePressed() {
  int max_speed = 50;
  toio.gotoDestination( (int)targetPos.x, (int)targetPos.y, max_speed );
}

void keyPressed() {
  if ( keyDown ) return;
  keyDown = true;

  if ( keyCode == UP )    toio.driveMotor(30, 30);
  if ( keyCode == DOWN )  toio.driveMotor(-30, -30);
  if ( keyCode == RIGHT ) toio.driveMotor(30, -30);
  if ( keyCode == LEFT )  toio.driveMotor(-30, 30);

  int volume = 255;
  if ( key == '1' ) toio.playSound(0, volume);
  if ( key == '2' ) toio.playSound(1, volume);
  if ( key == '3' ) toio.playSound(2, volume);
  if ( key == '4' ) toio.playSound(3, volume);
  if ( key == '5' ) toio.playSound(4, volume);
  if ( key == '6' ) toio.playSound(5, volume);
  if ( key == '7' ) toio.playSound(6, volume);
  if ( key == '8' ) toio.playSound(7, volume);
  if ( key == '9' ) toio.playSound(8, volume);
  if ( key == '0' ) toio.playSound(9, volume);
  if ( key == '-' ) toio.playSound(10, volume);
}

void keyReleased() {
  keyDown = false;
  toio.driveMotor(0, 0);
}

// スクリーン座標を現在の座標系の座標値に変換
PVector screenToLocal(float x, float y) {
  PVector in = new PVector(x, y);
  PVector out = new PVector();
  PMatrix2D current_matrix = new PMatrix2D();
  getMatrix(current_matrix);  
  current_matrix.invert();
  current_matrix.mult(in, out);
  return out;
}
