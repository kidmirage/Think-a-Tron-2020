#include "Adafruit_ADS1015.h"
#include "Adafruit_NeoPixel.h"
#include "ESPino32CAM.h"
#include "ESPino32CAM_QRCode.h"
#include "ShiftRegister74HC595.h"

// ADC module object.
Adafruit_ADS1115 ads1115;

// Definitions for the four 7-segment display score counters.
#define SDI 12
#define SCLK 13
#define LOAD 15
#define DIGITS 4

// Create shift register object (number of shift registers, data pin, clock pin, latch pin).
ShiftRegister74HC595<DIGITS> sr (SDI, SCLK, LOAD);

// Patterns for score counter digits.
uint8_t  digits[] = {B11000000, //0
                     B11111001, //1
                     B10100100, //2
                     B10110000, //3
                     B10011001, //4
                     B10010010, //5
                     B10000010, //6
                     B11111000, //7
                     B10000000, //8
                     B10010000  //9
                    };

// Camera objects.
ESPino32CAM cam;   // Image capture object
ESPino32QRCode qr; // Image decoding object

// Camera pins.
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Think-a-Tron Controls.
#define FLASH 4
#define SOUND 1

// Mappings tp ADS1115 ADC inputs.
#define QUESTION 0
#define P1_BUTTONS 1
#define P2_BUTTONS 2

// 5x7 LED Array.
#define NUM_LEDS 45
#define LEDS_PIN 3

// Create NeoPixel object.
Adafruit_NeoPixel pixels(NUM_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800);

// Pixel Offsets.
#define P1_OFFSET 5
#define P2_OFFSET 0
#define MATRIX_OFFSET 10

// Patterns for the 5x7 display.
int A[35] = {0, 0, 1, 1, 1, 1, 1,
             0, 1, 0, 0, 1, 0, 0,
             1, 0, 0, 0, 1, 0, 0,
             0, 1, 0, 0, 1, 0, 0,
             0, 0, 1, 1, 1, 1, 1
            };

int B[35] = {1, 1, 1, 1, 1, 1, 1,
             1, 0, 0, 1, 0, 0, 1,
             1, 0, 0, 1, 0, 0, 1,
             1, 0, 0, 1, 0, 0, 1,
             0, 1, 1, 0, 1, 1, 0
            };

int C[35] = {0, 1, 1, 1, 1, 1, 0,
             1, 0, 0, 0, 0, 0, 1,
             1, 0, 0, 0, 0, 0, 1,
             1, 0, 0, 0, 0, 0, 1,
             0, 1, 0, 0, 0, 1, 0
            };

int T[35] = {1, 0, 0, 0, 0, 0, 0,
             1, 0, 0, 0, 0, 0, 0,
             1, 1, 1, 1, 1, 1, 1,
             1, 0, 0, 0, 0, 0, 0,
             1, 0, 0, 0, 0, 0, 0
            };

int F[35] = {1, 1, 1, 1, 1, 1, 1,
             1, 0, 0, 1, 0, 0, 0,
             1, 0, 0, 1, 0, 0, 0,
             1, 0, 0, 1, 0, 0, 0,
             1, 0, 0, 0, 0, 0, 0
            };

int Q[35] = {0, 1, 0, 0, 0, 0, 0,
             1, 0, 0, 0, 0, 0, 0,
             1, 0, 0, 0, 1, 0, 1,
             1, 0, 0, 1, 0, 0, 0,
             0, 1, 1, 0, 0, 0, 0
            };

int X[35] = {1, 1, 0, 0, 0, 1, 1,
             0, 0, 1, 0, 1, 0, 0,
             0, 0, 0, 1, 0, 0, 0,
             0, 0, 1, 0, 1, 0, 0,
             1, 1, 0, 0, 0, 1, 1
            };

int S[35] = {0, 0, 0, 0, 1, 0, 0,
             0, 1, 0, 0, 0, 1, 0,
             0, 0, 0, 0, 0, 1, 0,
             0, 1, 0, 0, 0, 1, 0,
             0, 0, 0, 0, 1, 0, 0
            };

