// #include <WiFi.h>
// #include <WiFiUdp.h>


#include <queue>
#include <vector>
#include <Wire.h>
#include <VL53L0X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>  //Thư viện màn hình

// const char* ssid = "Yang";               // Thay bằng SSID WiFi của bạn
// const char* password = "quenmatroihee";  // Thay bằng mật khẩu WiFi của bạn

// WiFiUDP udp;
// const char* udpAddress = "192.168.187.42";  // Đổi thành IP máy tính của bạn
// const int udpPort = 8888;


// Định nghĩa chân nút nhấn
#define BUTTON_SELECT 21
#define BUTTON_BACK 20

// ------------------- khối khai báo encoder  -------------------
// Biến lưu trạng thái encoder
volatile int encoder1Position = 0;
volatile int encoder2Position = 0;
volatile int8_t lastState1 = 0;
volatile int8_t lastState2 = 0;


// Định nghĩa chân encoder
#define ENCODER2_PIN_A 4
#define ENCODER2_PIN_B 5
#define ENCODER1_PIN_A 6
#define ENCODER1_PIN_B 7


// Xử lý ngắt encoder
void IRAM_ATTR encoder1ISR() {
  int8_t stateA = digitalRead(ENCODER1_PIN_A);
  int8_t stateB = digitalRead(ENCODER1_PIN_B);

  if (stateA != lastState1) {
    encoder1Position += (stateA == stateB) ? 1 : -1;
    lastState1 = stateA;
  }
}

void IRAM_ATTR encoder2ISR() {
  int8_t stateA = digitalRead(ENCODER2_PIN_A);
  int8_t stateB = digitalRead(ENCODER2_PIN_B);

  if (stateA != lastState2) {
    encoder2Position += (stateA == stateB) ? -1 : 1;
    lastState2 = stateA;
  }
}

// ------------------- khối khai báo cảm biến -------------------
#define TCA9548A_ADDR 0x70
#define SENSOR_COUNT 6  // Số lượng cảm biến đang sử dụng

VL53L0X sensors[SENSOR_COUNT];
uint8_t sensorChannels[SENSOR_COUNT] = { 2, 3, 4, 5, 6, 7 };  // Kênh cảm biến 4 và 5 trên TCA9548A
int16_t offsets[SENSOR_COUNT] = { 13, 11, 10, 20, 11, 11 };   // Giá trị xoay quanh của cảm biến


// ------------------- khối khai báo màn màn hình -------------------
// Cấu hình màn hình OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SDA_PIN 9
#define SCL_PIN 10

// Khởi tạo màn hình OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// ------------------- khối khai báo điều khiển động cơ -------------------
// Định nghĩa chân điều khiển động cơ
// Chân nối với DRV8833
#define IN1 3  // Motor A
#define IN2 2
#define IN3 0  // Motor B
#define IN4 1

float efficiency = 0.8;  // Hệ số hiệu suất (0 - 1)


// ------------------- khối khai báo hiển thị trên màn màn hình -------------------
// Biến trạng thái menu
bool inSubmenu = false;      // Trạng thái submenu
int currentItem = 0;         // Mục hiện tại được chọn
int scrollOffset = 0;        // Độ cuộn của danh sách
const int totalItems = 9;    // Tổng số mục trong danh sách
const int visibleItems = 4;  // Số mục hiển thị trên màn hình




// ------------------- khối khai báo ma trận chứa mê cung -------------------
#define CELL_SIZE 4
#define MAZE_SIZE 33
#define VIEWPORT_HEIGHT (SCREEN_HEIGHT / CELL_SIZE)

int offsetY = 0;

// Ma trận mê cung (rút gọn để tiết kiệm không gian)
int maze[MAZE_SIZE][MAZE_SIZE] = {
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }

};



// ------------------- khối khai báo điều khiển động cơ -------------------
// Hệ số PID
float Kp = 5;     // Hệ số tỷ lệ
float Ki = 0.04;  // Hệ số tích phân
float Kd = 10;    // Hệ số vi phân
int PIDvalue;
float previousError = 0;  // Lỗi trước đó
float integral = 0;       // Tích phân của lỗi của hàm bám 2 cảm biến trước


