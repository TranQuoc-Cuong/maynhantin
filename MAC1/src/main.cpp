#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <Preferences.h>
#include <esp_now.h>
#include <WiFi.h>


Preferences preferences;

#define btn_right 16
#define btn_left 4
#define btn_top 15
#define btn_down 5
#define btn_select 17

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t broadcastAddress[] = {0x88, 0x13, 0xbf, 0x70, 0x70, 0x98}; //com5
//uint8_t broadcastAddress[] = {0x48, 0xe7, 0x29, 0xa1, 0x8c, 0x1c};; // com3

unsigned long lastActivityTime = 0;  // Thời điểm cuối cùng có hoạt động
const unsigned long screenTimeout = 60000;  // Thời gian chờ tắt màn hình (10 giây)
bool screenOn = true;  // Trạng thái màn hình (bật/tắt)

String datafromrecive;
String datasentdevice;

String success;

bool message_sent = false;

int8_t colum = 0, row = 0; 

bool change = false;
bool check_after_send = false, send_success = false;

esp_now_peer_info_t peerInfo;

// variable of text

const uint8_t MAX_TEXT_LENGTH = 22;

String resultStr;

const char code_keyboard[4][11] = {
  {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k'},
  {'l', 'm', 'n', 'o', 'p', 'q', 'r', 's','t', 'u', '<'},
  {'v', 'w', 'x', 'y', 'z', ',', '.', '\'', ' ', ' ', ' '},
  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  '$'}// Thêm ký tự trắng
};

uint8_t number = 0;

char selected_char = ' ';

String text_send_0 = "";
String text_send_1 = "";
String text_send_2 = "";
String text_send_3 = "";
String text_send_4 = "";

void turnOffScreen() {
  if (screenOn) {
    display.clearDisplay();
    display.display();
    screenOn = false;
    Serial.println("Screen turned off");
  }
}

void turnOnScreen() {
  if (!screenOn) {
    screenOn = true;
    change = true;  // Cập nhật nội dung màn hình
    Serial.println("Screen turned on");
  }
  lastActivityTime = millis();  // Cập nhật thời gian hoạt động cuối cùng
}

void savePreferences() {
  preferences.begin("my-app", false); // Khởi tạo namespace "my-app"
  preferences.putString("text_0", text_send_0);
  preferences.putString("text_1", text_send_1);
  preferences.putString("text_2", text_send_2);
  preferences.putString("text_3", text_send_3);
  preferences.putString("text_4", text_send_4);
  preferences.end();
}

void loadPreferences() {
  preferences.begin("my-app", true); // Khởi tạo read-only
  text_send_0 = preferences.getString("text_0", ""); // Tham số thứ 2 là giá trị mặc định nếu không tìm thấy
  text_send_1 = preferences.getString("text_1", "");
  text_send_2 = preferences.getString("text_2", "");
  text_send_3 = preferences.getString("text_3", "");
  text_send_4 = preferences.getString("text_4", "");
  preferences.end();
}

void capnhatchuoi(String text)
{
  text_send_0 = text_send_1;
  text_send_1 = text_send_2;
  text_send_2 = text_send_3;
  text_send_3 = text_send_4;
  text_send_4 = text;
  savePreferences(); // Lưu các biến vào Flash sau khi cập nhật
}

void printRightAligned(String &text) {
  // Lấy độ rộng và chiều cao của chuỗi
  int16_t x, yTemp;
  uint16_t width, height;
  display.getTextBounds(text.c_str(), 0, 0, &x, &yTemp, &width, &height);
  // Tính toán số khoảng trắng cần thêm vào chuỗi để căn lề phải
  uint8_t numSpaces = (display.width() - width) / 6;
  // Thêm các khoảng trắng vào đầu chuỗi
  for (int i = 0; i < numSpaces; i++) {
    text = " " + text;  // Thêm một khoảng trắng vào đầu chuỗi
  }
}

typedef struct
{
  uint8_t button_last;
  uint8_t is_debouncing, button_current, sta_button, button_filter, is_press_timeout;
  uint32_t time_debounce, time_start_press;
  uint8_t Pin;
}Button_typedef;

Button_typedef button_top;
Button_typedef button_down;
Button_typedef button_right;
Button_typedef button_left;
Button_typedef button_select;