// Variables for reading and displaying the player buttons.
int offsetA = 0;
int offsetB = 1;
int offsetC = 2;
int offsetT = 3;
int offsetF = 4;

// Scores.
int player1Score = 0;
int player2Score = 0;
boolean resetScores = false;

// Answers.
String player1Answer = "";
String player2Answer = "";

// Define two modes of operation.
#define QUESTION_MODE 0
#define ANSWER_MODE 1
int mode = QUESTION_MODE;

// Let's get things going.
void setup() {
  // Define the sound pin.
  pinMode(SOUND, OUTPUT);
  digitalWrite(SOUND, LOW); // Set sound off.

  // Define the flash pin.
  pinMode(FLASH, OUTPUT);
  digitalWrite(FLASH, LOW); // Set flash off.

#ifdef DEBUG
  // Serial debugging.
  Serial.begin(115200);
#endif

  // Initialize the I2C bus to talk to the ADS1115.
  Wire.begin(2,14);
  ads1115.begin();

  // Prepare the camera for action.
  initializeCamera();

  // Initialize the decoding object.
  qr.init(&cam);
  sensor_t *s = cam.sensor();
  s->set_framesize(s, FRAMESIZE_CIF);
  s->set_whitebal(s, true);

  // Setup all the NeoPixels.
  pixels.begin();
  pixels.show(); // Initialize all pixels to 'off'.

  // Get Ready for a question.
  showLetter(Q, 32, 32, 32);

  // Reset the score.
  showScore(0, 0);

  // Announce.
#ifdef DEBUG
  Serial.println();
  Serial.println("Think-a-Tron 2020 Ready.");
#endif  
}

// Main control loop.
void loop() {
  // Check for player requested reset.
  if (resetScores) {
    player1Score = 0;
    player2Score = 0;
    showLetter(Q, 32, 32, 32);
    mode = QUESTION_MODE;
    clearPlayerAnswers();
    delay(100);
    showScore(0, 0);
    resetScores = false;
  }
  // Check to see if the ? button has been pressed.
  int16_t questionButton = ads1115.readADC_SingleEnded(QUESTION);
  if (questionButton < 15000) {
    if (mode == QUESTION_MODE) {
      String answer = getQRCode();
      if (answer == "X") {
        // Show the error symbol.
        showLetter(X, 128, 0, 0);
      } else if (answer == "Q") {
        showLetter(Q, 128, 0, 0);
      } else {
        // Display the correct answer.
        if (answer == "A") {
          showLetter(A, 32, 32, 32);
        } else if (answer == "B") {
          showLetter(B, 32, 32, 32);
        } else if (answer == "C") {
          showLetter(C, 32, 32, 32);
        } else if (answer == "T") {
          showLetter(T, 32, 32, 32);
        } else if (answer == "F") {
          showLetter(F, 32, 32, 32);
        }
        // Update the players scores if they selected an answer.
        if (player1Answer != "") {
          if (player1Answer == answer) {
            player1Score++;
            showCorrectAnswer(P1_OFFSET, player1Answer);
          } else {
            showWrongAnswer(P1_OFFSET, player1Answer);
          }
        }
        if (player2Answer != "") {
          if (player2Answer == answer) {
            player2Score++;
            showCorrectAnswer(P2_OFFSET, player2Answer);
          } else {
            showWrongAnswer(P2_OFFSET, player2Answer);
          }
        }
        pixels.show();
        showScore(player1Score, player2Score);
        mode = ANSWER_MODE;
      }
    } else {
      showLetter(Q, 32, 32, 32);
      mode = QUESTION_MODE;
      clearPlayerAnswers();
    }
    // Wait for button up.
    unsigned long  startTime = millis();
    while (ads1115.readADC_SingleEnded(QUESTION) < 15000) {
      delay(50);
    }
    // Check for long press that indicates a reset request.
    if ((millis() - startTime) > 5000) {
      resetScores = true;
    }
  }

  // Check for other user interactions.
  if (mode == QUESTION_MODE) {
    // See if the players have made an answer selection.
    checkPlayerButtons(P1_BUTTONS);
    checkPlayerButtons(P2_BUTTONS);
  }
  delay(100);
}