float step = 1140;  //khai báo số bước cần đi để đi được 1 pixel
float step_coefficient = 0.1;
int length = 18;  // chiều dài của 1 pixel
float turn = 1.5;
int condition_after_turn = false;
float rotation_coefficient = 1.1;



int left = 0;
int up = 0;
int right = 0;

// ------------------- khối khai báo danh sách, giải mê cung và vị trí của robot -------------------
struct Point {
  int x, y;
};

int dx[] = { -1, 0, 1, 0 };
int dy[] = { 0, 1, 0, -1 };

int distanceMap[MAZE_SIZE][MAZE_SIZE];
int startDirection = 1;  //0 trên, 1 phải, 2 dưới, 3 trái
//{0, 1} là quay trái, {0, 2} là quay đầu, {0, 3} là rẽ phải

struct PathStep {
  int steps;
  int action;
};
#define MAX_PATH 1000
PathStep path[MAX_PATH];
int pathIndex = 0;

int location_maze_x = 1;  //trục tung
int location_maze_y = 1;  //trục hoành

//Đích số 1 (điểm suất phát)
int destination_maze_x1 = 1;  //trục tung
int destination_maze_y1 = 1;  //trục hoành

//Đích số 2 (1 trong 4 ô đích ở giữa)
int destination_maze_x2 = 15;  //trục tung
int destination_maze_y2 = 15;  //trục hoành

int obtain = true;  // biến lưu việc micromouse đã tìm được đường ngắn nhất hay chưa

int change_maze = false;

void floodFill(int endX, int endY) {
  std::queue<Point> q;
  q.push({ endX, endY });
  distanceMap[endX][endY] = 0;

  while (!q.empty()) {
    Point p = q.front();
    q.pop();
    for (int i = 0; i < 4; i++) {
      int nx = p.x + dx[i];
      int ny = p.y + dy[i];
      if (nx >= 0 && ny >= 0 && nx < MAZE_SIZE && ny < MAZE_SIZE && maze[nx][ny] != 1 && distanceMap[nx][ny] == -1) {
        distanceMap[nx][ny] = distanceMap[p.x][p.y] + 1;
        q.push({ nx, ny });
      }
    }
  }
}

void findPath(int startX, int startY, int endX, int endY) {
  pathIndex = 0;
  //PathStep path[MAX_PATH];
  Point p = { startX, startY };
  int prevDir = startDirection;
  int stepCount = 0;

  while (p.x != endX || p.y != endY) {
    for (int i = 0; i < 4; i++) {
      int nx = p.x + dx[i];
      int ny = p.y + dy[i];
      if (nx >= 0 && ny >= 0 && nx < MAZE_SIZE && ny < MAZE_SIZE && distanceMap[nx][ny] != -1 && distanceMap[nx][ny] < distanceMap[p.x][p.y]) {
        int action = (prevDir - i + 4) % 4;
        if (prevDir != i) {
          if (stepCount > 0) {
            path[pathIndex++] = { stepCount, 0 };
          }
          path[pathIndex++] = { 0, action };
          stepCount = 0;
        }
        p.x = nx;
        p.y = ny;
        prevDir = i;
        stepCount++;
        break;
      }
    }
  }
  if (stepCount > 0) {
    path[pathIndex++] = { stepCount, 0 };
  }
}