void capnhatmanhinh()
{
  display.clearDisplay();
  display.drawLine(0, 55, 128,  55, SH110X_WHITE);
  display.setCursor(0, 56);
  
  for(int i = 0; i < 11; i++)
  {
    display.setCursor(i * 12, 56);
    display.print(code_keyboard[row][i]);
  }
  display.drawRect(colum * 12, 55, 8, 9, SH110X_WHITE);

  display.setCursor(0, 0);
  display.println(text_send_0);

  display.setCursor(0, 8);
  display.println(text_send_1);

  display.setCursor(0, 16);
  display.println(text_send_2);

  display.setCursor(0, 24);
  display.println(text_send_3);

  display.setCursor(0, 32);
  display.println(text_send_4);

  display.setCursor(0, 40);
  printRightAligned(resultStr);
  display.println(resultStr);

  display.setCursor(0, 48);
  display.println(datasentdevice);

  display.display();
}

void button_pressing_callback(Button_typedef *ButtonX)
{
  if (!screenOn) {
    turnOnScreen();
  }

  change = true;
  lastActivityTime = millis();

  change = true;
  if(ButtonX == &button_top)
  {
    row--;
  }
  if(ButtonX == &button_down)
  {
    row++;
  }
  if(ButtonX == &button_right)
  {
    colum++;
  }
  if(ButtonX == &button_left)
  {
    colum--;
  }
}

void button_press_short_callback(Button_typedef *ButtonX)
{
  
}

void button_release_callback(Button_typedef *ButtonX)
{
  change = true;
  if (!screenOn) {
    turnOnScreen();
    return;
  }

  lastActivityTime = millis();
  
  if(ButtonX == &button_select)
  {
    if(!message_sent){
    selected_char = code_keyboard[row][colum];
    if (selected_char == '<' && datasentdevice.length() > 0) 
    {
      datasentdevice.remove(datasentdevice.length() - 1);
      number--;
    } 
    else if (datasentdevice.length() + 1 < MAX_TEXT_LENGTH && selected_char != '<') 
    {
      datasentdevice += selected_char;
      number++;
    }
    String numberStr = String(number);
    resultStr = numberStr + "/21";
    }
    message_sent = false; // Reset biến đánh dấu
  }
  capnhatmanhinh();
}

void button_press_timeout_callback(Button_typedef *ButtonX)
{
  change = true;

  if (!screenOn) {
    turnOnScreen();
    return;  // Không thực hiện thao tác nếu màn hình đang tắt
  }
  lastActivityTime = millis();

  if(ButtonX == &button_select)
  {
    message_sent = true;
    uint8_t* dataBuffer = new uint8_t[datasentdevice.length() + 1];
    memcpy(dataBuffer, datasentdevice.c_str(), datasentdevice.length());
    dataBuffer[datasentdevice.length()] = '\0';  // Thêm null terminator
    esp_err_t result = esp_now_send(broadcastAddress, dataBuffer, datasentdevice.length() + 1);
    delete[] dataBuffer;  // Giải phóng bộ nhớ sau khi gửi
    if (result == ESP_OK) {
      // Gửi thành công, nhưng đợi callback để xác nhận
      String numberStr = String(number);
      resultStr = numberStr + "/21";
      capnhatchuoi(datasentdevice);
      Serial.println("Sent with success");
    } else {
      Serial.println("Error sending the data");
      // Hiển thị thông báo lỗi
      text_send_4 = ". . .";
      change = true;
    }
  }
  if(ButtonX == &button_top)
  {
    preferences.begin("my-app", false);
    preferences.clear(); // Xóa tất cả dữ liệu trong namespace
    preferences.end();
    
    // Reset các biến text
    text_send_0 = "";
    text_send_1 = "";
    text_send_2 = "";
    text_send_3 = "";
    text_send_4 = "";
  }
  check_after_send = true;
}

