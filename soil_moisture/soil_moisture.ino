#include <avr/sleep.h>
#include <avr/wdt.h>

#include <Enerlib.h>
#include <DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <AM2320.h>

#define Moisture A0 //定义AO 引脚 为 IO-A0  
#define Moisture1 A1 //定义A1 引脚 为 IO-A1  
#define Moisture2 A2 //定义A2 引脚 为 IO-A2  
#define DO 7        //定义DO 引脚 为 IO-7  
#define D1 6        //定义DO 引脚 为 IO-6  
#define D2 5        //定义DO 引脚 为 IO-5  
#define DisDelay 5000        //displayTime

DS3231 Clock;
AM2320 th;
Energy energy;
LiquidCrystal_I2C lcd(0x3F, 16, 2);

volatile byte dataT=0;

volatile byte state = 0;

bool Century=false;
bool h12=false;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;
byte year, month, date, DoW, hour, minute, second;
char data[15];
char Str_year[3], Str_month[3], Str_date[3], Str_DoW[3], Str_hour[3], Str_minute[3], Str_second[3];
uint8_t bell[8] = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};


void setup() {  
  // Start the I2C interface
  pinMode(Moisture, INPUT);//定义A0为输入模式  
  pinMode(DO, INPUT); 
  pinMode(Moisture1, INPUT);//定义A0为输入模式  
  pinMode(D1, INPUT);
  pinMode(Moisture2, INPUT);//定义A0为输入模式  
  pinMode(D2, INPUT);
  Serial.begin(9600); 
  lcd.init(); 
  lcd.backlight(); 
  Serial.println("Please enter the time: year[2]month[2]date[2]DoW[2]hour[2]minute[2]second[2]");
  Serial.println("example: 2014-12-3 Wednesday 14:15:15 enter:14120303141515"); 


  setup_watchdog(9);
  // 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
  // 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
  ACSR |=_BV(ACD);//OFF ACD
  ADCSRA=0;//OFF ADC
  Sleep_avr();//Sleep_Mode

}  
  
void loop() {

  if(dataT>=7){
    dataT=0;
  //-------------------------------//此处是到达设置唤醒时间允许的程序
    setTime();
    delay(500);
    displayTime();     
    delay(DisDelay);
    displaySoilMoisture();  
    humidityAndTemperature();
  //--------------------------------    
    Sleep_avr();
  }
  else {
    Sleep_avr();  //Continue Sleep
  }


}  

//Sleep mode is activated
void setup_watchdog(int ii) {

  byte bb;

  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);


}
//WDT interrupt
ISR(WDT_vect) {

  ++dataT;
// wdt_reset();

}

void Sleep_avr(){
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
}


//Energy
void wakeISR() {
    if (energy.WasSleeping()) {
        state = 1;
    }
    else {
        state = 2;
    }
}

void checkState() {
    if (state == 1) {
        Serial.println("Was sleeping...");
    }
    else if (state == 2) {
        Serial.println("Was awake...");
    }
    state = 0;
}

void humidityAndTemperature() {
    char line0[32];
    char line1[32];
    int  temperature;
    switch (th.Read()) {
        case 2:
            temperature = Clock.getTemperature();
            sprintf(line0, "CRC failed");
            sprintf(line1, "Temp. = %2d", (int)temperature);
            lcdTemprature(line0,line1);
            break;
        case 1:
            temperature = Clock.getTemperature();
            sprintf(line0, "Sensor offline");
            sprintf(line1, "Temp. = %2d", (int)temperature);
            lcdTemprature(line0,line1);
            break;
        case 0:
            sprintf(line0, "Humidity = %2d", (int)th.h);
            sprintf(line1, "Temp. = %2d", (int)th.t);
            lcdTemprature(line0,line1);
            break;
    }
    delay(DisDelay);
}
void lcdTemprature(char* line0, char* line1)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line0);
  lcd.setCursor(0, 1);
  lcd.print(line1);
  lcd.write(0xdf);  // 显示温度单位
  lcd.print("C");
}

void displaySoilMoisture(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moisture1 = ");  
  lcd.print(analogRead(Moisture),DEC);//读取AO的数值  
  lcd.setCursor(0, 1); 
  lcd.print("|DO1 = ");  
  lcd.print(digitalRead(DO),DEC);//读取DO的数值 
  delay(DisDelay);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moisture2 = ");
  lcd.print(analogRead(Moisture1), DEC);//读取A1的数值  
  lcd.setCursor(0, 1);
  lcd.print("|DO2 = ");
  lcd.print(digitalRead(D1), DEC);//读取D1的数值 
  delay(DisDelay);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moisture3 = ");
  lcd.print(analogRead(Moisture2), DEC);//读取A2的数值  
  lcd.setCursor(0, 1);
  lcd.print("|DO3 = ");
  lcd.print(digitalRead(D2), DEC);//读取D2的数值 
  delay(DisDelay);
}