String getQRCode() {
  // Turn on flash.
  digitalWrite(FLASH, HIGH);

  // Turn on the sound.
  digitalWrite(SOUND, HIGH);

  // Show random display while waiting.
  for (int i = 0; i < 36; i++) {
    showRandom(i);
  }

  // Capture an image.
#ifdef DEBUG
  Serial.println("About to capture image.");
#endif
  camera_fb_t *fb = cam.capture();

  // Turn off the flash.
  digitalWrite(FLASH, LOW); 
  
  if (!fb) {
#ifdef DEBUG
    Serial.println("Image capture failed.");
#endif
    // Error return code.
    return "X";  // Error return code.
  }

  // Show the man inside of the machine.
  showLetter(S, 0, 128, 0);

  // Attempt to read a QR code from the image.
  dl_matrix3du_t *rgb888, *rgb565;
  if (cam.jpg2rgb(fb, &rgb888)) {
    rgb565 = cam.rgb565(rgb888);
  }
  cam.clearMemory(rgb888);
  cam.clearMemory(rgb565);
  dl_matrix3du_t *image_rgb;

  String result = "Q";
  if (cam.jpg2rgb(fb, &image_rgb)) {
    cam.clearMemory(fb);

    qrResoult res = qr.recognition(image_rgb); // Decodes the image containing the data.

    if (res.status) {
      // Successfully obtained the QR code.
#ifdef DEBUG
      Serial.println("Got QR code.");
#endif
      result = res.payload;
    }
  }
  // Shows result on the serial monitor.
#ifdef DEBUG
  Serial.println(result);
#endif

  // Delete image to receive a new image.
  cam.clearMemory(image_rgb);

  // Turn off the sound.
  digitalWrite(SOUND, LOW);
  
  // Retuen the answer.
  return result;
}

// Display the letter passed on the 5x7 LED Array.
void showLetter(int letter[], int R, int B, int G) {
  for (int i = 0; i < NUM_LEDS - MATRIX_OFFSET; i++) {
    if (letter[i] == 1) {
      pixels.setPixelColor(i + MATRIX_OFFSET, R, G, B);
    } else {
      pixels.setPixelColor(i + MATRIX_OFFSET, 0, 0, 0);
    }
  }
  pixels.show();
}

// Show a random changing pattern on the 5x7 LED Array.
void showRandom(int iterator) {
  for (int i = 0; i < NUM_LEDS - MATRIX_OFFSET; i++) {
    if (random(3) == 0) {
      pixels.setPixelColor(i + MATRIX_OFFSET, 32, 32, 32);
    } else {
      pixels.setPixelColor(i + MATRIX_OFFSET, 0, 0, 0);
    }
  }
  // Slowly show the man in the machine.
  if (iterator > 4) pixels.setPixelColor(8 + MATRIX_OFFSET, 0, 0, 128);
  if (iterator > 9) pixels.setPixelColor(22 + MATRIX_OFFSET, 0, 0, 128);
  if (iterator > 14) pixels.setPixelColor(4 + MATRIX_OFFSET, 0, 0, 128);
  if (iterator > 19) pixels.setPixelColor(12 + MATRIX_OFFSET, 0, 0, 128);
  if (iterator > 24) pixels.setPixelColor(19 + MATRIX_OFFSET, 0, 0, 128);
  if (iterator > 29) pixels.setPixelColor(26 + MATRIX_OFFSET, 0, 0, 128);
  if (iterator > 34) pixels.setPixelColor(32 + MATRIX_OFFSET, 0, 0, 128);
  pixels.show();
  delay(100);
}

