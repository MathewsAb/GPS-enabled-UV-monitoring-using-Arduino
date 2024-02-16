#include <LiquidCrystal.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

volatile unsigned long timerCount = 0; // variable to track timer interrupt counts
const unsigned long TIMER_INTERVAL = 10000; // interval of 10 seconds

bool runInitialCode = true; // flag to indicate whether to run the initial code or the following code


//gps

static const int RXPin = 50, TXPin = 51;
static const uint32_t GPSBaud = 9600;
//UV

int prev_uvsensor_bars = 0;
int prev_user_set_bars = 0;
const int uvSensorPin = A1;


const int buzzerPin = 22; // Pin connected to the buzzer
/*****************************************************/



int user_set_bars = 0; // Initial comma position
/*****************************************************/

int buzzer_beep_flag = 0;

// Define the custom character for the shaded block
byte shadedBlock[8] = 
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};


// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup() {
  lcd.begin(16, 2); // initialize the LCD
  lcd.print("SEE712-T1-2023"); // display initial message on LCD

  // Set up Timer1 to trigger interrupt every 1 millisecond
  noInterrupts(); // disable interrupts

  // Set Timer1 prescaler to 64
  TCCR1B = (1 << CS11) | (1 << CS10);

  // Set Timer1 compare match value for 1 millisecond interrupt interval
  OCR1A = 249;

  // Enable Timer1 compare match interrupt
  TIMSK1 |= (1 << OCIE1A);

  interrupts(); // enable interrupts

  //gps
  ss.begin(GPSBaud);
  // Load the custom character into LCD memory
  lcd.createChar(0, shadedBlock);

  lcd.setCursor(0, 1); // Set cursor to the first column of the second row
  lcd.print("UV");     // Display "UV" in the first two columns

  Serial.begin(9600);  // Initialize the serial communication
  
  //pinMode(buzzerPin, OUTPUT); // Set the buzzer pin as an output
  DDRA |= (1 << PA0); // LOW LEVEL CODE TO MAKE PIN 22 OUTPUT
  buzzer_off();
}

void buzzer_on()
{
  //digitalWrite(buzzerPin, LOW);
  PORTA &= ~(1 << PA0);
   // WRITE LOW SIGNAL TO BUZZER
}

void buzzer_off()
{
  //digitalWrite(buzzerPin, HIGH);
   PORTA |= (1 << PA0);//WRITE HIGH SIGNAL TO BUZZER
}

/*
 * funtion should be called in a loop. whenever "buzzer_beep_ctrl_flag"
 * is 1, this function makes the buzzer beep.
 */
void buzzer_control_loop(int buzzer_beep_ctrl_flag)
{
  const int buzzer_toggle_delay_ms = 80;
  static int buzzer_toggle_timer_ms = 80;
  static int buzzer_toggle_flag = 0;

  Serial.print("buzzer_beep_ctrl_flag : ");
  Serial.println(buzzer_beep_ctrl_flag);

  if(buzzer_beep_ctrl_flag)
  {
    if(buzzer_toggle_timer_ms > buzzer_toggle_delay_ms)
    {
      //XOR operation to toggle flag. make flag 1 if 0. make flag 0 if already 1.
      buzzer_toggle_flag = buzzer_toggle_flag ^ 1;
      if (buzzer_toggle_flag)
      {
        buzzer_on();
        //Serial.println("Buzzer_high");
      }
      else
      {
        buzzer_off();
        Serial.println("Buzzer_low");
      }
      buzzer_toggle_timer_ms = 0;
    }
    buzzer_toggle_timer_ms++;
    _delay_us(100);

  }
  else
  {
   buzzer_off();
  }
}