void initializeMazeSolver(int maze_x, int maze_y) {

  if (maze_x == destination_maze_x1 && maze_y == destination_maze_y1) {
    maze[maze_x][maze_y] = 2;
    maze[destination_maze_x2][destination_maze_y2] = 3;
  }
  if (maze_x == destination_maze_x2 && maze_y == destination_maze_y2) {
    maze[maze_x][maze_x] = 2;
    maze[destination_maze_x1][destination_maze_y1] = 3;
  }

  maze[maze_x][maze_y] = 2;
  int startX, startY, endX, endY;
  for (int i = 0; i < MAZE_SIZE; i++) {
    for (int j = 0; j < MAZE_SIZE; j++) {
      if (maze[i][j] == 2) {
        startX = i;
        startY = j;
      }
      if (maze[i][j] == 3) {
        endX = i;
        endY = j;
      }
      distanceMap[i][j] = -1;
    }
  }
  floodFill(endX, endY);
  findPath(startX, startY, endX, endY);
  maze[maze_x][maze_y] = 0;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void pid_flow_side(int d1, int d2) {
  int error_difference = (d1 <= d2) ? d1 : d2;

  if (error_difference > 0) {
    if (integral + error_difference < 100) {
      integral = integral + error_difference;
    }
  } else {
    if (integral + error_difference > -100) {
      integral = integral + error_difference;
    }
  }

  PIDvalue = Kp * error_difference + Ki * (integral / 20) + Kd * (error_difference - previousError);
  previousError = error_difference;

  if (d1 <= d2) {
    setMotor(255 - PIDvalue, 255 + PIDvalue);
  } else {
    setMotor(255 + PIDvalue, 255 - PIDvalue);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void pid_flow_front(int d1, int d2) {
  int error_difference = d1 - d2;
  float error_amplification = abs(d1) + abs(d2);
  if (error_difference > 0) {
    if (integral + error_difference < 40) {
      integral = integral + error_difference;
    }
  } else {
    if (integral + error_difference > -40) {
      integral = integral + error_difference;
    }
  }

  PIDvalue = 40 * error_difference + 10 * (integral / 20) + 150 * (error_difference - previousError);
  previousError = error_difference;

  float PIDvalue_amplification = error_amplification / 10;

  if (d1 + d2 >= 0) {
    setMotor((255 + PIDvalue) * PIDvalue_amplification, (255 - PIDvalue) * PIDvalue_amplification);
  } else {
    setMotor((-255 + PIDvalue) * PIDvalue_amplification, (-255 - PIDvalue) * PIDvalue_amplification);
  }
}


void moveForward(int cell) {
  encoder2Position = 0;
  int target = cell * step - step * step_coefficient / cell;

  int pid_flow_front_confirmation = 0;
  int approximate_location;
  int save_approximate_location;
  
  left = 0;
  up = 0;
  right = 0;

  int before_location_x = location_maze_x;
  int before_location_y = location_maze_y;
  int length_limit;
  int oke;

  if (condition_after_turn) {
    target -= step * 0.4;
    length_limit = step * 0.4;
  }


  while (encoder2Position < target && !change_maze) {
    int16_t distance1 = readSensor(0) / 10 - 13;
    int16_t distance6 = readSensor(5) / 10 - 10;
    int16_t distance3 = readSensor(2) / 10 - 10;
    int16_t distance4 = readSensor(3) / 10 - 20;
    int16_t distance2 = readSensor(1) / 10 - 11;
    int16_t distance5 = readSensor(4) / 10 - 11;

    approximate_location = (encoder2Position + length_limit + step / 2) / step;

    oke = (encoder2Position + length_limit) / step;
    Serial.println(oke);

    if (save_approximate_location != oke && oke > 0) {
      Serial.print(oke);
      if (abs(left) < 2 && abs(right) < 2) {
        if (distance3 > length * 0.5) {
          left++;
        } else {
          left--;
        }

        if (distance4 > length * 0.5) {
          right++;
        } else {
          right--;
        }
      } else {
        save_approximate_location = oke;
        save_change_maze(before_location_x, before_location_y, approximate_location, left, 10, right);

        left = 0;
        up = 0;
        right = 0;
      }
    }




    if (distance1 <= 5 && distance6 <= 5 && encoder2Position > step / 4 && encoder2Position < target - step / 2) {
      pid_flow_front(distance1, distance6);

      if (distance1 <= 1 && distance6 <= 1) {
        pid_flow_front_confirmation++;
        if (pid_flow_front_confirmation > 50) {
          setMotor(0, 0);
          //delay(500);
          save_change_maze(before_location_x, before_location_y, approximate_location, left, -10, right);
          change_maze = true;
        }
      }
    } else {
      pid_flow_front_confirmation = 0;
      int16_t distance2 = readSensor(1) / 10 - 11;
      int16_t distance5 = readSensor(4) / 10 - 11;

      int16_t distance1 = readSensor(0) / 10 - 17;
      int16_t distance6 = readSensor(5) / 10 - 14;
      if (distance1 < 1 && distance6 < 1 && encoder2Position > (target - step * 0.3)) {
        encoder2Position = target;
        approximate_location = (encoder2Position + length_limit + step / 2) / step;
        //delay (2000);
      }


      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      if (distance2 <= 2 || distance5 <= 2) {

        pid_flow_side(distance2, distance5);

      } else {
        setMotor(255, 255);
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
  }



  setMotor(0, 0);

  left = 0;
  up = 0;
  right = 0;
  while (abs(left) < 2 && abs(up) < 2 && abs(right) < 2) {
    int16_t distance3 = readSensor(2) / 10 - 10;
    int16_t distance4 = readSensor(3) / 10 - 20;
    int16_t distance1 = readSensor(0) / 10 - 17;
    int16_t distance6 = readSensor(5) / 10 - 14;

    if (distance3 > length * 0.5) {
      left++;
    } else {
      left--;
    }

    if (distance1 < length * 0.3 && distance6 < length * 0.3) {
      up--;
    } else {
      up++;
    }

    if (distance4 > length * 0.5) {
      right++;
    } else {
      right--;
    }
  }

  save_change_maze(before_location_x, before_location_y, approximate_location, left, up, right);


  condition_after_turn = false;
  //delay(500);
}



//   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void save_change_maze(int before_x, int before_y, int stepped, int left, int up, int right) {
  if (startDirection == 0) {  //Trường hợp hướng lên trên
    location_maze_x = before_x - 2 * stepped;
    if (left > 0) {
      maze[location_maze_x][location_maze_y - 1] = 0;
    } else if (left < 0) {
      maze[location_maze_x][location_maze_y - 1] = 1;
    }

    if (up > 0) {
      maze[location_maze_x - 1][location_maze_y] = 0;
    } else if (up < 0) {
      maze[location_maze_x - 1][location_maze_y] = 1;
    }

    if (right > 0) {
      maze[location_maze_x][location_maze_y + 1] = 0;
    } else if (right < 0) {
      maze[location_maze_x][location_maze_y + 1] = 1;
    }
  }
  if (startDirection == 1) {  //Trường hợp hướng sang phải
    location_maze_y = before_y + 2 * stepped;
    if (left > 0) {
      maze[location_maze_x - 1][location_maze_y] = 0;
    } else if (left < 0) {
      maze[location_maze_x - 1][location_maze_y] = 1;
    }

    if (up > 0) {
      maze[location_maze_x][location_maze_y + 1] = 0;
    } else if (up < 0) {
      maze[location_maze_x][location_maze_y + 1] = 1;
    }

    if (right > 0) {
      maze[location_maze_x + 1][location_maze_y] = 0;
    } else if (right < 0) {
      maze[location_maze_x + 1][location_maze_y] = 1;
    }
  }
  if (startDirection == 2) {  //Trường hợp hướng xuống dưới
    location_maze_x = before_x + 2 * stepped;
    if (left > 0) {
      maze[location_maze_x][location_maze_y + 1] = 0;
    } else if (left < 0) {
      maze[location_maze_x][location_maze_y + 1] = 1;
    }

    if (up > 0) {
      maze[location_maze_x + 1][location_maze_y] = 0;
    } else if (up < 0) {
      maze[location_maze_x + 1][location_maze_y] = 1;
    }

    if (right > 0) {
      maze[location_maze_x][location_maze_y - 1] = 0;
    } else if (right < 0) {
      maze[location_maze_x][location_maze_y - 1] = 1;
    }
  }
  if (startDirection == 3) {  //Trường hợp hướng sang trái
    location_maze_y = before_y - 2 * stepped;
    if (left > 0) {
      maze[location_maze_x + 1][location_maze_y] = 0;
    } else if (left < 0) {
      maze[location_maze_x + 1][location_maze_y] = 1;
    }

    if (up > 0) {
      maze[location_maze_x][location_maze_y - 1] = 0;
    } else if (up < 0) {
      maze[location_maze_x][location_maze_y - 1] = 1;
    }

    if (right > 0) {
      maze[location_maze_x - 1][location_maze_y] = 0;
    } else if (right < 0) {
      maze[location_maze_x - 1][location_maze_y] = 1;
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("{" + String(location_maze_x) + ", " + String(location_maze_y) + "} " + String(startDirection) + ", " + String(left) + ", " + String(up) + ", " + String(right));
  display.display();


  //setMotor(0, 0);
  // Gửi dữ liệu qua UDP
  // udp.beginPacket(udpAddress, udpPort);
  // udp.print("{" + String(location_maze_x) + ", " + String(location_maze_y) + "} " + String(startDirection) + ", " + String(left) + ", " + String(up) + ", " + String(right));
  // udp.endPacket();
  //delay(100);
}

//   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// Hàm điều khiển động cơ
void setMotor(int pwmA, int pwmB) {
  pwmA = constrain(pwmA * efficiency, -255, 255);
  pwmB = constrain(pwmB * efficiency, -255, 255);
  // --- Motor A ---
  if (pwmA > 0) {
    analogWrite(IN1, pwmA);
    analogWrite(IN2, 0);
  } else if (pwmA < 0) {
    analogWrite(IN1, 0);
    analogWrite(IN2, -pwmA);
  } else {
    analogWrite(IN1, 0);
    analogWrite(IN2, 0);
  }

  // --- Motor B ---
  if (pwmB > 0) {
    analogWrite(IN3, pwmB);
    analogWrite(IN4, 0);
  } else if (pwmB < 0) {
    analogWrite(IN3, 0);
    analogWrite(IN4, -pwmB);
  } else {
    analogWrite(IN3, 0);
    analogWrite(IN4, 0);
  }
}


//   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void turnOneMotor(int steps, int isMotor) {
  encoder1Position = 0;
  encoder2Position = 0;
  if (isMotor == 1) {
    setMotor(0, 255);  // Chỉ động cơ phải quay
    while (abs(encoder2Position) < steps * turn)
      ;
  } else if (isMotor == 2) {
    setMotor(255, -255);  // Chỉ động cơ trái quay
    while (abs(encoder1Position) < steps || abs(encoder2Position) < steps)
      ;
  } else if (isMotor == 3) {
    setMotor(255, 0);  // Chỉ động cơ trái quay
    while (abs(encoder1Position) < steps * turn)
      ;
  }
  setMotor(0, 0);
  //delay(500);
}


//   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// ------------------- khối vẽ mê xung lên màn hình -------------------
void draw_maze() {
  display.clearDisplay();
  for (int row = 0; row < VIEWPORT_HEIGHT; row++) {
    int mazeRow = row + offsetY;
    if (mazeRow >= MAZE_SIZE) break;
    for (int col = 0; col < MAZE_SIZE; col++) {
      int x = col * CELL_SIZE;
      int y = row * CELL_SIZE;
      if (mazeRow == location_maze_x && col == location_maze_y) {
        display.fillRect(x, y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
        display.fillRect(x + 1, y + 1, 2, 2, SSD1306_BLACK);
      } else if (maze[mazeRow][col] == 0) {
        display.fillRect(x, y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
      } else if (maze[mazeRow][col] == 1) {
        display.fillRect(x, y, CELL_SIZE, CELL_SIZE, SSD1306_BLACK);
      } else if (maze[mazeRow][col] == 2) {
        display.fillRect(x, y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
        display.fillRect(x + 1, y + 1, 2, 2, SSD1306_BLACK);
      } else if (maze[mazeRow][col] == 3) {
        display.fillRect(x, y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
        display.drawLine(x, y, x + CELL_SIZE, y + CELL_SIZE, SSD1306_BLACK);  // Đường chéo
        display.drawLine(x + CELL_SIZE, y, x, y + CELL_SIZE, SSD1306_BLACK);  // Đường chéo ngược
      }
    }
  }
  display.display();
}

// ------------------- khối di chuyển mê cung trên màn hình -------------------
void move_draw_maze() {
  if (abs(encoder2Position) > 7) {
    offsetY += (encoder2Position / 7);
    if (offsetY < 0) offsetY = 0;
    if (offsetY > MAZE_SIZE - VIEWPORT_HEIGHT) offsetY = MAZE_SIZE - VIEWPORT_HEIGHT;
    encoder2Position = 0;
    draw_maze();
  }
}



// ------------------- Phần cảm biến -------------------
// Hàm chọn kênh của TCA9548A
void tca9548a_select(uint8_t channel) {
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);  // Chọn kênh
  Wire.endTransmission();
}

// Hàm khởi tạo một cảm biến
bool initSensor(uint8_t index) {
  tca9548a_select(sensorChannels[index]);  // Chọn kênh của cảm biến
  if (!sensors[index].init()) {
    Serial.print("Không khởi tạo được cảm biến ");
    Serial.println(index + 1);
    return false;
  }
  sensors[index].setMeasurementTimingBudget(20000);  // 20ms mỗi lần đo
  sensors[index].startContinuous();
  Serial.print("Cảm biến ");
  Serial.print(index + 1);
  Serial.println(" khởi tạo thành công");
  return true;
}

//Hàm đọc dữ liệu từ một cảm biến
// uint16_t readSensor(uint8_t index) {
//   tca9548a_select(sensorChannels[index]);  // Chọn kênh của cảm biến
//   int16_t distance = (sensors[index].readRangeContinuousMillimeters()) / 10 - offsets[index];
//   return distance;
// }

// Hàm đọc dữ liệu từ một cảm biến
uint16_t readSensor(uint8_t index) {
  tca9548a_select(sensorChannels[index]);  // Chọn kênh của cảm biến
  uint16_t distance = sensors[index].readRangeContinuousMillimeters();
  if (sensors[index].timeoutOccurred()) {
    Serial.print("Lỗi cảm biến ");
    Serial.print(index + 1);
    Serial.println("!");
  }
  return distance;
}

// ------------------- Phần reset cảm biến -------------------
void resetSensors() {
  int resetCount = 0;
  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    if (initSensor(i)) {
      resetCount++;
    }
  }

  // Hiển thị số lượng cảm biến đã reset thành công
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Reset success: ");
  display.print(resetCount);
  display.print("/6");

  display.display();
}

// ------------------- Phần menu -------------------
void renderMenu() {
  display.clearDisplay();
  display.setTextSize(1);

  if (inSubmenu) {

  } else {
    const char* menuItems[] = {
      "Run and save",
      "Danh sach 2",
      "Luu offset - hien thi so",  //lưu giá trị mà cảm biến lấy làm mốc sau đó hiển thị giá trị cảm biến ở dạng số
      "Reset cam bien",            //Reset cảm biến
      "Show Map",                  //hiển thị map
      "step value",
      "step coefficient",
      "turn value",
      "rotation coefficient"

    };

    // Vẽ danh sách menu chính
    for (int i = 0; i < visibleItems; i++) {
      int itemIndex = (scrollOffset + i) % totalItems;  // Tính mục hiển thị
      int y = i * 8;                                    // Vị trí Y cho mỗi mục

      if (i == currentItem - scrollOffset) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);      // Chữ đen nền trắng
        display.fillRect(0, y, SCREEN_WIDTH, 8, SSD1306_WHITE);  // Vẽ hình chữ nhật
      } else {
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);  // Chữ trắng nền đen
      }
      display.setCursor(0, i * 8);
      display.print(menuItems[itemIndex]);
    }
  }

  display.display();
}



// ------------------- Phần hiển thị dạng số -------------------
void numberDisplay() {
  display.clearDisplay();

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    int16_t distance = readSensor(i);

    // Hiển thị dạng 2 cột dọc
    uint8_t column = i < 3 ? 0 : 64;  // Cột 1 cho cảm biến 0-2, cột 2 cho cảm biến 3-5
    uint8_t row = (i % 3) * 8;

    display.setCursor(column, row);
    display.print("S");
    display.print(i + 1);
    display.print(": ");
    display.print(distance);
    display.print(" cm");
  }

  display.display();
}


// ------------------- Phần sử lý encoder để điều hướng trong menu -------------------
void handleEncoderInput() {

  // Nếu giá trị encoder thay đổi
  if (abs(encoder2Position) > 7 && !inSubmenu) {
    if (encoder2Position < 0) {  // Cuộn xuống
      if (currentItem < totalItems - 1) {
        currentItem++;
        if (currentItem >= scrollOffset + visibleItems) {
          scrollOffset++;
        }
      }
    } else {  // Cuộn lên
      if (currentItem > 0) {
        currentItem--;
        if (currentItem < scrollOffset) {
          scrollOffset--;
        }
      }
    }

    encoder2Position = 0;
    renderMenu();
  }
}


// ------------------- Phần lưu giá trị offset -------------------
void saveOffset() {
  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    tca9548a_select(sensorChannels[i]);
    uint16_t distance = sensors[i].readRangeContinuousMillimeters();
    offsets[i] = distance / 10;
  }

  // Hiển thị giá trị offset đã lưu
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Offset saved:");

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    uint8_t column = i < 3 ? 0 : 64;
    uint8_t row = (i % 3) * 8 + 10;

    display.setCursor(column, row);
    display.print("S");
    display.print(i + 1);
    display.print(": ");
    display.print(offsets[i]);
    display.print(" cm");
  }

  display.display();
}


// ------------------- Phần sử lý quản lý giá trị trong menu -------------------
void step_value(uint8_t value) {

  // Nếu giá trị encoder thay đổi
  if (abs(encoder2Position) > 7) {
    if (value == 5) {
      step += int(encoder2Position / 7 * 10);
    } else if (value == 6) {
      step_coefficient += float(encoder2Position / 7 * 0.025);
    } else if (value == 7) {
      turn += float(encoder2Position / 7 * 0.05);
    } else if (value == 8) {
      rotation_coefficient += float(encoder2Position / 7 * 0.05);
    }

    encoder2Position = 0;
    // Hiển thị số lượng cảm biến đã reset thành công
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    if (value == 5) {
      display.print("step value: ");
      display.print(step);
    } else if (value == 6) {
      display.print("step coefficient: ");
      display.print(step_coefficient);
    } else if (value == 7) {
      display.print("turn value: ");
      display.print(turn);
    } else if (value == 8) {
      display.print("rotation coefficient: ");
      display.print(rotation_coefficient);
    }

    display.display();
  }
}


// ------------------- Phần sử lý nút ấn -------------------
void handleButtonInput() {
  if (digitalRead(BUTTON_SELECT) == LOW) {
    delay(200);
    if (!inSubmenu) {
      inSubmenu = true;
      if (currentItem == 0) {
        run_and_save();
      }
      if (currentItem == 2) {
        saveOffset();  // Lưu giá trị offset và hiển thị
      } else if (currentItem == 3) {
        resetSensors();  // Reset cảm biến và hiển thị kết quả
      } else if (currentItem == 4) {
        draw_maze();
      } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        if (currentItem == 5) {
          display.print("step value: ");
          display.print(step);
        } else if (currentItem == 6) {
          display.print("step coefficient: ");
          display.print(step_coefficient);
        } else if (currentItem == 7) {
          display.print("turn value: ");
          display.print(turn);
        } else if (currentItem == 8) {
          display.print("rotation coefficient: ");
          display.print(rotation_coefficient);
        }

        display.display();
      }
    }
  }

  if (inSubmenu) {
    if (currentItem == 2) {
      numberDisplay();
    }


    if (currentItem == 4) {
      move_draw_maze();  // Hiển thị map lên màn hình
    }

    if (currentItem >= 5) {
      step_value(currentItem);
    }
  }

  if (digitalRead(BUTTON_BACK) == LOW) {
    delay(200);
    if (inSubmenu) {
      inSubmenu = false;
      renderMenu();
    } else {  // khi đã ở menu mà vẫn ấn nút back thì in ra đường đi giải mê cung
      initializeMazeSolver(location_maze_x, location_maze_y);
      Serial.println("987648765432");

      //delay(10);
      for (int i = 0; i < pathIndex; i++) {
        Serial.print("{" + String(path[i].steps) + ", " + String(path[i].action) + "}");
        Serial.println();
      }
      Serial.println("9876543");
    }
  }

  if (!inSubmenu) {
    handleEncoderInput();
  }
}

