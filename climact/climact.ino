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

//#define DEBUG_RULES 1
//#define DEBUG_SENSORS 1
//#define DEBUG_ACT 1
//#define DEBUG_FAST 1
//#define DEBUG_RULES 1
//#define DEBUG_CONF 1

#define COND_NR 16
uint32_t result;
time_t right_now;

uint8_t time_is_set = 0;

#define TEMP1_CAL -0.5
 // *********************************************
// VARIABLES
// *********************************************

#define HZ 100
#define SAMPLING_PERIOD  10 //seconds. Minimum 2s for DHT22

uint8_t relay_conf_red = 0;

#ifdef ARDUINO_ARCH_AVR

#define VOLTS 4.72

const uint8_t relay_pins[] = {3,4};

#define DHTPIN 8
#define DHT2PIN 9

#define SOIL_EN 10

#define HBRIDGE_2 11
#define HBRIDGE_1 12

#define HALL_IN 4


#define kCePin    5  // Chip Enable
#define kIoPin    6  // Input/Output
#define kSclkPin  7  // Serial Clock
// Create a DS1302 object.
DS1302 rtc(kCePin, kIoPin, kSclkPin);

#define ADC_RES_BITS 10

#elif ARDUINO_ARCH_STM32F1

#include <RTClock.h>
RTClock rtc (RTCSEL_LSE);

#define VOLTS 3.3

const uint8_t relay_pins[] = {3,4};

#define DHTPIN PB15
#define DHT2PIN PB11

#define SOIL_EN 10

#define HBRIDGE_2 PB8
#define HBRIDGE_1 PB9

#define HALL_IN 4
#define ADC_RES_BITS 12

#endif

time_t next_watering;
uint8_t water_seconds = 0;
uint32_t water_interval = 3600;
uint32_t water_limit = 60;
float water_total = 0;


#define AC_HZ 10
 #define HALL_SENS 185      // mV/A , 5A  acs712 hall sensor
// #define HALL_SENS 100   // mV/A , 20A acs712 hall sensor
//#define HALL_SENS 66    // mV/A , 30A acs712 hall sensor
float current_amp = 0;
int amp_filter = 2; // samples averaged as a crude low-pass

#define AMP_BUFF 8
double amp_buff[AMP_BUFF];
int8_t amp_buff_ready = 0;

long sensors_read_count = 0;

#define ANALOG_PINS 4
int analog[ANALOG_PINS];



DHT dht,dht2;
#define DHTTYPE DHT22

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




#define RELAY_ADDR 64


#define TYPE_LIGHT  1 << 0
#define TYPE_WAIT   1 << 1   // Device needs 30 min. wait between state change, e.g. HPS
#define TYPE_TEMP_UP   1 << 2   // Device warms the place
#define TYPE_TEMP_DOWN   1 << 3   // Device cools the place
#define TYPE_HUM_UP   1 << 4   // Device ups humidity level
#define TYPE_HUM_DOWN   1 << 5   // Device downs humidity level
#define TYPE_PROBESTART    1 << 6   // Device can fail to start.
                                 // Current consumption will be checked
                                 // and device restarted when needed ..


#ifdef DEBUG_FAST
#define TYPE_WAIT_SECS 1 // min. time between state change
#else
#define TYPE_WAIT_SECS 1200 // min. time between state change
#endif

struct relay {
   uint8_t pin;
   uint8_t state;
   uint8_t type;
   int8_t rule;
   time_t lastchange;
};

union relay_store {
    struct relay r;
    uint8_t d[sizeof(struct relay)];
    }

relays[2];

struct relay_dat {
    float amp_on;
    float amp_off;
    float amp_exp;
    float amp_before;
    time_t time;
    uint8_t step;
    int8_t ready;
    }
relays_dat[2];


#define STEP_STARTING 1
#define STEP_WAITING 2
#define STEP_PROBING 3
#define STEP_FAIL 3
#define STEP_READY 8

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


