byte CHINA[8] = {
    100, //sensitivity
    10,  //threshold
    10,  //scan time
    30,  //mask time
    15,   //edge Threshold
    1,   // curve type(0-4)
    52,  //note of bow
    55   //note of edge
};

#include <hellodrum.h>
#include <MIDI.h>
#include <LiquidCrystal.h>

bool closingPedal = false;
bool closingPedal1 = false;
bool closedPedal = false;
unsigned long initScanPedal;
unsigned long endScanPedal;
unsigned long initScanChoke;
unsigned long endScanChoke;
int posicao_pedal = 0;
int posicao_pedal_anterior = 0;  
int sensor_hh_pedal_raw = 0;
int hh_threshold = 10;              //Valor mínimo para ser reconhecido como golpeado.
int mask_time = 10;                 //Tempo mínimo de espera para estar pronto para ser golpeado novamente. (ms)
bool hit = false;
bool chinaChoke = false;
int scanTime = 7;
int sensibilidade = 6;              //Valor que será dividido o valor original do piezo do prato de chimbal.
int sensorValue = 0;
int velocidade = 0;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
int midiByte = 0 ;

//Definicao dos pinos LCD
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); //(rs, en, d4, d5, d6, d7)

//Nome dos pads e porta conectada.
HelloDrum china(4);
HelloDrum aux2(6, 7);
HelloDrum aux3(8, 9);
HelloDrum aux4(10, 11);

//Definicao do pino analogico que os botoes estao conectados.
HelloDrumButtonLcdShield button(0); //A0

void setup()
{
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(38400);
  pinMode(A5, INPUT_PULLUP);
  

//Curva

  china.setCurve(CHINA[5]);

//Nomes que irao aparecer no display LCD. Necessario estar na mesma ordem que os pads foram nomeados.
  
  china.settingName("CHINA");
  aux2.settingName("AUX 2");
  aux3.settingName("AUX 3");
  aux4.settingName("AUX 4");

/*
//GRAVAR VALORES PADRÃO NA MEMÓRIA EPROMM, CASO DER PROBLEMA //
  //--> Descomentar o bloco inteiro, gravar no arduino, comentar novamente e gravar pela segunda vez.//
  china.initMemory();
  aux2.initMemory();
  aux3.initMemory();
  aux4.initMemory();
*/

  //Carregar configuracoes da EEPROM.
  //Necessario estar na mesma ordem que os pads foram nomeados.

  china.loadMemory();
  aux2.loadMemory();
  aux3.loadMemory();
  aux4.loadMemory();

//Mensagem de inicializacao.
  byte Sound[8] = {
  0b00001,
  0b00011,
  0b00101,
  0b01001,
  0b01001,
  0b01011,
  0b11011,
  0b11000
  };  
  lcd.createChar(0, Sound);  
  lcd.begin(16, 2);
  lcd.clear();  
  lcd.print("Bem vindo");
  lcd.setCursor(8, 1);
	lcd.write(byte(0));  
  lcd.setCursor(10, 0);
	lcd.write(byte(0));  
  lcd.setCursor(12, 1);
	lcd.write(byte(0));  
  lcd.setCursor(14, 0);
	lcd.write(byte(0));  
  lcd.setCursor(16, 1);
	lcd.write(byte(0));  
  lcd.setCursor(0, 1);
  lcd.print("BATERA!");
}

