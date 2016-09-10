// #include <LiquidCrystal.h>
#include <DHT.h>
//#include <TimerOne.h>
//#include <Wire.h>
//#include <DS1307new.h>
//#include <stdio.h>
#include <DS1302.h>
#include <EEPROM.h>
#include <Time.h>
#include <math.h>

#define DEBUG 1

#define COND_NR 8
uint32_t result;

#define TEMP1_CAL -0.5
 // *********************************************
// VARIABLES
// *********************************************

#define HZ 100
#define SAMPLING_PERIOD  5 //seconds. Minimum 2s for DHT22

const uint8_t relay_pins[] = {3,4};

const int kCePin   = 5;  // Chip Enable
const int kIoPin   = 6;  // Input/Output
const int kSclkPin = 7;  // Serial Clock


long called = 0;

#define ANALOG_PINS 6
int analog[ANALOG_PINS];


// Create a DS1302 object.
DS1302 rtc(kCePin, kIoPin, kSclkPin);

#define O_TEMP1 0
#define O_TEMP2 1
#define O_HUM1 2
#define O_HUM2 3
#define O_ROS1 4
#define O_ROS2 5
#define O_MOI1 6
#define O_MOI2 7
#define O_PWR  8
#define O_NOW  9


#define SOIL_EN 10

#define HBRIDGE_2 11
#define HBRIDGE_1 12


#define DHTTYPE DHT22
#define DHTPIN 8
#define DHT2PIN 9

//dht DHT;
DHT dht,dht2;

//DHT dht2(6,DHT11);

uint8_t modes = 0;
uint8_t modes2 = 0;

struct timer {
   uint8_t pin;
   uint8_t type;
   uint8_t heure_debut;
   uint8_t minute_debut;
   uint8_t heure_fin;
   uint8_t minute_fin;
   uint8_t temp_u;
   uint8_t temp_d;
   float   temp1;
};

struct sensor_state_d {
   float temp1;
   float temp2;
   float hum1;
   float hum2;
   float ros1;
   float ros2;
   float moist1;
   float moist2;
};

union sensor_state {
    sensor_state_d s;
    float d[sizeof(sensor_state_d)];
    };



sensor_state state[4];

#define RELAY1  4

boolean first = true;

void open_gardena_solenoid(){
    digitalWrite(HBRIDGE_1, HIGH);
    delay(500);
    digitalWrite(HBRIDGE_1, LOW);
    delay(150);
}

void close_gardena_solenoid(){
    digitalWrite(HBRIDGE_2, HIGH);
    delay(150);
    digitalWrite(HBRIDGE_2, LOW);
    delay(150);
}



void setup() {
  //Serial.begin(115200);
  Serial.begin(9600);

//  memset(timers,0,sizeof(timers));

  dht.setup(DHTPIN);
  dht2.setup(DHT2PIN);

  //  analogReadResolution(10);

  rtc.writeProtect(false);
  rtc.halt(false);

  pinMode(13, OUTPUT); // Led set off
  digitalWrite(13, LOW);


  pinMode(SOIL_EN, OUTPUT); // Dont throw current to roots allthetime
  digitalWrite(SOIL_EN, LOW);

  pinMode(HBRIDGE_1, OUTPUT); // 
  digitalWrite(HBRIDGE_1, LOW);

  pinMode(HBRIDGE_2, OUTPUT); // 
  digitalWrite(HBRIDGE_2, LOW);


  Time t = rtc.time();
  setTime(t.hr, t.min, t.sec, t.date, t.mon, t.yr);

  for(uint8_t r=0;r<sizeof(relay_pins);r++) {
    pinMode(relay_pins[r], OUTPUT); // 
    digitalWrite(relay_pins[r], HIGH);
  }

/*
  for(uint8_t r=0;r<sizeof(relay_pins);r++) {
  digitalWrite(relay_pins[r],LOW);
  delay(1000);
  digitalWrite(relay_pins[r],HIGH);
  }
*/
  Serial.print("Hello i'm Perceptor v0.1\n");
  /*
  open_gardena_solenoid();
  delay(1000);
  close_gardena_solenoid();
  */

//    evaluate();
}

