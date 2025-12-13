// Names: Casanova Segura, Joey Lewis, Nathan Coffman

#define TRUE 1
#define FALSE 0
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <DHT.h>
#define RDA 0x80
#define TBE 0x20  
#define WATERTHRESHOLD 10  //this line will change after testing (STATE: NOT TESTED)
#define TEMPTHRESHOLD 74
#define DHTPIN 56        // Change to YOUR pin number
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;  
const long interval = 60000; // updates every one minute for LCD display
     
// UART Pointers
volatile unsigned char *myUCSR0A = (unsigned char *)0xC0;
volatile unsigned char *myUCSR0B = (unsigned char *)0xC1;
volatile unsigned char *myUCSR0C = (unsigned char *)0xC2;
volatile unsigned int *myUBRR0 = (unsigned int *)0xC4;
volatile unsigned char *myUDR0 = (unsigned char *)0xC6;
// GPIO Pointers
volatile unsigned char *myPortE = (unsigned char *)0x2E;
volatile unsigned char *myDDRE = (unsigned char *)0x2D;
volatile unsigned char *myportB = (unsigned char *) 0x25;
volatile unsigned char *myDDRB = (unsigned char *) 0x24;
// Timer Pointers
volatile unsigned char *myTCCR1A = (unsigned char *)0x80;
volatile unsigned char *myTCCR1B = (unsigned char *)0x81;
volatile unsigned char *myTCCR1C = (unsigned char *)0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *)0x6F;
volatile unsigned char *myTIFR1 = (unsigned char *)0x16;
volatile unsigned int *myTCNT1 = (unsigned int *)0x85;
volatile unsigned char *myTCCR2A = (unsigned char *)0xB0;
volatile unsigned char *myTCCR2B = (unsigned char *)0xB1;
volatile unsigned char *myTCNT2 = (unsigned char *)0xB2;
volatile unsigned char *myTIFR2 = (unsigned char *)0x37;
// Interrupt Pointers
volatile unsigned char *myEICRB = (unsigned char *)0x6A;
volatile unsigned char *myEIMSK = (unsigned char *)0x3D;
volatile unsigned char *mySREG = (unsigned char *)0x5F;
// LED Pointers
volatile unsigned char *myDDRH = (unsigned char *)0x101;
volatile unsigned char *myPortH = (unsigned char *)0x102;
// ADC Pointers (Water Sensor)
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
// *** Motor Stuff ***
int d = 3;
int CCW;
int CW;
// Pointers for ports
volatile unsigned char *myDDRL  = (unsigned char *)0x106; // PL
volatile unsigned char *myPORTL = (unsigned char *)0x107;
volatile unsigned char *myPINH  = (unsigned char *)0x100;

// *** Real Time Clock ***
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
"Friday", "Saturday"};

volatile unsigned char RESET = FALSE;            

volatile int value = 0;
volatile unsigned char ENABLE = FALSE;  // Program starts disabled (off)
unsigned int currentTicks = 65535;
unsigned char timer_running = 0;
unsigned int pastDate = 0;
unsigned long previousStateMillis = 0;