void read_relay_conf(){
    if(!relay_conf_red) {

      for(uint8_t r=0;r<sizeof(relay_pins);r++) {
        for(uint8_t l=0;l<sizeof(relay_store);l++) {
                relays[r].d[l] = EEPROM.read(RELAY_ADDR + (r*sizeof(relay_store)) + l);
        }
        relays[r].r.pin=relay_pins[r];

        relays[r].r.state = 0;           // Avoid intempestive click on reset..
        pinMode(relay_pins[r], OUTPUT); // 
        digitalWrite(relays[r].r.pin, relays[r].r.state ? LOW:HIGH);

        relays[r].r.lastchange=now();
        relays_dat[r].step = STEP_READY;
        relays_dat[r].ready = !0;
        relays_dat[r].amp_on = 0;
        relays_dat[r].amp_off = 0;
        relays_dat[r].amp_exp = 0.6;

#ifdef DEBUG_CONF
        Serial.print("DBG:RELAY:Red conf for relay ");
        Serial.print(r);
        Serial.print(" pin ");
        Serial.print(relays[r].r.pin);
        Serial.print(" type ");
        Serial.print(relays[r].r.type);
        Serial.print(" state ");
        Serial.print(relays[r].r.state);
        Serial.print(" rule ");
        Serial.print(relays[r].r.rule);
        Serial.print(" lastchange ");
        Serial.print(relays[r].r.lastchange);
        Serial.print('\n');
#endif
        
    //    digitalWrite(relay_pins[r], relays[r].r.state ? LOW:HIGH);

      }
    relay_conf_red=1;
  }


}


void setup() {
  //Serial.begin(115200);
  Serial.begin(9600);

//  memset(timers,0,sizeof(timers));

  dht.setup(DHTPIN);
  dht2.setup(DHT2PIN);

  //analogReadResolution(ADC_RES_BITS);


#ifdef ARDUINO_ARCH_AVR
  rtc.writeProtect(false);
  rtc.halt(false);
  Time t = rtc.time(); // We get time from rtc
  setTime(t.hr, t.min, t.sec, t.date, t.mon, t.yr); // And set our "system time"

  pinMode(13, OUTPUT); // Led set off
  digitalWrite(13, LOW);

#elif ARDUINO_ARCH_STM32F1
  
#endif



  pinMode(SOIL_EN, OUTPUT); // Dont throw current to roots allthetime
  digitalWrite(SOIL_EN, LOW);

  pinMode(HBRIDGE_1, OUTPUT); // 
  digitalWrite(HBRIDGE_1, LOW);

  pinMode(HBRIDGE_2, OUTPUT); // 
  digitalWrite(HBRIDGE_2, LOW);


  Serial.print("Hello i'm Perceptor v0.1\n");

  /*
  open_gardena_solenoid();
  delay(2000);
  close_gardena_solenoid();
  */

  amp_buff_ready = 0;
  relay_conf_red = 0;

#ifdef DEBUG_CONF
  log_rules();
#endif
  Serial.print("DBG:RESTART:\n");

  log_event("restart",0,0,-1);

//    evaluate();
}

void irrigate() {
    int32_t microsecs = water_seconds;
    if (microsecs < 20) {
        microsecs = microsecs * 1000;
    }
    if (microsecs > 20000) {
        microsecs = 20000;
    }
    if(microsecs > 0) {
        open_gardena_solenoid();
        water_total = water_total + (microsecs / 1000);
        while (microsecs>0) {
            delay(100);
            microsecs = microsecs - 100;
        }
        close_gardena_solenoid();
    }
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
#ifdef ARDUINO_ARCH_AVR

    tmElements_t tm;
    breakTime(t,tm);
    Time rt(tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second, Time::kSunday);
    rtc.time(rt);
#elif ARDUINO_ARCH_STM32F1
    rtc.setTime(t);
#endif
}


