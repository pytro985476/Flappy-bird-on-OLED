#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// กำหนดขนาดจอ OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// สร้างอ็อบเจ็กต์จอ OLED (ใช้ I2C)
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// กำหนดขาอุปกรณ์ (Pin Assignment) ตามใบงาน
#define BTN_PIN 18      // ปุ่มกดกระโดด (ต่อกับ GPIO 18 และอีกขาลง GND)

// ตัวแปรสำหรับตัวละครนก (Bird)
float birdY = 32.0;           // ตำแหน่งแนวตั้งเริ่มต้น
float velocityY = 0.0;        // ความเร็วในแนวแกน Y
float gravity = 0.5;          // แรงโน้มถ่วง ดึงตกลงมา
float flapForce = -3.8;       // แรงกระโดดขึ้นเมื่อกดปุ่ม
int birdX = 20;               // ตำแหน่งแนวนอนของนก (คงที่)
int birdSize = 4;             // ขนาดตัวนก

// ตัวแปรสำหรับท่อสิ่งกีดขวาง (Pipes)
float pipeX = 128.0;          // ตำแหน่ง X ของท่อ
int pipeWidth = 12;           // ความกว้างของท่อ
int gapHeight = 26;           // ความกว้างของช่องว่างระหว่างท่อบนและล่าง
int pipeTopHeight = 20;       // ความสูงของท่อส่วนบน (จะสุ่มเปลี่ยนไป)
float pipeSpd = 2.5;          // ความเร็วที่ท่อวิ่งเข้าหาผู้เล่น

// ตัวแปรระบบเกม
int score = 0;
bool gameOver = false;
bool scoreCounted = false;    // ป้องกันการนับคะแนนซ้ำในท่อเดิม
int lastScoreCheck = 0;

void setup() {
  Serial.begin(115200);

  // กำหนดค่าเริ่มต้นปุ่มกดแบบ Pull-up (Active-LOW)[cite: 1, 2]
  pinMode(BTN_PIN, INPUT_PULLUP);

  // เริ่มต้นใช้งานจอ OLED (รองรับทั้งแอดเดรส 0x3C และ 0x3D อัตโนมัติ)
  bool displayInitialized = false;
  
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    displayInitialized = true;
    Serial.println(F("OLED found at 0x3C"));
  } else if(display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    displayInitialized = true;
    Serial.println(F("OLED found at 0x3D"));
  }

  if(!displayInitialized) {
    Serial.println(F("SSD1306 allocation failed. Check wiring (SDA->GPIO21, SCL->GPIO22)"));
    for(;;); // หยุดการทำงานถ้าหาจอไม่พบ
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // หน้าจอเริ่มต้น
  display.setCursor(22, 20);
  display.println(F("FLAPPY BIRD"));
  display.setCursor(15, 35);
  display.println(F("Press Button to Start"));
  display.display();

  // รอจนกว่าจะมีการกดปุ่มเริ่มเกม
  while(digitalRead(BTN_PIN) == HIGH) {
    delay(10);
  }
  delay(500); // ป้องกันการกดค้าง
}

void loop() {
  if (gameOver) {
    // หน้าจอจบเกม (Game Over)
    display.clearDisplay();
    display.setCursor(32, 20);
    display.setTextSize(1);
    display.println(F("GAME OVER"));
    
    display.setCursor(28, 35);
    display.print(F("Score: "));
    display.println(score);
    
    display.setCursor(10, 50);
    display.println(F("Press Button to Retry"));
    display.display();

    // รีสตาร์ทเกมเมื่อกดปุ่ม
    if (digitalRead(BTN_PIN) == LOW) {
      birdY = 32.0;
      velocityY = 0.0;
      pipeX = 128.0;
      score = 0;
      pipeSpd = 2.5;
      scoreCounted = false;
      pipeTopHeight = random(10, 30);
      gameOver = false;
      delay(500);
    }
    return;
  }

  // --- 1. การควบคุมนก (Input & Physics) ---
  // กดปุ่มเพื่อกระโดดขึ้น (Active-LOW)[cite: 1, 2]
  if (digitalRead(BTN_PIN) == LOW) {
    velocityY = flapForce;
    delay(50); // ดีบอนซ์ปุ่มเบื้องต้น
  }

  // แรงโน้มถ่วงดึงนกตกลงด้านล่าง (แกน Y เพิ่มขึ้น)
  velocityY += gravity;
  birdY += velocityY;

  // ตรวจสอบการชนขอบจอ บน/ล่าง
  if (birdY < 0) {
    birdY = 0;
    velocityY = 0;
  }
  if (birdY > 64 - birdSize) {
    gameOver = true; // ตกพื้นจอถือว่าตาย
  }

  // --- 2. การเคลื่อนที่ของท่อ (Pipe Logic) ---
  pipeX -= pipeSpd;
  if (pipeX < -pipeWidth) {
    pipeX = 128.0;                      // รีเซ็ตท่อกลับมาขวาสุด
    pipeTopHeight = random(10, 32);     // สุ่มความสูงของท่อบนใหม่
    scoreCounted = false;               // รีเซ็ตสถานะการนับแต้มสำหรับท่อชุดใหม่
  }

  // นับคะแนนเมื่อนกบินผ่านท่อพ้นไปแล้ว
  if (!scoreCounted && pipeX + pipeWidth < birdX) {
    score += 1;
    scoreCounted = true;
  }

  // เพิ่มความเร็วของเกมทุกๆ 5 คะแนน
  if (score > 0 && score % 5 == 0 && score != lastScoreCheck) {
    pipeSpd += 0.4;
    lastScoreCheck = score;
  }

  // --- 3. การตรวจจับการชน (AABB Collision Detection) ---
  // ขอบเขตตัวนก
  float birdLeft = birdX;
  float birdRight = birdX + birdSize;
  float birdTop = birdY;
  float birdBottom = birdY + birdSize;

  // ขอบเขตท่อบน
  float pipeTopLeft = pipeX;
  float pipeTopRight = pipeX + pipeWidth;
  float pipeTopBottom = pipeTopHeight;

  // ขอบเขตท่อล่าง
  float pipeBottomLeft = pipeX;
  float pipeBottomRight = pipeX + pipeWidth;
  float pipeBottomTop = pipeTopHeight + gapHeight;

  // เช็คการชนกับท่อบน หรือ ท่อล่าง
  if (birdRight > pipeTopLeft && birdLeft < pipeTopRight) {
    if (birdTop < pipeTopBottom || birdBottom > pipeBottomTop) {
      gameOver = true;
    }
  }

  // --- 4. การแสดงผลกราฟิกบนจอ OLED (Rendering) ---
  display.clearDisplay();

  // วาดคะแนนที่มุมซ้ายบน
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(F("Score: "));
  display.print(score);

  // วาดท่อส่วนบน
  display.fillRect(pipeX, 0, pipeWidth, pipeTopHeight, SSD1306_WHITE);

  // วาดท่อส่วนล่าง
  display.fillRect(pipeX, pipeTopHeight + gapHeight, pipeWidth, 64 - (pipeTopHeight + gapHeight), SSD1306_WHITE);

  // วาดตัวนก (สี่เหลี่ยมจัตุรัส)
  display.fillRect(birdX, birdY, birdSize, birdSize, SSD1306_WHITE);

  display.display();
  
  // ควบคุมเฟรมเรตเกม (ประมาณ 30-40 FPS)
  delay(25);
}