// *** LCD SCREEN ***
const int RS = 23, EN = 25, D4 = 22, D5 = 24, D6 = 26, D7 = 28;
byte customChar[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

bool error = 0;

void setup() {
// *** LED SETUP ***
  *myDDRH |= 0b01111000;
  /*  *myPortH &= 0b1RYGB111; // Clear 
      *myPortH |= 0b0RYGB000; // LED on
      LED_R 9 - PH6  Red LED
      LED_Y 8 - PH5  Yellow LED
      LED_G 7 - PH4  Green LED
      LED_B 6 - PH3  Blue LED
  */

// *** INTERUPT SETUP ***
  *myDDRE &= 0b11101111;   // (PE4 Input Pin D2 ON/OFF Button)
  *myPortE |= 0b00010000;  // Enable pullup on PE4
  *myEICRB = (*myEICRB & 0b11111100) | 0b00000011;  // Enables rising edge interrupts
  *myEIMSK |= 0b00010000;                           // Enable INT4 in EIMSK at bit 4
  *mySREG |= 0b10000000;                            // Enables global interrupts

  *myDDRE &= 0b11011111;   // PE5 as INPUT
  *myPortE |= 0b00110000;  // Enable pullup on PE5
  *myEIMSK |= 0b00110000;  // Enable INT4 and INT5

// *** TIMER SETUP ***
  *myDDRB |= 0b01000000; // PB6 output (pin 12)
  *myportB &= 0b10111111; // set PB6 low
  setup_timer_regs(); // setup timer for normal mode, with TOV interrupt enabled

// *** LCD SCREEN SETUP ***
  lcd.begin(16, 2);
  lcd.createChar(1, customChar);
  lcd.setCursor(0,0);
  lcd.write((byte)1);

// *** MOTOR SETUP ***
  pinMode(A7, OUTPUT); // For DC motor, analog write is allowed
// Set PL0, PL2, PL4, PL6 as OUTPUT
  *myDDRL |= 0b01010101;
  *myPORTL &= 0b10101010; // all LEDs off

  // Set buttons PH0 (pin 17) and PH1 (pin 16) as INPUT
  *myDDRH &= 0b11111100;  // clear bits 0 and 1
  *myPortH |= 0b00000011; // enable internal pull-ups

// *** RTC ***
  rtc.begin();
  
  U0Init(9600);
//  Serial.begin(9600);


// *** WATER SENSOR SETUP ***
  adc_init();
  dht.begin();

}





void loop() {
  ventControl();
    // == ERROR == 
    error = Error(adc_read(0));
    while ((Error(adc_read(0)) == TRUE) && (ENABLE == TRUE)){
      timeCheck('Y');
      *myPortH &= 0b10000111; // Clear
      *myPortH |= 0b01000000; // Red LED on
      clockChange(0);
      ventControl();
      analogWrite(A7, 0);
      adc_read(0);
      getTemperature();
      RESET = 0;
      if(RESET == TRUE){
        break;
      }
      /*Serial.println("Error Water Level:  ");
      Serial.println(adc_read(0));
      Serial.println("Error Temp:  ");
      Serial.println(getTemperature());
      delay(1000); */
      Idle(getTemperature());
      Error(adc_read(0));
    }
    // == IDLE == 
    while(((Idle(getTemperature()) == TRUE && Error(adc_read(0)) == FALSE) && RESET == TRUE) && (ENABLE == TRUE)){
      timeCheck('N');
      *myPortH &= 0b10000111; // Clear
      *myPortH |= 0b00010000; // Green LED on
      clockChange(1);
      ventControl();
      analogWrite(A7, 0);
      adc_read(0);
      getTemperature();
      // need to implement clock and LED display
      /*Serial.println("Idle Water Level:  ");
      Serial.println(adc_read(0));
      Serial.println("Idle Temp:  ");
      Serial.println(getTemperature());
      delay(1000); */
      Idle(getTemperature());
      Error(adc_read(0));
    }
    //  == RUNNING == 
    while((Idle(getTemperature()) == FALSE && Error(adc_read(0)) == FALSE && RESET == TRUE) && (ENABLE == TRUE)){
      timeCheck('N');
      *myPortH &= 0b10000111;  // Clear
      *myPortH |= 0b00001000;  // Blue LED on
      clockChange(2);
      ventControl();
      analogWrite(A7, 150);
      adc_read(0);
      getTemperature();
      /*Serial.println("Running Water Level:  ");
      Serial.println(adc_read(0));
      Serial.println("Running Temp:  ");
      Serial.println(getTemperature());
      delay(1000);*/
      Idle(getTemperature());
      Error(adc_read(0));
    }


    while(ENABLE == FALSE) {  // ** Disabled State **
      clockChange(3);
      analogWrite(A7, 0);
      *myPortH &= 0b10000111;      // Clear
      *myPortH |= 0b00100000;      // Yellow LED on
      error = 0;
      timeCheck('D');
      //Serial.println("lcd");
      //delay(500);
  }
}

// ============ DELAY FUNCTION ============
void myDelay (unsigned int ms){
  *myTCCR2A = 0x00;
  *myTCCR2B = 0x00;
  while (ms--){
    *myTCNT2 = 6;
    *myTIFR2 = 0x01;
    *myTCCR2B = 0b00000100;
    while (!(*myTIFR2 & 0x01));
  }
  *myTCCR2B = 0x00;
}

// ============ LCD UPDATE TIMER (ONE MINTUE) ============
void timeCheck(unsigned char state){ 
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    lcdDisplay(state);
  }
}