void loop()
{
  MIDI.turnThruOn();
  MIDI.read();
  if (MIDI.getType()==0xA0){
    int notaChoke = MIDI.getData1();
    MIDI.sendPolyPressure(notaChoke, 0, 10);
  }    

  /////////// 1. LCD & MODO DE EDICAO /////////////

  bool buttonPush = button.GetPushState();
  bool editStart = button.GetEditState();
  bool editDone = button.GetEditdoneState();
  bool display = button.GetDisplayState();

  char *padName = button.GetPadName();
  char *item = button.GetSettingItem();
  int settingValue = button.GetSettingValue();

  button.readButtonState();
  
  china.settingEnable();
  aux2.settingEnable();
  aux3.settingEnable();
  aux4.settingEnable();

  if (buttonPush == true)
  {
    lcd.clear();
    lcd.print(padName);
    lcd.setCursor(0, 1);
    lcd.print(item);
    lcd.setCursor(13, 1);
    lcd.print(settingValue);
  }

  if (editStart == true)
  {
    lcd.clear();
    lcd.print("EDICAO");
    delay(500);
    lcd.clear();
    lcd.print(padName);
    lcd.setCursor(0, 1);
    lcd.print(item);
    lcd.setCursor(13, 1);
    lcd.print(settingValue);
  }

  if (editDone == true)
  {
    lcd.clear();
    lcd.print("EDICAO OK!");
    delay(500);
    lcd.clear();
    lcd.print(padName);
    lcd.setCursor(0, 1);
    lcd.print(item);
    lcd.setCursor(13, 1);
    lcd.print(settingValue);
  }

  //Mostrar o PAD acionado e velocidade no LCD.
  if (display == true)
  {
    int velocity = button.GetVelocity();
    char *hitPad = button.GetHitPad();

    lcd.clear();
    lcd.print(hitPad);
    lcd.setCursor(0, 1);
    lcd.print(velocity);
  }

 //Declaracao do tipo de pad. 
  //china.singlePiezo(CHINA[0], CHINA[1], CHINA[2], CHINA[3]); //cymbal2zone(byte sens, byte thre, byte scan, byte mask);
  china.singlePiezo();
  aux2.dualPiezo(); 
  aux3.dualPiezo();
  aux4.dualPiezo();

//Leitura inicial do prato de chimbal.
  
  
  sensorValue = analogRead(A2)/sensibilidade;
  if (sensorValue >= 127){
        sensorValue = 127;
  }

//  Pedal do Chimbal. Leitura do sensor, determinação da posição e envio do sinal midi da posição.
  
  sensor_hh_pedal_raw = analogRead(A1);
  if (sensor_hh_pedal_raw <= 200){
    sensor_hh_pedal_raw = 200;
  }
  if (sensor_hh_pedal_raw >= 850){
    sensor_hh_pedal_raw = 850;
  }
  posicao_pedal = map(sensor_hh_pedal_raw, 200, 850, 1, 127);

  if (posicao_pedal <= 3){
    posicao_pedal = 0;    
  }
  if (posicao_pedal >= 120){
    posicao_pedal = 127;    
  }
  
  if (posicao_pedal != posicao_pedal_anterior)
    {
      MIDI.sendControlChange(4, posicao_pedal, 10);
      posicao_pedal_anterior = posicao_pedal;
    }

/*
  if(hit == false && closingPedal == true && closingPedal1 == true && closedPedal == true){
    int timeClosePedal = endScanPedal - initScanPedal;     
    int velocity = timeClosePedal;
    if (velocity <= 1)
    {
      velocity = 1;
    }

    if (velocity >= 127)
    {
      velocity = 127;
    }   
    velocity = 127 - velocity;
    MIDI.sendNoteOn(21, velocity, 10);
    MIDI.sendNoteOff(21, 0, 10);
    lcd.clear();
    lcd.print("Veloc. fech. pedal");
    lcd.setCursor(0, 1);
    lcd.print(velocity);    
    closingPedal = false;  
    closingPedal1 = false;
    hit = true;
    delay(mask_time);
  }
*/  
  
  if (sensorValue > hh_threshold &&  hit == false) {
    int peak1 = analogRead(A2)/sensibilidade;
    if (peak1 >= 127){                                  //Leitura inicial do sensor do prato chimbal.
        peak1 = 127;
    }
    velocidade = peak1;


    for (int i = 1; i < scanTime; i++) {
      delay(1);
      int peak = analogRead(A2/sensibilidade);
      if (peak >= 127){                               //Looping que faz a leitura de acordo com Scantime. Cada loop é de 1 ms.
        peak = 127;                                   //Salva o maior valor do sensor. (Valor de pico máximo).
      }      
      Serial.println(peak);
      if (peak > velocidade) {
        velocidade = peak;
      }
    }  
    hit = true;

    Serial.println("  ");
    Serial.print("velocidade MAX = ");      //Escrita em serial da intensidade máxima ao ser golpeado.
    Serial.print(velocidade);
    Serial.println("  ");  
  }
  

  //MIDI.sendControlChange(4, posicao_pedal, 10);
  
  
  if (hit == true) {
    switch (posicao_pedal)
    {
      case 0 ... 14:
        MIDI.sendNoteOn(17, velocidade, 10);  //(nota totalmente aberto, velocidade, canal)
        MIDI.sendNoteOff(17, 0, 10);
		    hit = false;
        lcd.clear();
        lcd.print("CHIMBAL   0% 17");
        lcd.setCursor(0, 1);
        lcd.print(velocidade);
        break;

      case 15 ... 39:
        MIDI.sendNoteOn(60, velocidade, 10);  //(nota parcialmente aberto, velocidade, canal)
        MIDI.sendNoteOff(60, 0, 10);
		    hit = false;
        lcd.clear();
        lcd.print("CHIMBAL  20% 60");
        lcd.setCursor(0, 1);
        lcd.print(velocidade);        
        break;

      case 40 ... 54:
        MIDI.sendNoteOn(26, velocidade, 10);  //(nota meio aberto, velocidade, canal)
        MIDI.sendNoteOff(26, 0, 10);
		    hit = false;
        lcd.clear();
        lcd.print("CHIMBAL  40% 26");
        lcd.setCursor(0, 1);
        lcd.print(velocidade);          
        break;

      case 55 ... 74:
        MIDI.sendNoteOn(25, velocidade, 10);  //(nota meio fechado, velocidade, canal)
        MIDI.sendNoteOff(25, 0, 10);
		    hit = false;
        lcd.clear();
        lcd.print("CHIMBAL  60% 25");
        lcd.setCursor(0, 1);
        lcd.print(velocidade);        
        break;

      case 75 ... 109:
        MIDI.sendNoteOn(24, velocidade, 10);  //(nota parcialmente fechado, velocidade, canal)
        MIDI.sendNoteOff(24, 0, 10);
		    hit = false;
        lcd.clear();
        lcd.print("CHIMBAL  80% 24");
        lcd.setCursor(0, 1);
        lcd.print(velocidade);        
        break;

      case 110 ... 127:
        MIDI.sendNoteOn(22, velocidade, 10);  //(nota totalmente fechado, velocidade, canal)
        MIDI.sendNoteOff(22, 0, 10);
		    hit = false;
        lcd.clear();
        lcd.print("CHIMBAL 100% 22");
        lcd.setCursor(0, 1);
        lcd.print(velocidade);        
        break;
    }
    delay(mask_time); //mask time  (Tempo de descanso após chimbal ser golpeado) .
  }
    
  sensorValue = analogRead(A2)/sensibilidade;  
  if (sensorValue <= hh_threshold && hit == true) {    //Habilitando sensor do chimbal novamente.
    hit = false;
  }  

  int aux1State = digitalRead(A5);
  if(aux1State == 0){
    chinaChoke = true;
  }
  else{
    chinaChoke = false;      
  }  
      
  //bow
  if (china.hit == true)
  {
    MIDI.sendNoteOn(china.note, china.velocity, 10); //(note, velocity, channel)
    MIDI.sendNoteOff(china.note, 0, 10);
  }

  //edge
  else if (china.hitRim == true)
  {
    MIDI.sendNoteOn(china.noteRim, china.velocity, 10); //(note, velocity, channel)
    MIDI.sendNoteOff(china.noteRim, 0, 10);
  }

  //choke
  if (chinaChoke == true)
  {
    MIDI.sendPolyPressure(CHINA[6], 127, 10);
    MIDI.sendPolyPressure(CHINA[7], 127, 10);
    MIDI.sendPolyPressure(CHINA[6], 0, 10);
    MIDI.sendPolyPressure(CHINA[7], 0, 10);
  }

  //aux2//
  if (aux2.hit == true)
  {
    MIDI.sendNoteOn(aux2.note, aux2.velocity, 10); //(nota, velocidade, canal)
    MIDI.sendNoteOff(aux2.note, 0, 10);
  } 

  //aux2_rim
  else if (aux2.hitRim == true)
  {
    MIDI.sendNoteOn(aux2.noteRim, aux2.velocity, 10); //(note, velocity, channel)
    MIDI.sendNoteOff(aux2.noteRim, 0, 10);
  }  

  //aux3//
  if (aux3.hit == true)
  {
    MIDI.sendNoteOn(aux3.note, aux3.velocity, 10); //(nota, velocidade, canal)
    MIDI.sendNoteOff(aux3.note, 0, 10);
  }  

  //aux3_rim
  else if (aux3.hitRim == true)
  {
    MIDI.sendNoteOn(aux3.noteRim, aux3.velocity, 10); //(note, velocity, channel)
    MIDI.sendNoteOff(aux3.noteRim, 0, 10);
  }   

  //aux4//
  if (aux4.hit == true)
  {
    MIDI.sendNoteOn(aux4.note, aux4.velocity, 10); //(nota, velocidade, canal)
    MIDI.sendNoteOff(aux4.note, 0, 10);
  }  
  
  //aux4_rim
  else if (aux4.hitRim == true)
  {
    MIDI.sendNoteOn(aux4.noteRim, aux4.velocity, 10); //(note, velocity, channel)
    MIDI.sendNoteOff(aux4.noteRim, 0, 10);
  }   
  
}