char buffer [32];
uint8_t cnt = 0;
boolean ready = false;
uint8_t val;

uint8_t ctoi(char c) {
    if ((c>='0') && (c<='9')){
        return c - '0';
    } else if ((c>='A') && (c<='F')){
        return 10 + (c - 'A');
    } else if ((c>='a') && (c<='f')){
        return 10 + (c - 'a');
    } else {
        return 0;
    }
}

void print_uint8(uint8_t i) {
    char out;
    uint8_t t;
    t = i>>4;
    if(t>9){
       out='A'+(t-10);
    } else {
       out='0'+t;
    }
    Serial.print(out);
    t = i % 16;
    if(t>9){
       out='A'+(t-10);
    } else {
       out='0'+t;
    }
    Serial.print(out);
}

void set_rtc(time_t t) {
    tmElements_t tm;
    breakTime(t,tm);
    Time rt(tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second, Time::kSunday);
    rtc.time(rt);
}



boolean ParseLine()
{
char * key;
char * value;
boolean is_write = false;
uint32_t t;

    key = strtok(buffer, "=");    // Everything up to the '=' is the color name
    value = strtok(NULL, "\n");  // Everything else is the color value

    if ((key != NULL) && (value != NULL))
    {
        val = atoi(value);

        switch (toupper(key[0]))
        {
            case 'Y':
                rtc.year((uint16_t) 2000 + val);
                break;
            case 'M':
                rtc.month(val);
                break;
            case 'D':
                rtc.date(val);
                break;
            case 'H':
                rtc.hour(val);
                break;
            case 'N':
                rtc.minutes(val);
                break;
            case 'S':
                rtc.seconds(val);
                break;
            case 'A':
                if ((val < ANALOG_PINS) && (val >=0)){
                    Serial.print("a");
                    Serial.print(val) ;
                    Serial.print("=") ;
                    Serial.print(analog[val]) ;
                    Serial.print("\n");
                } else {
                    return false;
                }
                break;
            case 'W':  // Hex string, first 3 bytes = address
                is_write = true;
            case 'R':
                int address;
                uint8_t c;
                c=0;
                address = ctoi(value[0]) * 16 * 16 + ctoi(value[1]) * 16 + ctoi(value[2]);
                value +=3;
                if(is_write) {
                    while((value[0] != '\n') && (value[0]!='\0') && (value[1] != '\n') && (value[1]!='\0')){
                        uint8_t v = ctoi(value[0]) * 16  + ctoi(value[1]) ;
                        EEPROM.update(address++, v);
                        value+=2;
                        c++;
                    }
                } else {
                    c=32;
                    address += c;
                }

                Serial.print(is_write ? "W" : "R");
                Serial.print("=");
                Serial.print(address);
                for (uint8_t m=0;m<c;m++){
                    print_uint8 (EEPROM.read((address -c)+m));
                }
                Serial.print("\n");
                break;
            case 'U':
                t = strtoul(value,NULL,0);
                setTime(t);
                set_rtc(t);
                Serial.print("U=");
                Serial.print(t);
                Serial.print("\n");
                break;
            default:
                 Serial.print("NAK:");
                 Serial.print(key[0]);
                 Serial.print("\n");
                 break;
        }
    }
    else if (key != NULL) {
        switch (toupper(key[0]))
        {
            case 'S':
                 Serial.print("t1=");
                 Serial.print(state[0].d[O_TEMP1]);
                 Serial.print(",h1=");
                 Serial.print(state[0].s.hum1);
                 Serial.print(",t2=");
                 Serial.print(state[0].s.temp2);
                 Serial.print(",h2=");
                 Serial.print(state[0].s.hum2);
                 Serial.print("\n");
                 Serial.print("r1=");
                 Serial.print(state[0].s.ros1);
                 Serial.print(",r2=");
                 Serial.print(state[0].s.ros2);
                 Serial.print("\n");
                 break;
//            case 'T':
//                 printTime();
                 break;
            case 'U':
                 Serial.print("U=");
                 Serial.print((uint32_t)now());
                 Serial.print("\n");
                 break;
            case 'E':
                 Serial.print("E=");
                 for (uint8_t iter=0;iter<COND_NR;iter++) {
                    Serial.print((result >> iter) & 1);
                 }
                 Serial.print("\n");
                 break;

            default:
                 Serial.print("NAK:");
                 Serial.print(key[0]);
                 Serial.print("\n");
                 break;

        }

    }
    return true;
}