// ------------------- Phần setup và vòng lặp chính -------------------
void setup() {
  Serial.begin(115200);
  Wire.begin(9, 10);
  Wire.setClock(400000);



  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("\nWiFi connected.");
  // Serial.print("ESP32 IP Address: ");
  // Serial.println(WiFi.localIP());



  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  // Thiết lập chân encoder
  pinMode(ENCODER1_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER1_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER2_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER2_PIN_B, INPUT_PULLUP);

  // Gắn ngắt vào encoder
  attachInterrupt(digitalPinToInterrupt(ENCODER1_PIN_A), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER2_PIN_A), encoder2ISR, CHANGE);



  pinMode(BUTTON_SELECT, INPUT_PULLUP);  // Khai báo lại nút SELECT
  pinMode(BUTTON_BACK, INPUT_PULLUP);    // Khai báo lại nút BACK

  pinMode(8, OUTPUT);  // Khai báo lại nút BACK
  digitalWrite(8, LOW);



  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  // Trên ESP32, analogWrite là PWM qua LEDC
  // Ta set tần số PWM cho 4 kênh
  ledcAttach(IN1, 5000, 8);  // chân, tần số, độ phân giải 8bit
  ledcAttach(IN2, 5000, 8);
  ledcAttach(IN3, 5000, 8);
  ledcAttach(IN4, 5000, 8);


  resetSensors();
  renderMenu();
}

