#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

//int address = 2;
int limit = 10;

const int detectPin1 = 3;
const int detectPin2 = 4;
const int detectPin3 = 5;
int count = 0; int flag = 0;

static const u1_t PROGMEM APPEUI[8]={0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the early prototype TTN
// network.
static const u1_t PROGMEM DEVEUI[8]= {0x4E, 0x95, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// LoRaWAN end-device address (DevAddr)
static const u1_t PROGMEM APPKEY[16] = {0x24, 0x2F, 0x2B, 0xF7, 0x2F, 0xCB, 0xA5, 0x02, 0xBC, 0x1E, 0x0A, 0x80, 0x8C, 0xF9, 0x44, 0x73};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[4] = {0x00,0x00,0x00,0x00};
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

void detect()
{
  int value1 = 0;
  int value2 = 0;
  int value3 = 0;

  if(digitalRead(detectPin1) == 1){
    value1 = 1; count = 0;
    Serial.println("\nElephant Detected!");
  }
  else if(digitalRead(detectPin1) == 0){
    value1 = 0; 
    //Serial.println("\nElephant Not Detected!");
  }

  if(digitalRead(detectPin2) == 1){
    value2 = 1; count = 0;
    Serial.println("Cheetah Detected!");
  }
  else if(digitalRead(detectPin2) == 0){
    value2 = 0;
    //Serial.println("Cheetah Not Detected!");
  }

  if(digitalRead(detectPin3) == 1){
    value3 = 1; count = 0;
    Serial.println("Wild Boar Detected!");
  }
  else if(digitalRead(detectPin2) == 0){
    value3 = 0;
    //Serial.println("Wild Boar Not Detected!");
  }
  Serial.println();

  if ((value1==0)&&(value2==0)&&(value3==0)){
    flag = 0;
    if(count>=limit){
      flag = 2;
      count = 0;
    } 
    else flag = 3;
    count++;
  }

  else flag = 1;

  mydata[0] = flag;
  mydata[1] = value1;
  mydata[2] = value2;
  mydata[3] = value3;
  //mydata[4] = address;
  
  //delay(1000); //Delay of 1 second for ease of viewing
}

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Event Occured!"));
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) 
    {
        Serial.println(F("OP_TXRXPEND, not sending"));
    }
    else 
    {
        detect();
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {

    pinMode(detectPin1, INPUT);
    pinMode(detectPin2, INPUT);
    pinMode(detectPin3, INPUT);
    
    Serial.begin(9600);
    Serial.println(F("Starting"));

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);

    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
