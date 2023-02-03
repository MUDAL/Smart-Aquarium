#pragma once

//MNI: Master-Node-Interface
//Handles serial communication between Master (ESP32) and the Node(Nano)
//Features: Bi-directional communication, 16-bit encoded/decoded data.

class MNI
{
  private:
    enum BufferSize {TX = 10, RX = 2};
    SoftwareSerial* port;
    uint8_t rxDataCounter;
    uint8_t txBuffer[BufferSize::TX];
    uint8_t rxBuffer[BufferSize::RX];
    
  public:
    enum {QUERY = 0xAA, ACK = 0xBB};
    enum TxDataId
    {
      DATA_ACK = 0,
      PH = 2,
      TEMPERATURE = 4,
      TDS = 6,
      TURBIDITY = 8
    };
    enum RxDataId {DATA_QUERY = 0};
    
    MNI(SoftwareSerial* serial,uint32_t baudRate = 9600);
    void EncodeData(uint16_t dataToEncode,TxDataId id);
    void TransmitData(void);
    bool ReceivedData(void);
    uint16_t DecodeData(RxDataId id);    
};