#define ROS_A 17.27
#define ROS_B 237.7

float compute_alpha(float t, float rh) {
    return ((ROS_A*t)/(ROS_B+t)) + log(rh/100.0);
}

float compute_ros(float t, float rh) {
    float alpha = compute_alpha(t,rh);
    return (ROS_B*alpha)/(ROS_A-alpha);
}

void store_ros(){
    state[0].s.ros1 = compute_ros(state[0].s.temp1,state[0].s.hum1);
    state[0].s.ros2 = compute_ros(state[0].s.temp2,state[0].s.hum2);
}

uint8_t settings[6];

void store_capteur_val(uint8_t nr, float val ) {
#ifdef DEBUG2
    Serial.print(nr);
    Serial.print('|');
    Serial.print(val);
    Serial.print('\n');
#endif

    if (((val <0) or (val >0) or (val=0))) {    /* XXX This is needed because hard to detect dht read errors ?? */
        if (first) {
            state[0].d[nr] = val;
            state[1].d[nr] = val;
            state[2].d[nr] = val;
        }
        float tmp;
        tmp = state[0].d[nr];
        state[0].d[nr] = ((tmp *3) + val)/4;
        state[1].d[nr] = ((state[1].d[nr] *15) + val)/16;
        state[2].d[nr] = ((state[2].d[nr] *255) + val)/256;
    }
}

#ifdef DEBUG
int stat[3];
int stat2[3];

void print_stat(int *stat){
    Serial.print("OK: ");
    Serial.print(stat[0]);
    Serial.print(" TO: ");
    Serial.print(stat[1]);
    Serial.print(" CS: ");
    Serial.print(stat[2]);
    Serial.print(" ");
    Serial.print(called);
    Serial.print("\n");

}
#endif

void lis_capteur() {
      store_capteur_val(O_HUM1, dht.getHumidity());
        lis_cmd();
    store_capteur_val(O_TEMP1,dht.getTemperature() + TEMP1_CAL);
#ifdef DEBUG
      stat[dht.getStatus()]++;
      print_stat(stat);
#endif
        lis_cmd();
    store_capteur_val(O_HUM2, dht2.getHumidity());
        lis_cmd();
    store_capteur_val(O_TEMP2,dht2.getTemperature());
    store_ros();
#ifdef DEBUG
      stat2[dht2.getStatus()]++;
      print_stat(stat2);
#endif
        lis_cmd();
}

struct cmd {
    uint8_t command;
    uint8_t key;
    uint32_t value;
};

void lis_cmd() {
while (Serial.available())
{
    while (Serial.available())
    {
        char c = Serial.read();
        buffer[cnt++] = c;
//        Serial.println(c);
        if ((c == '\n') || (c == ';') || (cnt == sizeof(buffer)-1))
        {
            buffer[cnt] = '\0';
            cnt = 0;
            ready = true;
            continue;
        }
     } 
     if (ready)
     {
        ParseLine();
        ready = false;
     }
}
}




void lis_analog() {

    digitalWrite(SOIL_EN, HIGH);
    delay(100);
    for(uint8_t pin=0;pin<ANALOG_PINS;pin++) {
        analog[pin] = analogRead(pin);
        delay(10);
        analog[pin] = analogRead(pin);
        lis_cmd();
        delay(1);
    }
    digitalWrite(SOIL_EN, LOW);
}

boolean red = false;
boolean synced = false;