// Show the players scores on the 7 segment dsiplays.
void showScore(int score1, int score2) {

  int digit1 = (score1 / 10) % 10 ;
  int digit2 = score1 % 10 ;
  int digit3 = (score2 / 10) % 10 ;
  int digit4 = score2 % 10 ;
  
  // Send them to 7 segment displays.
  uint8_t numberToPrint[] = {digits[digit3], digits[digit4], digits[digit1], digits[digit2]};
  sr.setAll(numberToPrint);
}

// Check for a player button press.
void checkPlayerButtons(int playerButtonPin) {

  int16_t adc = ads1115.readADC_SingleEnded(playerButtonPin);
  if (adc > 15000) {
    // Button not pressed.
    return;
  }

  int pOffset;
  if (playerButtonPin == P1_BUTTONS) {
#ifdef DEBUG
    Serial.print("Player 1: ");
#endif
    pOffset = P1_OFFSET;
  } else {
#ifdef DEBUG
    Serial.print("Player 2: ");
#endif
    pOffset = P2_OFFSET;
  }
#ifdef DEBUG
  Serial.println(adc);
#endif
  
  // Light up the button pressed.
  int aOffset = 0;
  if (adc < 1000) {
    aOffset = offsetA;
  } else if (adc < 6000) {
    aOffset = offsetB;
  } else if (adc < 8800) {
    aOffset = offsetC;
  } else if (adc < 10500) {
    aOffset = offsetT;
  } else if (adc < 15000) {
    aOffset = offsetF;
  }
  showAnswer(pOffset, aOffset);
  pixels.show();
  waitForButtonUp(playerButtonPin);
}

// Wait for the button to be released.
void waitForButtonUp(int playerButtonPin) {
  int16_t buttonValue = 0;
  while (buttonValue < 15000) {
    buttonValue = ads1115.readADC_SingleEnded(playerButtonPin);
    delay(50);
  }
}

// Clear the answers from the player panels.
void clearAnswer(int playerOffset) {
  for (int i = 0; i < 5; i++) {
    pixels.setPixelColor(i + playerOffset, 0, 0, 0);
  }
}

// Show the answer selected for a specific player.
void showAnswer(int playerOffset, int answer) {
  clearAnswer(playerOffset);
  pixels.setPixelColor(answer + playerOffset, 128, 128, 128);

  // Remember the player's answer.
  String playerAnswer = "";
  switch (answer) {
    case 0: playerAnswer = "A"; break;
    case 1: playerAnswer = "B"; break;
    case 2: playerAnswer = "C"; break;
    case 3: playerAnswer = "T"; break;
    case 4: playerAnswer = "F"; break;
  }
  if (playerOffset == P1_OFFSET) {
    player1Answer = playerAnswer;
  } else {
    player2Answer = playerAnswer;
  }
}

// Show the answer selected for a specific player as green.
void showCorrectAnswer(int playerOffset, String answer) {
  clearAnswer(playerOffset);
  pixels.setPixelColor(getAnswerOffset(answer) + playerOffset, 0, 128, 0);
}

// Show the answer selected for a specific player as red.
void showWrongAnswer(int playerOffset, String answer) {
  clearAnswer(playerOffset);
  pixels.setPixelColor(getAnswerOffset(answer) + playerOffset, 128, 0, 0);
}

// Convert and answer string to it's equivalent offset.
int getAnswerOffset(String answer) {
  if (answer == "A") {
      return offsetA;
  } else if (answer == "B") {
    return offsetB;
  } else if (answer == "C") {
    return offsetC;
  } else if (answer == "T") {
    return offsetT;
  } else {
    return offsetF;
  }
}

// Get the camera ready for action.
void initializeCamera() {
  // Configure the camera pins.
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 4;
  config.fb_count = 1;

  // Initialize the camera.
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
#ifdef DEBUG
    Serial.printf("Camera start failed with error 0x%x", err);
#endif
    delay(1000);
    ESP.restart(); // Reboot the ESP
  }
}

// Clear the player answers.
void clearPlayerAnswers() {
  clearAnswer(P1_OFFSET);
    player1Answer = "";
    clearAnswer(P2_OFFSET);
    player2Answer = "";
    pixels.show();
}
