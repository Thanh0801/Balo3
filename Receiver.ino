#include <SoftwareSerial.h>
#include "E32-TTL-100.h"

#define M0_PIN  8
#define M1_PIN  9
#define AUX_PIN A0
#define SOFT_RX 11
#define SOFT_TX 10

SoftwareSerial softSerial(SOFT_RX, SOFT_TX);  // LORA RX, LORA TX

//=== AUX ===========================================+
bool AUX_HL;
bool ReadAUX()
{
  int val = analogRead(AUX_PIN);

  if (val < 50)
  {
    AUX_HL = LOW;
  } else {
    AUX_HL = HIGH;
  }

  return AUX_HL;
}

//return default status
RET_STATUS WaitAUX_H()
{
  RET_STATUS STATUS = RET_SUCCESS;

  uint8_t cnt = 0;
  uint8_t data_buf[100], data_len;

  while ((ReadAUX() == LOW) && (cnt++ < TIME_OUT_CNT))
  {
    //Serial.print(".");
    delay(100);
  }

  if (cnt == 0)
  {
  }
  else if (cnt >= TIME_OUT_CNT)
  {
    STATUS = RET_TIMEOUT;
    //Serial.println(" TimeOut");
  }
  else
  {
    //Serial.println("");
  }

  return STATUS;
}
//=== AUX ===========================================-
//=== Mode Select ===================================+
bool chkModeSame(MODE_TYPE mode)
{
  static MODE_TYPE pre_mode = MODE_INIT;

  if (pre_mode == mode)
  {
    //Serial.print("SwitchMode: (no need to switch) ");  Serial.println(mode, HEX);
    return true;
  }
  else
  {
    //Serial.print("SwitchMode: from ");  Serial.print(pre_mode, HEX);  Serial.print(" to ");  Serial.println(mode, HEX);
    pre_mode = mode;
    return false;
  }
}

void SwitchMode(MODE_TYPE mode)
{
  if (!chkModeSame(mode))
  {
    WaitAUX_H();

    switch (mode)
    {
      case MODE_0_NORMAL:
        // Mode 0 | normal operation
        digitalWrite(M0_PIN, LOW);
        digitalWrite(M1_PIN, LOW);
        break;
      case MODE_1_WAKE_UP:
        digitalWrite(M0_PIN, HIGH);
        digitalWrite(M1_PIN, LOW);
        break;
      case MODE_2_POWER_SAVIN:
        digitalWrite(M0_PIN, LOW);
        digitalWrite(M1_PIN, HIGH);
        break;
      case MODE_3_SLEEP:
        // Mode 3 | Setting operation
        digitalWrite(M0_PIN, HIGH);
        digitalWrite(M1_PIN, HIGH);
        break;
      default:
        return ;
    }

    WaitAUX_H();
    delay(10);
  }
}
//=== Mode Select ===================================-
//=== Basic cmd =====================================+
void cleanUARTBuf()
{
  bool IsNull = true;

  while (softSerial.available())
  {
    IsNull = false;

    softSerial.read();
  }
}

void triple_cmd(SLEEP_MODE_CMD_TYPE Tcmd)
{
  uint8_t CMD[3] = {Tcmd, Tcmd, Tcmd};
  softSerial.write(CMD, 3);
  delay(50);  //need ti check
}

RET_STATUS Module_info(uint8_t* pReadbuf, uint8_t buf_len)
{
  RET_STATUS STATUS = RET_SUCCESS;
  uint8_t Readcnt, idx;

  Readcnt = softSerial.available();
  //Serial.print("softSerial.available(): ");  Serial.print(Readcnt);  Serial.println(" bytes.");
  if (Readcnt == buf_len)
  {
    for (idx = 0; idx < buf_len; idx++)
    {
      *(pReadbuf + idx) = softSerial.read();
      //Serial.print(" 0x");
      //Serial.print(0xFF & *(pReadbuf + idx), HEX);  // print as an ASCII-encoded hexadecimal
    } 
    //Serial.println("");
  }
  else
  {
    STATUS = RET_DATA_SIZE_NOT_MATCH;
    //Serial.print("  RET_DATA_SIZE_NOT_MATCH - Readcnt: ");  Serial.println(Readcnt);
    cleanUARTBuf();
  }

  return STATUS;
}
//=== Basic cmd =====================================-
//=== Sleep mode cmd ================================+
RET_STATUS Write_CFG_PDS(struct CFGstruct* pCFG)
{
  softSerial.write((uint8_t *)pCFG, 6);

  WaitAUX_H();
  delay(1200);  //need ti check

  return RET_SUCCESS;
}