char _result_str[COND_NR+1];
char * result_str() {
    for (uint8_t iter=0;iter<COND_NR;iter++) {
        _result_str[iter] = ((result >> iter) & 1) ? '1':'0';
    }
    _result_str[COND_NR]='\0';
    return _result_str;
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

                Serial.print("address ");
                Serial.print(address);
                    Serial.print("\n");
                value +=3;
                if(is_write) {
                    while((value[0] != '\n') && (value[0]!='\0') && (value[1] != '\n') && (value[1]!='\0')){
                        uint8_t v = ctoi(value[0]) * 16  + ctoi(value[1]) ;
#ifdef ARDUINO_ARCH_AVR
                        EEPROM.update(address++, v);
#else
                        EEPROM.write(address++, v);
#endif
                        value+=2;
                        c++;
                    }
                    relay_conf_red = 0;
                    read_relay_conf();
                } else {
                    c=32;
                    address += c;
                }

                Serial.print(is_write ? "W" : "R");
                Serial.print("=");
                Serial.print(address-c);
                for (uint8_t m=0;m<c;m++){
                    print_uint8 (EEPROM.read((address -c)+m));
                }
                Serial.print("\n");
                break;
            case 'U':
                t = strtoul(value,NULL,0);
                setTime(t);
                set_rtc(t);
                right_now = now();
                time_is_set = 1;
                Serial.print("U=");
                Serial.print(now());
                Serial.print("\n");
                break;
            case 'P':
                if((val>=0) && (val < 20)) {
                    water_seconds = val;
                    out_stat();
                }
                break;
            case 'I':
                t = strtoul(value,NULL,0);
                if((t>=60) && (t < 2000000)) {
                    water_interval = t;
                    out_stat();
                }
                break;
            case 'L':
                t = strtoul(value,NULL,0);
                if((t>=60) && (t < 2000000)) {
                    water_limit = t;
                    out_stat();
                }
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
                 Serial.print(result_str());
                 Serial.print("\n");
                 break;
            case 'D':
                 log_rules();
                 log_relays();
                 //print_stat();
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


void log_event(char *name, int32_t p1, int32_t p2, int32_t p3) {
                 Serial.print('\n');
                 Serial.print("EV:");
                 Serial.print(right_now);
                 Serial.print(",");
                 Serial.print(name);
                 Serial.print(",");
                 Serial.print(result_str());
                 Serial.print(",");
                 Serial.print(p1);
                 Serial.print(",");
                 Serial.print(p2);
                 Serial.print(",");
                 Serial.print(p3);
                 Serial.print('\n');
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

#ifdef DEBUG_SENSORS
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
    Serial.print(sensors_read_count);
    Serial.print("\n");

}
#endif

void lis_capteur() {
      store_capteur_val(O_HUM1, dht.getHumidity());
        lis_cmd();
    store_capteur_val(O_TEMP1,dht.getTemperature() + TEMP1_CAL);
#ifdef DEBUG_SENSORS
      stat[dht.getStatus()]++;
      print_stat(stat);
#endif
      if(dht.getStatus() >0){
          log_event("sensor",0,dht.getStatus(),-1);
      }

        lis_cmd();
    store_capteur_val(O_HUM2, dht2.getHumidity());
        lis_cmd();
    store_capteur_val(O_TEMP2,dht2.getTemperature());
    store_ros();
#ifdef DEBUG_SENSORS
      stat2[dht2.getStatus()]++;
      print_stat(stat2);
#endif
      if(dht2.getStatus() >0){
          log_event("sensor",1,dht2.getStatus(),-1);
      }
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

void lis_hall_amp() {

        double tmp, tmp_raw, tmp_avg, tmp_max;
        tmp = analogRead(HALL_IN); // discard first reading
        double avg_total = 0;
        delay(10);
        uint32_t end_time = micros() + (1000000/AC_HZ) ; // Sample a bit more than a whole 50/60 Hz period

        tmp_avg = ((double)analogRead(HALL_IN)) - (2.5 * ( (1<<ADC_RES_BITS) / VOLTS) );
        tmp_max = 0;
        uint32_t cur_time = micros();
        uint32_t total_time = 0;
        uint32_t last_time;
        while((cur_time <= end_time) || (( cur_time - end_time ) > 1000000000 )) {
            tmp_raw = ((double)analogRead(HALL_IN)) - ((VOLTS/2.0) * ( (1<<ADC_RES_BITS) / VOLTS) ); // offset to 0
            tmp_avg = ((tmp_avg * amp_filter) + tmp_raw) / (amp_filter + 1) ; // crude low-pass
            tmp = (tmp_avg <0) ? -tmp_avg : tmp_avg;

            tmp_max = tmp > tmp_max ? tmp : tmp_max;
            last_time=cur_time;
            cur_time = micros();
            if((cur_time-last_time) < 1000000){
                avg_total += abs(tmp_raw) * (cur_time-last_time);
                total_time += cur_time-last_time ;
            }
        }
//        current_amp_max = tmp_max  ;
//        current_amp = (avg_total / total_time) * (VOLTS / (1<<ADC_RES_BITS)) ;
        current_amp =( (avg_total / total_time) * (VOLTS / (1<<ADC_RES_BITS))) / (HALL_SENS /1000.0) ;
        amp_buff[(sensors_read_count +1) % AMP_BUFF]=current_amp;
        if(sensors_read_count >= AMP_BUFF -1) {
            amp_buff_ready = !0;
        }
}




void lis_analog() {

    digitalWrite(SOIL_EN, HIGH);
    delay(20);
    lis_cmd();
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
boolean rtc_synced = false;

void out_stat(){
     if (time_is_set==1){
         Serial.print("ST:");
         Serial.print (right_now);
         Serial.print (',');
         Serial.print(state[0].d[O_TEMP1]);
         Serial.print(',');
         Serial.print(state[0].s.hum1);
         Serial.print(',');
         delay(100);
         Serial.print(state[0].s.temp2);
         Serial.print(',');
         Serial.print(state[0].s.hum2);
         Serial.print(',');
         Serial.print(state[0].s.ros1);
         Serial.print(',');
         delay(100);
         Serial.print(state[0].s.ros2);
         Serial.print(',');
         Serial.print(analog[0]);
         Serial.print(',');
         Serial.print(analog[1]);
         Serial.print(',');
         delay(100);
         Serial.print(analog[2]);
         Serial.print(',');
         Serial.print(analog[3]);
         Serial.print(',');
         Serial.print(current_amp);
         delay(100);
         Serial.print(',');
         Serial.print(0);
         Serial.print(',');
         Serial.print(water_total);
         Serial.print(',');
         Serial.print(water_seconds);
         Serial.print(',');
         Serial.print(water_interval);
         Serial.print('\n');
     }
}

void loop() {
   // int time = 

    //printTime();
    int period = now() % SAMPLING_PERIOD;
    if(((period==0) and not red) or first){
        if(first) {
            lis_analog();
            delay (600);
            lis_capteur();
            delay (1600);
            lis_hall_amp();
            first=false;
        }
        else{
            lis_capteur();
            lis_analog();
            lis_hall_amp();
        }
        red = true;
        if((sensors_read_count % 8)==0){
            out_stat();
        }
        sensors_read_count++;
    }
    else if((period!=0) and red) {
        red = false;
    }
    lis_cmd();
    if (time_is_set==1) {
#ifdef ARDUINO_ARCH_AVR
        if((now()%3600)==0 and not rtc_synced){
            set_rtc(now());
            rtc_synced=true;
        }
        else {
            rtc_synced = false;
        }
#endif

        right_now = now();
        read_relay_conf();
        evaluate();
        actuate_relays();
        follow_relay_state();

        delay(100);

        if((right_now > next_watering) && (relays[1].r.state == 0)) {
            irrigate();
            next_watering = right_now + water_interval;
        }

        /*
        if(((right_now % 86400) > (3600*6)) && ((right_now % 86400) < (3600*16))) {
            digitalWrite(4,LOW);
        } else {
            digitalWrite(4,HIGH);
        }
        */

    }
    //lis_capteur();
}



#define COND_LT (1<<0) // Lower than
#define COND_BT (1<<1) // Bigger than than
#define COND_AND (1<<2) // Logical And        -- then both ops are  other conditions ?
#define COND_OR (1<<3) // Logical Or
#define COND_T2 (1<<4) // Is type 2 ?
#define COND_MOD (1<<5) // Is modulo ?
#define COND_NOT (1<<6) // Invert result
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
#ifdef DEBUG_FAST
                return (int32_t) right_now*(86400/60);  // Plays 1 day in 1 minute. Caution: no lamp protection in this mode, unplug them first !
#else
                return (int32_t) right_now;
#endif
        }
  return -1;
}

uint32_t booloperate(uint8_t flags, int32_t left, int32_t right) {
#ifdef DEBUG_RULES
        Serial.print('\n');
        Serial.print("op");
        Serial.print(' ');
        Serial.print(flags);
        Serial.print(' ');
        Serial.print(left);
        Serial.print(' ');
        Serial.print(right);
        Serial.print('\n');
        delay(300);
#endif
    uint32_t rtrue, rfalse;
    if ((flags & COND_NOT)!=0) {
        rtrue = 0;
        rfalse = 1;
    } else {
        rtrue = 1;
        rfalse = 0;
    }
    if ((flags & COND_LT)!=0) {
       // digitalWrite(3,LOW);
        return (left < right) ? rtrue:rfalse;

    }
    else if ((flags & COND_BT)!=0) {
        return (left > right) ? rtrue:rfalse;
    }
    else if ((flags & COND_AND) !=0) {
#ifdef DEBUG_RULES
        Serial.print("l ");
        Serial.print(result & (1 << left));
        Serial.print(" r ");
        Serial.print(result & (1 << right));
        Serial.print('\n');
#endif
        return ((result & (1 << left)) !=0) && ((result & (1 << right)) != 0) ? rtrue:rfalse;
    }
    else if ((flags & COND_OR) !=0) {
        return ((result & (1 << left)) != 0) || ((result & (1 << right)) != 0) ? rtrue:rfalse;
    }
    else {
        return 0;
    }
}


void evaluate() {
    result = 0;
//    right_now = now();
    for(uint8_t k=0;k<COND_NR;k++) {
        uint8_t l;
        for(l=0;l<sizeof(condition);l++) {
            cond.data[l] = EEPROM.read(START_ADDR + (k*sizeof(condition)) + l);
        }
#ifdef DEBUG_RULES
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
        Serial.print(cond.node.right);
        Serial.print(' ');
        Serial.print(right_now);
        Serial.print(' ');
#endif
        if ((cond.leaf.flags  & COND_MOD) != 0){
            int32_t op = get_operand(cond.leaf_mod.left);
            result |= booloperate(cond.leaf.flags, op % cond.leaf_mod.modulo, cond.leaf_mod.right) << k;
        } else if ((cond.leaf.flags  & (COND_AND|COND_OR)) != 0){
            result |= booloperate(cond.leaf.flags, cond.node.left, cond.node.right) << k;
        } else if ((cond.leaf.flags  & (COND_LT|COND_BT)) != 0){
            int32_t op = get_operand(cond.leaf_mod.left);
            result |= booloperate(cond.leaf.flags, op, cond.leaf_mod.right) << k;
        }
#ifdef DEBUG_RULES
                 for (uint8_t iter=0;iter<COND_NR;iter++) {
                    Serial.print((result >> iter) & 1);
                 }
        Serial.print('\n');
#endif
    }
}

void actuate_relays() {
  for(uint8_t i=0;i<sizeof(relay_pins);i++) {
     relay *r = &relays[i].r;
     relay_dat *rd = &relays_dat[i];

#ifdef DEBUG_ACT
     delay(200);
     Serial.print("E");
                 for (uint8_t iter=0;iter<COND_NR;iter++) {
                    Serial.print((result >> iter) & 1);
                 }


             Serial.print(" r ");
             Serial.print(i);
             Serial.print(" ru ");
             Serial.print(r->rule);
             Serial.print('\n');
#endif
     if(r->rule>=0){

         uint8_t new_state = ((result & (1<< r->rule))!=0) ? 1:0;
#ifdef DEBUG_ACT
             Serial.print(" st ");
             Serial.print(r->state);
             Serial.print(" new ");
             Serial.print(new_state);
             Serial.print('\n');
#endif
         if ((r->state != new_state) && rd->ready ) {
             if(((r->type& TYPE_WAIT) == 0) || (r->lastchange < now() - TYPE_WAIT_SECS) ) {
//                 right_now = now();
                 
                 log_event("relay",r->pin,new_state,right_now - r->lastchange) ;
                 r->state = new_state;
                 set_relay_state(i);
//                 digitalWrite(r->pin,r->state?LOW:HIGH);
                 r->lastchange = right_now;

                 if ((r->type& TYPE_WAIT) != 0) {
#ifndef DEBUG_FAST
                     int adress = RELAY_ADDR + i * sizeof(relay);
#ifdef ARDUINO_ARCH_AVR
                     EEPROM.update (adress + 1, relays[i].d[1]);
                     EEPROM.update (adress + 4, relays[i].d[4]);
                     EEPROM.update (adress + 5, relays[i].d[5]);
                     EEPROM.update (adress + 6, relays[i].d[6]);
                     EEPROM.update (adress + 7, relays[i].d[7]);
#else
                     EEPROM.write (adress + 1, relays[i].d[1]);
                     EEPROM.write (adress + 4, relays[i].d[4]);
                     EEPROM.write (adress + 5, relays[i].d[5]);
                     EEPROM.write (adress + 6, relays[i].d[6]);
                     EEPROM.write (adress + 7, relays[i].d[7]);

#endif
#endif
                 }
                 delay(100);
             }
        }
     }
  }
}

void set_relay_state(uint8_t i) {
    uint8_t state = relays[i].r.state;
    if (!state) {
        float last_amp = current_amp;
        digitalWrite(relays[i].r.pin,HIGH);
        lis_hall_amp();
        relays_dat[i].amp_off = last_amp - current_amp;
    } else {
        relays_dat[i].step = STEP_WAITING;
        relays_dat[i].time = now();
    }
}

time_t last_start = 0;

void follow_relay_state(){
    uint8_t has_probe = 0;
    for (uint8_t i = 0;i<sizeof(relay_pins);i++) {
        relay *r = &relays[i].r;
        relay_dat *rd = &relays_dat[i];
        if (rd->step == STEP_PROBING) {
            has_probe = !0;
            if((right_now - rd->time) > 60) {
                rd->amp_on = current_amp - rd->amp_before;
                rd->step = STEP_READY;
                has_probe = 0;
                if((r->type & TYPE_PROBESTART) == 0) {
                    if(rd->amp_on < rd->amp_exp) {
                        rd->step = STEP_FAIL;
                        rd->time = right_now;
                        digitalWrite(relays[i].r.pin,HIGH);
                        lis_hall_amp();

                        log_event("probe_fail",relays[i].r.pin,i,0);
                    }
                    else {
                        log_event("probe_success",relays[i].r.pin,i,0);
                    }
                }
            }
        }
    }
    for (uint8_t i = 0;i<sizeof(relay_pins);i++) {
        relay *r = &relays[i].r;
        relay_dat *rd = &relays_dat[i];
        if ((rd->step == STEP_WAITING) && ((right_now - rd->time) > 2) && (!has_probe) ) {
            //lis_hall_amp();
            rd->amp_before = current_amp;
            digitalWrite(relays[i].r.pin,LOW);
            rd->time = right_now;
            //last_start= right_now;
            rd->step = STEP_PROBING;
            has_probe = !0;
        } else if ((rd->step == STEP_FAIL) && ((right_now - rd->time) > 30) && (!has_probe)  ) {
                rd->step = STEP_WAITING;
                rd->time = right_now;
        }
    }

}

void log_rules() {
    for(uint8_t k=0;k<COND_NR;k++) {
        uint8_t l;
        for(l=0;l<sizeof(condition);l++) {
            cond.data[l] = EEPROM.read(START_ADDR + (k*sizeof(condition)) + l);
        }
        Serial.print("DBG:RULE:");
        Serial.print(k);
        Serial.print(" addr=");
        Serial.print(START_ADDR + (k*sizeof(condition)));
        Serial.print(" flags=");
        Serial.print(cond.leaf.flags);
        Serial.print(" left=");
        Serial.print(cond.leaf_mod.left);
        Serial.print(" mod=");
        Serial.print(cond.leaf_mod.modulo);
        Serial.print(" mright=");
        Serial.print(cond.leaf_mod.right);
        Serial.print(" nleft=");
        Serial.print(cond.node.left);
        Serial.print(" nright=");
        Serial.print(cond.node.right);
        Serial.print('\n');
    }
}

void log_result() {
        Serial.print("DBG:RESULT:");
        for (uint8_t iter=0;iter<COND_NR;iter++) {
            Serial.print((result >> iter) & 1);
            if(iter!=COND_NR-1)
                Serial.print(',');
        }
        Serial.print('\n');
}

void log_relays() {
  for(uint8_t i=0;i<sizeof(relay_pins);i++) {
     relay *r = &relays[i].r;
     Serial.print("DBG:RELAY:");
     Serial.print(i);
     Serial.print(',');
     Serial.print(r->pin);
     Serial.print(',');
     Serial.print(r->type);
     Serial.print(',');
     Serial.print(r->rule);
     Serial.print(',');
     Serial.print(r->state);
     Serial.print(',');
     Serial.print(r->lastchange);
     Serial.print('\n');
  }
}