void loop() 
{
  if (timerCount >= TIMER_INTERVAL) {
    runInitialCode = false; // set the flag to indicate not running the initial code
    lcd.clear(); // clear the LCD
    timerCount = 0; // reset the timer count
  }

  if (runInitialCode) {
    // 
    static int pos = 0;
    char str[] = " Mathews Abraham 221407594 ";

    // Update second row every 5 seconds
    if (pos == 0) {
      lcd.setCursor(0, 1);
      lcd.print(str + pos);
      pos++;
    } else if (pos < sizeof(str) - 16) {
      lcd.setCursor(0, 1);
      lcd.print(str + pos);
      pos++;
      _delay_us(1000000);
    } else {
      pos = 0;
    }
  } else 
  {
    
    
    // code to be executed after 10 seconds
    lcd.setCursor(0, 1); // Set cursor to the first column of the second row
    lcd.print("UV");
    int uvValue = analogRead(uvSensorPin);  // Read the UV intensity value

 

  // Convert the raw ADC value to a voltage
  float voltage = uvValue * (5.0 / 1023.0);
  float uvIndex = voltage / 0.1;

  // Calculate the percentage directly
  int percentage = (uvIndex >= 1.0) ? 100 : (uvIndex * 100);

  // Calculate the number of shaded columns based on the percentage
  int uv_sensor_bars = (percentage * 14) / 100;
  
  if ((prev_uvsensor_bars != uv_sensor_bars) || prev_user_set_bars != user_set_bars) 
  {
   // Clear the columns after "UV" in the second row
    lcd.setCursor(2, 1);
    lcd.print("               ");
    
    // Clear the remaining columns
    for (int i = uv_sensor_bars; i < 14; i++) 
    {
      lcd.write(' ');// Write a space character
    }
    prev_uvsensor_bars = uv_sensor_bars;
    prev_user_set_bars = user_set_bars; 
  } 
  // Set the cursor back to the first column after "UV"
  lcd.setCursor(2, 1);

  // Shade the appropriate number of columns
  for (int i = 0; i < uv_sensor_bars; i++) 
  {
    lcd.write(byte(0));  // Write the custom character for shaded block
  }

  int x = analogRead(0);

  // Check if UP button is pressed
  if (x < 200) 
  {
    _delay_us(2000000); // Debounce delay
    user_set_bars++; // Move the comma right
    if (user_set_bars > 13) {
      user_set_bars = 13; // Limit comma position to the rightmost column
    }
  }
  // Check if DOWN button is pressed
  else if (x < 400)
   {
    _delay_us(2000000); // Debounce delay
    user_set_bars--; // Move the comma left
    if (user_set_bars < 0) {
      user_set_bars = 0; // Limit comma position to the leftmost column
    }
  }

  lcd.setCursor(user_set_bars + 2, 1); // Set the cursor position for comma
  lcd.write("'"); // Display the comma character

    //Serial.print("  uv_sensor_bars:  ");
    //Serial.print(uv_sensor_bars);
    //Serial.print("  user_set_bars:");
    //Serial.print(user_set_bars);
    //Serial.print("\n");

  // Check if shaded columns touch or exceed the apostrophe
  if (uv_sensor_bars > user_set_bars) 
  {
    buzzer_beep_flag = 1;
  } else {
    buzzer_beep_flag = 0;
  }

  buzzer_control_loop(buzzer_beep_flag);

    lcd.clear();
    while (ss.available() > 0)
    if (gps.encode(ss.read()))
        displayInfo();
    
  }
}

// Timer1 compare match interrupt service routine
ISR(TIMER1_COMPA_vect) {
  timerCount++; // increment the timer count
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    //Serial.print(gps.location.lat(), 6);
   //Serial.print(F(","));
    //Serial.print(gps.location.lng(), 6);
    //lcd.clear(); // clear the LCD display
    lcd.setCursor(0, 0);
    lcd.print("La:");
    lcd.print(gps.location.lat(), 3); // display the latitude on the left side of the first row, up to 6 decimal places
    lcd.setCursor(8, 0);
    lcd.print("Lo:");
    lcd.print(gps.location.lng(), 3); // display the longitude on the right side of the first row, up to 6 decimal places
    _delay_us(1500000);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  

  Serial.println();
}