RET_STATUS Read_CFG(struct CFGstruct* pCFG)
{
  RET_STATUS STATUS = RET_SUCCESS;

  //1. read UART buffer.
  cleanUARTBuf();

  //2. send CMD
  triple_cmd(R_CFG);

  //3. Receive configure
  STATUS = Module_info((uint8_t *)pCFG, sizeof(CFGstruct));
  /*if (STATUS == RET_SUCCESS)
  {
    Serial.print("  HEAD:     ");  Serial.println(pCFG->HEAD, HEX);
    Serial.print("  ADDH:     ");  Serial.println(pCFG->ADDH, HEX);
    Serial.print("  ADDL:     ");  Serial.println(pCFG->ADDL, HEX);
    Serial.print("  CHAN:     ");  Serial.println(pCFG->CHAN, HEX);
  }*/

  return STATUS;
}

RET_STATUS Read_module_version(struct MVerstruct* MVer)
{
  RET_STATUS STATUS = RET_SUCCESS;

  //1. read UART buffer.
  cleanUARTBuf();

  //2. send CMD
  triple_cmd(R_MODULE_VERSION);

  //3. Receive configure
  STATUS = Module_info((uint8_t *)MVer, sizeof(MVerstruct));
  /*if (STATUS == RET_SUCCESS)
  {
    Serial.print("  HEAD:     0x");  Serial.println(MVer->HEAD, HEX);
    Serial.print("  Model:    0x");  Serial.println(MVer->Model, HEX);
    Serial.print("  Version:  0x");  Serial.println(MVer->Version, HEX);
    Serial.print("  features: 0x");  Serial.println(MVer->features, HEX);
  }*/

  return RET_SUCCESS;
}

void Reset_module()
{
  triple_cmd(W_RESET_MODULE);

  WaitAUX_H();
  delay(1000);
}

RET_STATUS SleepModeCmd(uint8_t CMD, void* pBuff)
{
  RET_STATUS STATUS = RET_SUCCESS;

  //Serial.print("SleepModeCmd: 0x");  Serial.println(CMD, HEX);
  WaitAUX_H();

  SwitchMode(MODE_3_SLEEP);

  switch (CMD)
  {
    case W_CFG_PWR_DWN_SAVE:
      STATUS = Write_CFG_PDS((struct CFGstruct* )pBuff);
      break;
    case R_CFG:
      STATUS = Read_CFG((struct CFGstruct* )pBuff);
      break;
    case W_CFG_PWR_DWN_LOSE:

      break;
    case R_MODULE_VERSION:
      Read_module_version((struct MVerstruct* )pBuff);
      break;
    case W_RESET_MODULE:
      Reset_module();
      break;

    default:
      return RET_INVALID_PARAM;
  }

  WaitAUX_H();
  return STATUS;
}
//=== Sleep mode cmd ================================-

RET_STATUS SettingModule(struct CFGstruct *pCFG)
{
  RET_STATUS STATUS = RET_SUCCESS;

#ifdef Device_A
  pCFG->ADDH = DEVICE_A_ADDR_H;
  pCFG->ADDL = DEVICE_A_ADDR_L;
#else
  pCFG->ADDH = DEVICE_B_ADDR_H;
  pCFG->ADDL = DEVICE_B_ADDR_L;
#endif

  pCFG->OPTION_bits.trsm_mode = TRSM_FP_MODE;
  pCFG->OPTION_bits.tsmt_pwr = TSMT_PWR_10DB;

  STATUS = SleepModeCmd(W_CFG_PWR_DWN_SAVE, (void* )pCFG);

  SleepModeCmd(W_RESET_MODULE, NULL);

  STATUS = SleepModeCmd(R_CFG, (void* )pCFG);

  return STATUS;
}