// ============ LCD ============
void lcdDisplay(unsigned char errorState){
  if (errorState == 'Y'){
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("Water  level");
    lcd.setCursor(3,1);
    lcd.print("is too low");
  }
  else if (errorState == 'N'){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.setCursor(6,0);
    lcd.print(getTemperature());
    lcd.setCursor(0,1);
    lcd.print("Humidity:");
    lcd.setCursor(10,1);
    lcd.print(adc_read(0));
  }
  else if (errorState == 'D'){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("====DISABLED====");
    lcd.setCursor(0,1);
    lcd.print("================");
  }
}

// ============ CLOCK ============
void clockToSerial(){
  DateTime now1 = rtc.now();
  DateTime now = now1 - TimeSpan(0,16,7,0);
  put4(now.year());
  putChar('/');
  put2(now.month());
  putChar('/');
  put2(now.day());
  putChar(' ');
  putString(daysOfTheWeek[now.dayOfTheWeek()]);
  putChar(' ');
  put2(now.hour());
  putChar(':');
  put2(now.minute());
  putChar(':');
  put2(now.second());
  putChar('\n');
}
void clockChange(unsigned int state){
  if (pastDate != state){
    putChar('\n');
    clockToSerial(); // prints current time (not Beijing China)
    if (state == 2){
      putString("FAN MOTOR ON\n"); // prints DC motor
    }
    putString("STATE CHANGE: "); // prints stage changes
    unsigned long currentStateMillis = millis();
    unsigned long deltaTms = currentStateMillis - previousStateMillis;
    unsigned long deltaTs = deltaTms/1000;
    put2(deltaTs);
    putString(" seconds elapsed\n");
    previousStateMillis = currentStateMillis;
  }
  pastDate = state;
}

// ========== VENT CONTROL ===========
void ventControl(){
    // Read buttons (active high)
    CCW = (*myPINH & 0b00000001);
    CW = (*myPINH & 0b00000010);
    if(CW>0){
      CW = 1;
    }
    if (CW == CCW){ CW = 0; CCW = 0; }

    //Serial.print("CCW: "); Serial.print(CCW);
    //Serial.print("  CW: "); Serial.println(CW);
    if (CW){  
      lcd.setCursor(15,1);
      lcd.print("<");
    }
    else if (CCW){
      lcd.setCursor(15,1);
      lcd.print(">");
    }

    if(CW){
    //pin 49 ON, other control pins OFF
    PORTL = (PORTL & 0b10101010) | 0b00000001;
    myDelay(d);

    //pin 47 ON, other control pins OFF
    PORTL = (PORTL & 0b10101010) | 0b00000100;
    myDelay(d);

    //pin 45 ON, other control pins OFF
    PORTL = (PORTL & 0b10101010) | 0b00010000;
    myDelay(d);

    //pin 43 ON, other control pins OFF
    PORTL = (PORTL & 0b10101010) | 0b01000000;
    myDelay(d);
    }

    else if(CCW){
    //pin 43 ON, other control pins OFF
    PORTL = (PORTL & 0b10101010) | 0b01000000;
    myDelay(d);

    //pin 45 ON, other control pins OFF
    PORTL = (PORTL & 0b10101010) | 0b00010000;
    myDelay(d);

    //pin 47 ON, other control pins OFF
    PORTL = ((PORTL & 0b10101010) | 0b00000100);
    myDelay(d);

    //pin 49 ON, other control pins OFF
    PORTL = (PORTL & 0b10101010) | 0b00000001; // PL0 HIGH
    myDelay(d);
    }
    else {
      PORTL = (PORTL & 0b10101010) | 0b00000000;
    }
}


// ========== BUTTON ENABLE/DISABLE INTERUPT ===========
ISR(INT4_vect) {
  ENABLE ^= 1;  // Toggle
}

ISR(INT5_vect) {
  RESET = 1;  // NO TOGGLE 
}