void loop() {
   // int time = 

    //printTime();
    int period = now() % SAMPLING_PERIOD;
    if(((period==0) and not red) or first){
        if(first) {
            lis_analog();
            delay (2000);
            lis_capteur();
            delay (2000);
            first=false;
        }
        else{
            lis_capteur();
            lis_analog();
        }
        red = true;
        called++;
    }
    else if((period!=0) and red) {
        red = false;
    }
    lis_cmd();
    if((now()%3600)==0 and not synced){
        set_rtc(now());
        synced=true;
    }
    else {
        synced = false;
    }
    evaluate();

    //lis_capteur();
}



#define COND_LT (1<<0) // Lower than
#define COND_BT (1<<1) // Bigger than than
#define COND_AND (1<<2) // Logical And        -- then both ops are  other conditions ?
#define COND_OR (1<<3) // Logical Or
#define COND_T2 (1<<4) // Is type 2 ?
#define COND_MOD (1<<4) // Is modulo ?
#define COND_LV (1<<6) // Left operand is a var (est-ce toujours le cas ?)
#define COND_RV (1<<7) // Right operand is a var



struct condition_leaf {
 uint8_t flags;
 uint8_t left;
 int32_t res;
 int32_t right;
};

struct condition_node {
 uint8_t flags;
 uint8_t left;
 int32_t res;
 int32_t right;
};


struct condition_leaf_mod {
 uint8_t flags;
 uint8_t left;
 uint32_t modulo; // Use case above 86400 ?
 int32_t right;
};

union condition {
    struct condition_leaf leaf; 
    struct condition_node node;
    struct condition_leaf_mod leaf_mod;
    uint8_t data[sizeof( struct condition_leaf_mod)];
}; 

#define START_ADDR 128

condition cond;

int32_t get_operand(uint8_t op) {
  switch (op)
        {
            case O_NOW:
                return (int32_t) now();
        }
  return -1;
}

uint32_t booloperate(uint8_t flags, int32_t left, int32_t right) {
    /*
        Serial.print('\n');
        Serial.print("op");
        Serial.print(' ');
        Serial.print(flags);
        Serial.print(' ');
        Serial.print(left);
        Serial.print(' ');
        Serial.print(right);
        Serial.print('\n');
        */
    if ((flags & COND_LT)!=0) {
       // digitalWrite(3,LOW);
        return left < right ? 1:0;

    }
    else if ((flags & COND_BT)!=0) {
        return left > right ? 1:0;
    }
    else if ((flags & COND_AND) !=0) {
        return (result & (1 << left)) && (result & (1 << right)) ? 1:0;
    }
    else if ((flags & COND_OR) !=0) {
        return (result & (1 << left)) || (result & (1 << right)) ? 1:0;
    }
    else {
        return 0;
    }
//        Serial.print('\n');
}


void evaluate() {
    result = 0;
    for(uint8_t k=0;k<COND_NR;k++) {
        uint8_t l;
        for(l=0;l<sizeof(condition);l++) {
            cond.data[l] = EEPROM.read(START_ADDR + (k*sizeof(condition)) + l);
        }
/*
        Serial.print(k);
        Serial.print(' ');
        Serial.print(START_ADDR + (k*sizeof(condition)));
        Serial.print(' ');
        Serial.print(cond.leaf.flags);
        Serial.print(' ');
        Serial.print(cond.leaf_mod.left);
        Serial.print(' ');
        Serial.print(cond.leaf_mod.modulo);
        Serial.print(' ');
        Serial.print(cond.leaf_mod.right);
        Serial.print(' ');
        */
        if ((cond.leaf.flags  & COND_MOD) != 0){
            int32_t op = get_operand(cond.leaf_mod.left);
            result |= booloperate(cond.leaf.flags, op % cond.leaf_mod.modulo, cond.leaf_mod.right) << k;
        } else if ((cond.leaf.flags  & (COND_AND|COND_OR)) != 0){
            result |= booloperate(cond.leaf.flags, cond.node.left, cond.node.right) << k;
        } else if ((cond.leaf.flags  & (COND_LT|COND_BT)) != 0){
        }
    }

    if(((result >>2)&1)!=0) {
        digitalWrite(3,LOW);
            }
            else {
        digitalWrite(3,HIGH);
            }

}