void button_handle(Button_typedef *ButtonX)
{
    // Đọc trạng thái nút hiện tại
    ButtonX->sta_button = digitalRead(ButtonX->Pin);
    
    //--xử lý nhiễu (debounce)--
    if(ButtonX->button_filter != ButtonX->sta_button)
    {
        ButtonX->button_filter = ButtonX->sta_button;
        ButtonX->is_debouncing = 1;
        ButtonX->time_debounce = millis();
    }
    
    //--tín hiệu xác lập sau debounce
    if(ButtonX->is_debouncing && (millis() - ButtonX->time_debounce) >= 15)
    {
        ButtonX->button_current = ButtonX->button_filter;
        ButtonX->is_debouncing = 0;
    }

    //--xử lý sự kiện khi trạng thái nút thay đổi
    if(ButtonX->button_current != ButtonX->button_last)
    {
        if(ButtonX->button_current == 0) // nhấn nút (LOW là nhấn)
        {
            ButtonX->is_press_timeout = 1;
            ButtonX->time_start_press = millis();
            button_pressing_callback(ButtonX);  // Gọi callback khi nút được nhấn
        }
        else // nhả nút
        {
            if(millis() - ButtonX->time_start_press <= 100)
            {
                button_press_short_callback(ButtonX);  // Gọi callback nhấn ngắn
            }
            button_release_callback(ButtonX);  // Gọi callback khi nút được nhả
            ButtonX->is_press_timeout = 0;
        }
        
        ButtonX->button_last = ButtonX->button_current;  // Cập nhật trạng thái nút cuối
    }
    
    //--xử lý nhấn giữ (di chuyển ra ngoài để kiểm tra liên tục)
    if(ButtonX->is_press_timeout && (millis() - ButtonX->time_start_press >= 1200))
    {
        button_press_timeout_callback(ButtonX);  // Gọi callback khi nhấn giữ đủ lâu
        ButtonX->is_press_timeout = 0;  // Tránh gọi lại callback nhiều lần
    }
}

void button_init(Button_typedef *ButtonX, uint8_t Pin)
{
    ButtonX->Pin = Pin;
    pinMode(Pin, INPUT);  // Thiết lập chân làm đầu vào
    
    // Khởi tạo các biến
    ButtonX->button_last = 1;  // Mặc định là HIGH (không nhấn)
    ButtonX->button_current = 1;
    ButtonX->sta_button = 1;
    ButtonX->button_filter = 1;
    ButtonX->is_debouncing = 0;
    ButtonX->is_press_timeout = 0;
    ButtonX->time_debounce = 0;
    ButtonX->time_start_press = 0;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) 
  {
    Serial.println("Delivery success");
    send_success = true;
    // Chỉ lưu tin nhắn vào lịch sử khi gửi thành công
    text_send_4 = datasentdevice;
    // Reset dữ liệu gửi
    datasentdevice = "";
    number = 0;
    // Cập nhật hiển thị

    String numberStr = String(number);
    resultStr = numberStr + "/21";
    change = true;
    capnhatmanhinh();
  } 
  else 
  {
    Serial.println("Delivery fail");
    send_success = false;
    // Hiển thị thông báo lỗi
    change = true;
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  change = true;
  lastActivityTime = millis();
  if (!screenOn) {
    turnOnScreen();
  }
  char* buffer = new char[len + 1];
  memcpy(buffer, incomingData, len);
  buffer[len] = '\0';  // Đảm bảo có null terminator

  datafromrecive = String(buffer);
  delete[] buffer;

  if (datafromrecive.length() > 21) {
    datafromrecive = datafromrecive.substring(0, 21);
  }
  Serial.println(datafromrecive);
  printRightAligned(datafromrecive);
  capnhatchuoi(datafromrecive);
  capnhatmanhinh();
}

void setup()   {
  Serial.begin(115200);

  button_init(&button_top, btn_top);
  button_init(&button_down, btn_down);
  button_init(&button_right, btn_right);
  button_init(&button_left, btn_left);
  button_init(&button_select, btn_select);

  //code ESP NOW
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW");
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  display.begin(i2c_Address, true); // Address 0x3C default
  display.clearDisplay();

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  lastActivityTime = millis();
  screenOn = true;

  loadPreferences();

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  String numberStr = String(number);
  resultStr = numberStr + "/21";
  printRightAligned(resultStr);
  display.display();
}

void loop() {
  button_handle(&button_top);
  button_handle(&button_down);
  button_handle(&button_right);
  button_handle(&button_left);
  button_handle(&button_select);

  if (screenOn && (millis() - lastActivityTime > screenTimeout)) {
    turnOffScreen();
  }

  if(colum < 0) 
  {
    colum = 10;
    row--;
  }
  if(colum > 10) 
  {
    colum = 0;
    row++;
  }
  if(row < 0)
  {
    row = 3;
  }
  if(row > 3)
  {
    row = 0;
  }

  if(change == true)
  {
    capnhatmanhinh();
    change = false;
  }
}