void setTime(){
   if (Serial.available() >=14)     //串口读取数据
   {
      for (int a = 0; a <14; a++)
       {
       data[a] = Serial.read();
       }
       Str_year[0] = data[0];    //拆包
       Str_year[1] = data[1];
       Str_month[0] = data[2];
       Str_month[1] = data[3];
       Str_date[0] = data[4];
       Str_date[1] = data[5];
       Str_DoW[0] = data[6];
       Str_DoW[1] = data[7];
       Str_hour[0] = data[8];
       Str_hour[1] = data[9];
       Str_minute[0] = data[10];
       Str_minute[1] = data[11];
       Str_second[0] = data[12];
       Str_second[1] = data[13];

       //Str to byte
       year = atoi(Str_year);    //转换数据类型
       month = atoi(Str_month);
       date = atoi(Str_date);
       DoW = atoi(Str_DoW);
       hour = atoi(Str_hour);
       minute = atoi(Str_minute);
       second = atoi(Str_second);
    
       Clock.setSecond(second);//Set the second 
       Clock.setMinute(minute);//Set the minute 
       Clock.setHour(hour);  //Set the hour 
       Clock.setDoW(DoW);    //Set the day of the week
       Clock.setDate(date);  //Set the date of the month
       Clock.setMonth(month);  //Set the month of the year
       Clock.setYear(year);  //Set the year (Last two digits of the year)     
  }              
}

void displayTime(){
   lcd.clear();
   int second,minute,hour,date,month,dow,year,temperature;
   second= Clock.getSecond();
   minute=Clock.getMinute();
   hour=Clock.getHour(h12, PM);
   date=Clock.getDate();
   month=Clock.getMonth(Century);
   year=Clock.getYear();
   dow=Clock.getDoW();
   temperature=Clock.getTemperature();
   
   Serial.print("20");
   Serial.print(year,DEC);
   Serial.print('-');
   Serial.print(month,DEC);
   Serial.print('-');
   Serial.print(date,DEC);
   Serial.print(' ');
   Serial.print(hour,DEC);
   Serial.print(':');
   Serial.print(minute,DEC);
   Serial.print(':');
   Serial.print(second,DEC);
   Serial.print('\n');
   Serial.print("Temperature=");
   Serial.print(temperature); 
   Serial.print('\n');
   
  lcd.setCursor(0, 0);
   lcd.print("20");  // 显示20世纪
     if (year>=10)  // 显示年份
       {
       lcd.print(year,DEC);
       }
       else
       {
       lcd.print("0");
       lcd.print(year,DEC);
       }
   lcd.print('-');
  
   lcd.setCursor(5, 0);
     if (month>=10)  // 显示月份
       {
       lcd.print(month,DEC);
       }
       else
       {
       lcd.print("0");
       lcd.print(month,DEC);
       }
     lcd.print('-');
    
     lcd.setCursor(8, 0);
       if (date>=10)  // 显示日期
         {
         lcd.print(date,DEC);
         }
         else
         {
         lcd.print("0");
         lcd.print(date,DEC);
         }
    
     lcd.setCursor(11, 0);
       switch (dow)  // 显示星期
         {
         case 1:  // 当dow等于1时，执行以下语句
           lcd.print("Mon");
           break;
         case 2:  // 当dow等于2时，执行以下语句
           lcd.print("Tue");
           break;
         case 3:
           lcd.print("Wed");
           break;
         case 4:
           lcd.print("Thu");
           break;
         case 5:
           lcd.print("Fri");
           break;
         case 6:
           lcd.print("Sat");
           break;
         case 7:
           lcd.print("Sun");
           break;
         }
    
     lcd.setCursor(0, 1);  //光标移至第2行
       if (hour>=10)  // 显示小时
         {
         lcd.print(hour,DEC);
         }
         else
         {
         lcd.print("0");
         lcd.print(hour,DEC);
         }
     lcd.print(':');
    
     lcd.setCursor(3, 1);
       if (minute>=10)  // 显示分钟
         {
         lcd.print(minute,DEC);
         }
         else
         {
         lcd.print("0");
         lcd.print(minute,DEC);
         }
     lcd.print(':');
    
     lcd.setCursor(6, 1);
       if (second>=10)  // 显示秒钟
         {
         lcd.print(second,DEC);
         }
         else
         {
         lcd.print("0");
         lcd.print(second,DEC);
         }
    
     lcd.setCursor(9, 1);
     if (Clock.checkAlarmEnabled(1))  // 显示闹钟标识
       {
       lcd.write(0x00);
       }
       else
       {
       lcd.print(' ');
       }
    
     lcd.setCursor(12, 1);
     lcd.print(temperature);  // 显示温度
     lcd.write(0xdf);  // 显示温度单位
     lcd.print("C");
}
