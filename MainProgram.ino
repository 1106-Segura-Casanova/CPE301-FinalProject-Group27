// Names: Casanova Segura, Joey Lewis, Nathan Coffman

#define TRUE 1
#define FALSE 0
#include <LiquidCrystal.h>


            // CASANOVA 12-7-25 //      START
#include <DHT.h>
#define RDA 0x80
#define TBE 0x20  
#define WATERTHRESHOLD 90  //this line will change after testing (STATE: NOT TESTED)
#define TEMPTHRESHOLD 74
#define DHTPIN 56        // Change to YOUR pin number
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
            // CASANOVA 12-7-25 //      END
            // CASANOVA 12-10-25 //      START
unsigned long previousMillis = 0;  
const long interval = 1000; 
            // CASANOVA 12-10-25 //      END            
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
// Interrupt Pointers
volatile unsigned char *myEICRB = (unsigned char *)0x6A;
volatile unsigned char *myEIMSK = (unsigned char *)0x3D;
volatile unsigned char *mySREG = (unsigned char *)0x5F;
// LED Pointers
volatile unsigned char *myDDRH = (unsigned char *)0x101;
volatile unsigned char *myPortH = (unsigned char *)0x102;

            // CASANOVA 12-7-25 //      START
// ADC Pointers (Water Sensor)
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
            // CASANOVA 12-7-25 //      END

            // CASANOVA 12-9-25 //      START
volatile unsigned char RESET = FALSE;            
            // CASANOVA 12-9-25 //      END
volatile int value = 0;
volatile unsigned char ENABLE = FALSE;  // Program starts disabled (off)
unsigned int currentTicks = 65535;
unsigned char timer_running = 0;

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
              // CASANOVA 12-9-25 //      START
  *myDDRE &= 0b11011111;   // PE5 as INPUT
  *myPortE |= 0b00110000;  // Enable pullup on PE5
  *myEIMSK |= 0b00110000;  // Enable INT4 and INT5
              // CASANOVA 12-9-25 //      END
// *** TIMER SETUP ***
  *myDDRB |= 0b01000000; // PB6 output (pin 12)
  *myportB &= 0b10111111; // set PB6 low
  setup_timer_regs(); // setup timer for normal mode, with TOV interrupt enabled

// *** LCD SCREEN SETUP ***
  lcd.begin(16, 2);
  lcd.createChar(1, customChar);
  lcd.setCursor(0,0);
  lcd.write((byte)1);

  U0Init(9600);
  Serial.begin(9600);

            // CASANOVA 12-7-25 //      START
// *** WATER SENSOR SETUP ***
  adc_init();
  dht.begin();
            // CASANOVA 12-7-25 //      END
}





void loop() {
  if (ENABLE == TRUE) {  // ** Enabled State **

            // CASANOVA 12-10-25 //      START
  //60 second temp/water clock
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    printWaterLevel(adc_read(0)); // output to LCD
  }
            // CASANOVA 12-10-25 //      END    

              // CASANOVA 12-9-25 //      START
    // == ERROR == 
    error = Error(adc_read(0));
    while (error == TRUE){
      *myPortH &= 0b10000111; // Clear
      *myPortH |= 0b01000000; // Red LED on
      if(RESET == TRUE){
        break;
      }
    }
    // == IDLE == 
    while((Idle(getTemperature()) == TRUE && Error(adc_read(0)) == FALSE) || RESET == TRUE ){
      *myPortH &= 0b10000111; // Clear
      *myPortH |= 0b00010000; // Green LED on
      // need to implement clock and LED display
    }
    RESET = 0;
    //  == RUNNING == 
    while(Idle(getTemperature()) == FALSE && Error(adc_read(0)) == FALSE){
      // ADD FAN MOTOR CODE
      *myPortH &= 0b10000111;  // Clear
      *myPortH |= 0b00001000;  // Blue LED on
    }
              // CASANOVA 12-9-25 //      END

  } else if (ENABLE == FALSE) {  // ** Disabled State **
    *myPortH &= 0b10000111;      // Clear
    *myPortH |= 0b00100000;      // Yellow LED on
    delay(100);
    error = 0;
  }
}

              // CASANOVA 12-11-25 //      START
// ============ CLOCK ============

              // CASANOVA 12-11-25 //      END




// ========== BUTTON ENABLE/DISABLE INTERUPT ===========
ISR(INT4_vect) {
  ENABLE ^= 1;  // Toggle
  Serial.println("BUTTON PRESSED");
}
              // CASANOVA 12-9-25 //      START
ISR(INT5_vect) {
  RESET = 1;  // NO TOGGLE 
  Serial.println("BUTTON PRESSED");
}
              // CASANOVA 12-9-25 //      END

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

            // CASANOVA 12-7-25 //      START
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
  if (threshold > waterlevel){
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
  if (threshold < temperature){
    return (1);
  } else {
    return (0);
  }
}
            // CASANOVA 12-7-25 //      END

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
}
void putChar(unsigned char U0pdata) {
  while (!(*myUCSR0A & 1 << 5));
  *myUDR0 = U0pdata;
}

// ----   NATHAN 12/7 Stepper Motor Progress:   ------ 

// int d;
// int CCW;
// int CW;
//  ---    SETUP ---  {
//   // Set pins 49,47,45,43 as OUTPUT
//   DDRL |= 0b01010101;
//   PORTL &= 0b10101010;
  
//   DDRH &= 0b11111110;  // Clear PH0
//   //PORTH |= 0b00000001; // Enable pull up

//   // Set pin 26 input
//   DDRH &= 0b11111011;  // Clear PH2
//   //PORTH |= 0b00000100; // Enable pull up


//   d = 10;
// }

//. ------ LOOP -----{

//   CCW = PINH & 0b00000001;
//   if(CCW == 0){
//     CW = PINH & 0b00000100;
//   }
  
//   if(CW){
//   //pin 49 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b00000001; // PL0 HIGH
//   delay(d);

//   //pin 47 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b00000100; // PL2 HIGH
//   delay(d);

//   //pin 45 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b00010000; // PL4 HIGH
//   delay(d);

//   //pin 43 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b01000000; // PL6 HIGH
//   delay(d);
//   }
  
//   else if(CCW){
//   //pin 43 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b01000000; // PL6 HIGH
//   delay(d);

//   //pin 45 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b00010000; // PL4 HIGH
//   delay(d);

//   //pin 47 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b00000100; // PL2 HIGH
//   delay(d);

//   //pin 49 ON, other control pins OFF
//   PORTL = (PORTL & 0b10101010) | 0b00000001; // PL0 HIGH
//   delay(d);
//   }
// }

/*
Record the time and date every time the motor is turned on or of. This information
should be transmitted to a host computer (over USB)

The real-time clock module must be used for event reporting.
– You may use the Arduino library for the clock

Humidity and temperature should be continuously monitored and reported on the LDC
screen. Updates should occur once per minute

(IDLE)
Exact time stamp (using real time clock) should record transition times

*/