void run_and_save() {
  delay(1000);
  obtain = false;
  while (!obtain) {
    obtain = true;

    if (location_maze_x == destination_maze_x1 && location_maze_y == destination_maze_y1) {
      maze[destination_maze_x1][destination_maze_y1] = 2;
      maze[destination_maze_x2][destination_maze_y2] = 3;
    }
    if (location_maze_x == destination_maze_x2 && location_maze_y == destination_maze_y2) {
      maze[destination_maze_x2][destination_maze_y2] = 2;
      maze[destination_maze_x1][destination_maze_y1] = 3;
    }

    while (maze[location_maze_x][location_maze_y] != 3) {

      initializeMazeSolver(location_maze_x, location_maze_y);
      //delay(1200);
      for (int i = 0; i < pathIndex; i++) {



        if (path[i].steps != 0) {
          moveForward((path[i].steps) / 2);
          //delay (500);
        } else {


          if (path[i].action == 1) {
            if (left > 0) {
              turnOneMotor(540, 1);  // Chỉ động cơ trái quay thêm 300 bước
              startDirection = (startDirection + 3) % 4;
              condition_after_turn = true;
            } else {
              change_maze = true;
              save_change_maze(location_maze_x, location_maze_y, 0, -10, 0, 0);
            }

          } else if (path[i].action == 2) {
            setMotor(0, 0);
            delay(200);
            turnOneMotor(540 * rotation_coefficient, 2);
            setMotor(0, 0);
            delay(200);
            setMotor(-255, -255);
            delay(500);
            startDirection = (startDirection + 2) % 4;

          } else if (path[i].action == 3) {
            if (right > 0) {
              turnOneMotor(540, 3);  // Chỉ động cơ phải quay thêm 300 bước
              startDirection = (startDirection + 1) % 4;
              condition_after_turn = true;
            } else {
              change_maze = true;
              save_change_maze(location_maze_x, location_maze_y, 0, 0, 0, -10);
            }
          }
        }

        if (change_maze) {
          i = pathIndex;
          change_maze = false;
          obtain = false;
        }
      }
    }
    delay(2000);
  }
}

void loop() {

  handleButtonInput();
}