// ========== TIMER FUNCTIONS ==========
void setup_timer_regs() {
  // setup timer control registers
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  TCCR1C = 0x00;
  TIFR1 |= 0x01; // reset the TOV flag
  TIMSK1 |= 0x01; // enable TOV interrupt
}
ISR(TIMER1_OVF_vect) {
  TCCR1B &= 0b11111000; // stop timer
  TCNT1 = (unsigned int) (65535 - (unsigned long) (currentTicks)); // load counter
  TCCR1B |= 0b00000001; // start timer
  if (currentTicks != 65535) { // if it's not the STOP amount
    PORTB ^= 0x40; // XOR to toggle PB6
  }
}

// ========= WATER SENSOR FUNCTIONS =========
void adc_init() 
{
  *my_ADCSRA |= 0b10000000;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRA &= 0b11110111;
  *my_ADCSRA &= 0b11111000;

  *my_ADCSRB &= 0b111101111;
  *my_ADCSRB &= 0b111110000;

  *my_ADMUX &= 0b01111111;
  *my_ADMUX |= 0b01000000;
  *my_ADMUX &= 0b01111111;
  *my_ADMUX &= 0b11011111;
  *my_ADMUX &= 0b11100000;
}
unsigned int adc_read(unsigned char adc_channel_num) //work with channel 0
{
  *my_ADMUX &= 0b11100000;
  *my_ADCSRB &= 0b11110111;//
  *my_ADMUX |= 0b00000000;
  *my_ADCSRA |= 0b01000000;
  while((*my_ADCSRA & 0x40) != 0);
  unsigned int val = *my_ADC_DATA;
  return val;
}
//OUTPUTS TO CONSOL (DEBUGGING)
void printWaterLevel(unsigned int level) {
  char message[] = "Water Level: ";
  char numBuffer[6];
  int i = 0;
  
  // Print message
  for(int j = 0; message[j] != '\0'; j++) {
    putChar(message[j]);
  }
  
  // Handle zero case
  if (level == 0) {
    putChar('0');
  } else {
    // Convert number to string (reversed)
    while (level > 0) {
      numBuffer[i++] = (level % 10) + '0';
      level /= 10;
    }
    // Print in correct order (reverse)
    for (int j = i - 1; j >= 0; j--) {
      putChar(numBuffer[j]);
    }
  }
  putChar('\n');
}
//WATER LEVEL ERROR CONDITIONS OUTPUT
bool Error(unsigned int level){
  int threshold = WATERTHRESHOLD;             
  int waterlevel = adc_read(0);    
  if (threshold < waterlevel){
    return (0);
  } else {
    return (1);
  }
}
// ========= TEMPERATURE SENSOR FUNCTIONS =========
int getTemperature(){
  int temp = dht.readTemperature(true);  
  if (temp < -40 || temp > 180) return -1;  // Error
  return (temp);
}
int getHumidity(){
  int humid = dht.readHumidity();
  if (humid < 0 || humid > 100) return -1;  // Error
  return (humid);
}
bool Idle(unsigned int temp){
  int threshold = TEMPTHRESHOLD;             
  int temperature = temp;    
  if (threshold > temperature){
    return (1);
  } else {
    return (0);
  }
}

// ========= UART FUNCTIONS =========
void U0Init(int U0baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  // Same as (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}
unsigned char kbhit() {
  if ((*myUCSR0A & 0b0010000) == 1) return 0;       // If 1, empty, False
  else if ((*myUCSR0A & 0b0010000) == 0) return 1;  // If 0, charater read, True
}
unsigned char getChar() {
  unsigned char ch;
  while (!(*myUCSR0A & (1 << 7)));
  ch = *myUDR0;
  //Serial.print(ch);
  return ch;
} // CHARACTER PRINT FUNCTIONS ==========
void putChar(unsigned char U0pdata) {
  while (!(*myUCSR0A & 1 << 5));
  *myUDR0 = U0pdata;
}
void putString(const char *s){
  while (*s) {
    putChar(*s++);
  }
}
void put2(unsigned int n){
  putChar('0' + (n/10) % 10);
  putChar('0' + (n % 10));
}
void put4(unsigned int n){
  putChar('0' + (n/1000) % 10);
  putChar('0' + (n/100) % 10);
  putChar('0' + (n/10) % 10);
  putChar('0' + (n % 10));
}


