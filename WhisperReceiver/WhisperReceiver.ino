#include <SoftwareSerial.h>

SoftwareSerial BTserial(2, 3);

int motorPins[] = {6, 7, 8, 9, 10, 11};
const int numMotors = 6;
int symbolDuration = 800;
int letterPause = 300;
const int symbolDurationBase = 800;
const int letterPauseBase = 300;

// Структура для символов Брайля (Unicode)
struct BrailleLetter {
  uint16_t unicode;
  bool dots[6];
};

// Таблица Брайля для русских букв (Unicode коды UTF-8)
BrailleLetter brailleAlphabet[] = {
  // Базовые буквы
  {0x0410, {1,0,0,0,0,0}}, // А 
  {0x0411, {1,1,0,0,0,0}}, // Б 
  {0x0412, {0,1,0,1,1,1}}, // В 
  {0x0413, {1,1,0,1,1,0}}, // Г 
  {0x0414, {1,0,0,1,1,0}}, // Д 
  {0x0415, {1,0,0,0,1,0}}, // Е 
  {0x0401, {1,0,0,0,0,1}}, // Ё 
  {0x0416, {0,1,0,1,1,0}}, // Ж
  {0x0417, {1,0,1,0,1,1}}, // З
  {0x0418, {0,1,0,1,0,0}}, // И
  {0x0419, {1,1,1,1,0,1}}, // Й
  {0x041A, {1,0,1,0,0,0}}, // К
  {0x041B, {1,1,1,0,0,0}}, // Л
  {0x041C, {1,0,1,1,0,0}}, // М
  {0x041D, {1,0,1,1,1,0}}, // Н 
  {0x041E, {1,0,1,0,1,0}}, // О
  {0x041F, {1,1,1,1,0,0}}, // П
  {0x0420, {1,1,1,0,1,0}}, // Р
  {0x0421, {0,1,1,1,0,0}}, // С
  {0x0422, {0,1,1,1,1,0}}, // Т
  {0x0423, {1,0,1,0,0,1}}, // У
  {0x0424, {1,1,0,1,0,0}}, // Ф
  {0x0425, {1,1,0,0,1,0}}, // Х
  {0x0426, {1,0,0,1,0,0}}, // Ц
  {0x0427, {1,1,1,1,1,0}}, // Ч
  {0x0428, {1,0,0,0,1,1}}, // Ш
  {0x0429, {1,0,1,1,0,1}}, // Щ
  {0x042A, {1,1,1,0,1,1}}, // Ъ
  {0x042B, {0,1,1,1,0,1}}, // Ы
  {0x042C, {0,1,1,1,1,1}}, // Ь
  {0x042D, {0,1,0,1,0,1}}, // Э
  {0x042E, {1,1,0,0,1,1}}, // Ю
  {0x042F, {1,1,0,1,0,1}}, // Я

  // Специальные символы
  {0x0020, {0,0,0,0,0,0}}, // Пробел
  {0x002E, {0,1,0,0,1,1}}, // Точка (.)
  {0x002C, {0,1,0,0,0,0}}, // Запятая (,)
};

#define MAX_MESSAGE_LENGTH 200
char messageBuffer[MAX_MESSAGE_LENGTH + 1];
int messageIndex = 0;

void setup() {
  Serial.begin(9600);
  BTserial.begin(9600);

  while(BTserial.available() > 0) {
    BTserial.read();
  }
  
  for(int i = 0; i < numMotors; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }
  Serial.println("Устройство инициализировано!");
}

void loop() {
  receiveBluetoothData();
}

void receiveBluetoothData() {
  while(BTserial.available() > 0) {
    char c = BTserial.read();
    
    if(c == '}' || messageIndex >= MAX_MESSAGE_LENGTH) {
      messageBuffer[messageIndex] = c;
      messageBuffer[messageIndex + 1] = '\0';
      
      Serial.println("\n--- Получено новое сообщение ---");
      
      processMessage(messageBuffer);
      messageIndex = 0;
      break;
    } else {
      messageBuffer[messageIndex++] = c;
    }
  }
}

void processMessage(const char* msg) {
  char text[100] = {0};
  char speedStr[10] = {0};
  
  if(parseJSON(msg, "text", text, sizeof(text)) && 
     parseJSON(msg, "speed", speedStr, sizeof(speedStr))) {
    float speedMultiplier = constrain(atof(speedStr), 0.5, 1.5);
    speedMultiplier = 2 - speedMultiplier;
    symbolDuration = symbolDurationBase * speedMultiplier;
    letterPause = letterPauseBase * speedMultiplier;

    Serial.print("Текст: ");
    Serial.println(text);
    Serial.print("Скорость: ");
    Serial.println(symbolDuration);
    
    processAndPrintUnicode(text);
  }
}

bool parseJSON(const char* input, const char* key, char* output, size_t maxLen) {
  char pattern[20];
  snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
  const char* start = strstr(input, pattern);
  
  if(!start) return false;
  start += strlen(pattern);
  
  const char* end = strchr(start, '"');
  if(!end || (end - start) >= maxLen) return false;
  
  strncpy(output, start, end - start);
  output[end - start] = '\0';
  return true;
}

void processAndPrintUnicode(const char* text) {
  const uint8_t* ptr = (const uint8_t*)text;
  size_t bytePosition = 0;
  
  while(*ptr) {
    uint32_t unicode = 0;
    size_t bytesRead = decodeUTF8(ptr, &unicode);
    if(unicode >= 0x0430 && unicode <= 0x044F) {
      unicode -= 0x20;
    }
    
    activateBrailleSymbol(unicode);
    
    ptr += bytesRead;
    bytePosition += bytesRead;
  }
}

size_t decodeUTF8(const uint8_t* bytes, uint32_t* unicode) {
  if(bytes[0] <= 0x7F) {
    *unicode = bytes[0];
    return 1;
  }
  else if((bytes[0] & 0xE0) == 0xC0) {
    *unicode = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
    return 2;
  }
  else if((bytes[0] & 0xF0) == 0xE0) {
    *unicode = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
    return 3;
  }
  return 0;
}

void activateBrailleSymbol(uint32_t unicode) {
  for(size_t i = 0; i < sizeof(brailleAlphabet)/sizeof(BrailleLetter); i++) {
    if(brailleAlphabet[i].unicode == unicode) {
      activateMotors(brailleAlphabet[i]);
      return;
    }
  }
  Serial.println("Символ не найден!");
}

void activateMotors(BrailleLetter letter) {
  for(int i = 0; i < numMotors; i++) {
    digitalWrite(motorPins[i], letter.dots[i] ? HIGH : LOW);
  }
  
  delay(constrain(symbolDuration, 400, 1200));
  
  for(int i = 0; i < numMotors; i++) {
    digitalWrite(motorPins[i], LOW);
  }
  delay(constrain(letterPause, 150, 450));
}