uint32_t read_uint32_t()
{
  uint8_t C[4];
  uint32_t val = 0;
  for (int i = 0; i < sizeof(val); i++)
  {
    C[i] = softSerial.read();
  }
   
  uint8_t* p = (uint8_t*)(void*)&val;
  for (int i = 0; i < sizeof(val); i++)
  {
    *p++=C[i];
  }
  return val;
}

double read_double()
{
  uint8_t C[4];
  double val = 0.0;
  for (uint8_t i = 0; i < sizeof(val); i++)
  {
    C[i] = softSerial.read();
  }
  uint8_t* p = (uint8_t*)(void*)&val;
  for (int i = 0; i < sizeof(val); i++)
  {
    *p++=C[i];
  }
  return val;
}

float read_float()
{
  uint8_t C[4];
  float val = 0.0;
  for (uint8_t i = 0; i < sizeof(val); i++)
  {
    C[i] = softSerial.read();
  }
   
  uint8_t* p = (uint8_t*)(void*)&val;
  for (int i = 0; i < sizeof(val); i++)
  {
     *p++=C[i];
  }
  return val;
}


RET_STATUS ReceiveMsg(uint8_t* timeH, uint8_t* timeM, uint8_t* timeS, uint32_t* numSat, double* spedSat, double* latitud, double* longitud, double* altitud, float* power)
{

  RET_STATUS STATUS = RET_SUCCESS;
  uint8_t idx;

  SwitchMode(MODE_0_NORMAL);
  
  if (softSerial.available()>=27)
  {
    (*timeH) = softSerial.read();
    (*timeM) = softSerial.read();
    (*timeS) = softSerial.read();
    (*numSat) = read_uint32_t();
    (*spedSat) = read_double();
    (*latitud) = read_double();
    (*longitud) = read_double();
    (*altitud) = read_double();
    (*power) = read_float();

    Serial.print(*timeH);
    Serial.print(":"); 
    Serial.print(*timeM);
    Serial.print(":");
    Serial.print(*timeS);
    Serial.print(", ");

    //Number of satellites in use (u32)
    Serial.print(*numSat);
    Serial.print(", "); 
     
    // Speed in meters per second (double)
    Serial.print(*spedSat); 
    Serial.print(", ");
    // Latitude in degrees (double)
    Serial.print(*latitud, 6);
    Serial.print(", ");
    // Longitude in degrees (double)
    Serial.print(*longitud, 6);
    Serial.print(", ");
      
    // Altitude in meters (double)
    Serial.print(*altitud); 
    Serial.print(", ");

    Serial.println(*power,2);
  }
  else
  {
    STATUS = RET_NOT_IMPLEMENT;
  }
  return STATUS;
}

//The setup function is called once at startup of the sketch
void setup()
{
  RET_STATUS STATUS = RET_SUCCESS;
  struct CFGstruct CFG;
  struct MVerstruct MVer;

  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(AUX_PIN, INPUT);

  softSerial.begin(9600);
  Serial.begin(9600);

#ifdef Device_A
  //Serial.println("[10-A] ");
#else
  //Serial.println("[10-B] ");
#endif

  STATUS = SleepModeCmd(R_CFG, (void* )&CFG);
  STATUS = SettingModule(&CFG);

  STATUS = SleepModeCmd(R_MODULE_VERSION, (void* )&MVer);

  // Mode 0 | normal operation
  SwitchMode(MODE_0_NORMAL);

  //self-check initialization.
  WaitAUX_H();
  delay(10);

  /*if (STATUS == RET_SUCCESS)
    Serial.println("Setup init OK!!");*/
}

// The loop function is called in an endless loop
void loop()
{
  uint8_t h, m, s;
  uint32_t sat;
  double sped, lati, longi, alti;
  float voltage;

  if (ReceiveMsg(&h, &m, &s, &sat, &sped, &lati, &longi, &alti, &voltage) == RET_SUCCESS)
  {
        
  }
}
