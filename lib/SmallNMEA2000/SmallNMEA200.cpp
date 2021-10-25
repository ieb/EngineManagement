#include <Arduino.h>

#include <MemoryFree.h>
#include <mcp_can.h>
#include <SPI.h>

#include "SmallNMEA2000.h"





bool SNMEA2000::open() {
    if ( CAN.begin(CAN_250KBPS, MCP_8MHz)==CAN_OK ) {
        delay(200);
        claimAddress();
        Serial.println("Claimed Address Sent");
        return true;
    }
    return false;
}


void SNMEA2000::processMessages() {
    unsigned char len = 0;
    int frames = 0;
    unsigned char buf[8];
    hasClaimedAddress();
    while(frames < 20 && CAN_MSGAVAIL == CAN.checkReceive()){
        frames++;
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
        MessageHeader messageHeader(CAN.getCanId());
        switch (messageHeader.pgn) {
          case 59392L: /*ISO Acknowledgement*/
            Serial.print("<ack ");
             messageHeader.print(buf,len);
            break;
          case 59904L: /*ISO Request*/
            Serial.print("<req ");
             messageHeader.print(buf,len);
            handleISORequest(&messageHeader, buf, len);
            break;
          case 60928L: /*ISO Address Claim*/
            Serial.print("<claim ");
             messageHeader.print(buf,len);
            handleISOAddressClaim(&messageHeader, buf, len);
            break;
           default:
             Serial.print("<unknown");
             messageHeader.print(buf,len);

        }
    }
}



void SNMEA2000::handleISOAddressClaim(MessageHeader *messageHeader, byte * buffer, int len) {
    if ( messageHeader->source == 254 ) return;
    uint64_t callerName = (((uint64_t)buffer[2])<<16) | (((uint64_t)buffer[1])<<8) | buffer[0];
    if ( deviceName > callerName ) { // our name is < callers we can keep it.
        deviceAddress++;
    }
    claimAddress();
}

void SNMEA2000::claimAddress() {
    clearRXFilter();
    sendIsoAddressClaim();
    addressClaimStarted = millis();
}

bool SNMEA2000::hasClaimedAddress() {
    if (addressClaimStarted+250 > millis() ) {
        Serial.print("Address claimed as ");
        Serial.println(deviceAddress);
        addressClaimStarted=0;
        setupRXFilter();
    }
    return (addressClaimStarted==0);
}



void SNMEA2000::handleISORequest(MessageHeader *messageHeader, byte * buffer, int len) {
    if ( len < 3 || len > 8) {
        Serial.println(" lenfail");
        return; // ISO requests are expected to be between 3 and 8 bytes.
    }
    if (messageHeader->destination == 0xff || messageHeader->destination != deviceAddress ) {
        Serial.println(" notus");
        return;
    }
    if (!hasClaimedAddress()) {
        Serial.println("Address not claimed.");
        return; // dont respond to queries until address is claimed.
    }
    unsigned long requestedPGN = (((unsigned long )buffer[2])<<16)|(((unsigned long )buffer[1])<<8)|(buffer[0]);
    Serial.print("ReqPGN:");Serial.println(requestedPGN);
    switch(requestedPGN) {
      case 60928L: /*ISO Address Claim*/  // Someone is asking others to claim their addresses
        Serial.print("other_claim>");
        sendIsoAddressClaim();
        break;
      case 126464L:
        Serial.print("pgns>");
        sendPGNLists(messageHeader);
        break;
      case 126996L: /* Product information */
        Serial.print("prod>");
        sendProductInformation(messageHeader);
        break;
      case 126998L: /* Configuration information */
        Serial.print("cond>");
        sendConfigurationInformation(messageHeader);
        break;
      default:
        Serial.print("sending nak>");
        sendIsoAcknowlegement(messageHeader, 1, 0xff);
    }

}




// the PDN lists are 
void SNMEA2000::sendPGNLists(MessageHeader *requestMessageHeader) {
    MessageHeader messageHeader(126464L, 6, deviceAddress, requestMessageHeader->source);
    // 126464L structure is a fast packet sequence with the
    // total length is 1+npgns*3
    sendPGNList(&messageHeader, 0, txPGNList);
    sendPGNList(&messageHeader, 1, rxPGNList);
}



void SNMEA2000::sendPGNList(MessageHeader *messageHeader, int listType, const unsigned long *pgnList) {
    int ipgn = 0; // pgn length
    while(pgm_read_dword(&pgnList[ipgn++]) !=0 );
    startFastPacket(messageHeader, 1+ipgn*3);
    outputByte(listType); // RX PGN List
    for(int i = 0; i < ipgn; i++) {
        output3ByteInt(pgm_read_dword(&pgnList[ipgn]));
    }
    finishFastPacket();
}




void SNMEA2000::sendIsoAddressClaim() {
  MessageHeader messageHeader(60928L, 6, deviceAddress, 0xff);
  // CAN and AVR are little endian, so this is ok.
  sendMessage(&messageHeader, (byte *)&deviceName, 8);
}



void SNMEA2000::sendProductInformation(MessageHeader *requestMessageHeader) {
    MessageHeader messageHeader(126996L, 6, deviceAddress, requestMessageHeader->source);
    // this is a fast packet, has to be constructed from the struc on the fly.
    startFastPacket(&messageHeader, 4+32*4+2);
    output2ByteUInt(productInfo->nk2version);
    output2ByteUInt(productInfo->productCode);
    outputFixedString(productInfo->modelID, 32, 0xff);
    outputFixedString(productInfo->softwareVersion, 32, 0xff);
    outputFixedString(productInfo->modelVersion, 32, 0xff);
    outputFixedString(productInfo->serialNumber, 32, 0xff);
    outputByte(productInfo->certificationLevel);
    outputByte(productInfo->loadEquivalency);
    finishFastPacket();
}


void SNMEA2000::sendConfigurationInformation(MessageHeader *requestMessageHeader) {
    MessageHeader messageHeader(126998L, 6, deviceAddress, requestMessageHeader->source);
    // this is a fast packet.
    int manufacturerInfoLen = getPgmSize(configInfo->manufacturerInfo,70);
    int installDesc1Len = getPgmSize(configInfo->installDesc1,70);
    int installDesc2Len = getPgmSize(configInfo->installDesc2,70);
    startFastPacket(&messageHeader, manufacturerInfoLen+installDesc1Len+installDesc2Len+6 );
    outputVarString(configInfo->manufacturerInfo, manufacturerInfoLen);
    outputVarString(configInfo->installDesc1, installDesc1Len);
    outputVarString(configInfo->installDesc1, installDesc2Len);
    finishFastPacket();
}
int SNMEA2000::getPgmSize(const char *str, int maxLen) {
    for(int i =0; i < maxLen; i++) {
        char c = pgm_read_byte(str[i]);
        if ( c == '\0') return i;
    }
    return maxLen;
}
void SNMEA2000::outputVarString(const char * str,  uint8_t strLen) {
    outputByte(strLen+2);
    outputByte(0x01);
    for(int i = 0; i < strLen; i++) {
        outputByte(pgm_read_byte(&str[i]));
    }
}

void SNMEA2000::outputFixedString(const char * str, int maxLen, byte padding) {
    int ib = 0;
    while( ib < maxLen ) {
        byte bs = pgm_read_byte(&str[ib++]);
        if (bs == '\0') break;
        outputByte(bs);
    }
    while (ib < maxLen) {
        outputByte(padding);
    }
}

void SNMEA2000::output2ByteUInt(uint16_t i) {
    outputByte((i>>8)&0xff);
    outputByte(i&0xff);
}
void SNMEA2000::output2ByteInt(uint16_t i) {
    outputByte((i>>8)&0xff);
    outputByte(i&0xff);
}
void SNMEA2000::output3ByteInt(int32_t i) {
    outputByte((i>>16)&0xff);
    outputByte((i>>8)&0xff);
    outputByte(i&0xff);
}
void SNMEA2000::output2ByteDouble(double value, double precision) {
    if (value == SNMEA2000::n2kDoubleNA ) {
        // udefined is 32768 = 0x8000
        outputByte(0x80);
        outputByte(0x00);
    } else {
        double vd=round(value/precision);
        int16_t i = (vd>=-32768 && vd<0x7fee)?(int16_t)vd:0x7fee;
        outputByte((i>>8)&0xff);
        outputByte(i&0xff);
    }
}
void SNMEA2000::output2ByteUDouble(double value, double precision) {
    // indef is 65535U = 0xFFFF
    if (value == SNMEA2000::n2kDoubleNA) {
        outputByte(0xff);
        outputByte(0xff);
    } else {
        double vd=round(value/precision);
        uint16_t i = (vd>=0 && vd<0xfffe)?(int16_t)vd:0xfffe;
        outputByte((i>>8)&0xff);
        outputByte(i&0xff);
    }

}
void SNMEA2000::output4ByteDouble(double value, double precision) {
    if (value == SNMEA2000::n2kDoubleNA ) {
        // undef is 2147483647 = 7FFFFFFF
        outputByte(0x7f);
        outputByte(0xff);
        outputByte(0xff);
        outputByte(0xff);
    } else {
        double vd=round(value/precision);
        int32_t i = (vd>=-2147483648L && vd<0x7ffffffe)?(int16_t)vd:0x7ffffffe;
        outputByte((i>>24)&0xff);
        outputByte((i>>16)&0xff);
        outputByte((i>>8)&0xff);
        outputByte(i&0xff);
    }
}
void SNMEA2000::output4ByteUDouble(double value, double precision) {
    if (value == SNMEA2000::n2kDoubleNA ) {
        // undef is 4294967295U = FFFFFFFF
        outputByte(0xff);
        outputByte(0xff);
        outputByte(0xff);
        outputByte(0xff);
    } else {
        double vd=round(value/precision);
        uint32_t i = (vd>=0 && vd<0xfffffffe)?(int16_t)vd:0xfffffffe;
        outputByte((i>>24)&0xff);
        outputByte((i>>16)&0xff);
        outputByte((i>>8)&0xff);
        outputByte(i&0xff);
    }
}

void SNMEA2000::setupRXFilter() {
        // CAN ID is 29 bits long
        // 111 1 1 11111111 11111111 11111111
        //                           -------- Source Addresss 8 bits @0
        //                  -------- PDU specific 8 bits @ 8
        //         -------- PDU Format 8 bits @ 16
        //       - Data Page 1 but @24
        //     - reserved 1 bit @25
        // --- priority (3 bits starting @26)

        // PDU Format < 240 is transmitted with a destination address, which is in 8 bits 8
        // PDU Format 240 > 255 is a broadcast with the Group extension in 8 bits @8.

        // For all PDU < 240 we are only interested destination addresses
        // of our destination or 0xff.
        // For all PDU's >= 240 then we are only interested in packets matching
        // the recieved PGNS.

        // since we interested in 0xff messages, then that makes the mask and filter for the PSU specific part as 0xff 0xff which 
        // 

        // 59392L == 11101000 00000000 232 PDU1
        // 59904L == 11101010 00000000 234 PDU1
        // 60928L == 11101110 00000000 238 PDU1
        //
        // Accept for our device and 0xfff
        // Mask                 11111001 11111111 00000000
        // Filter 59392L to us  11101000 <our device> 00000000
        // Filter 59392L to 0xff  11101000 11111111 00000000
        // Filter 59904L to 0xff  11101010 11111111 00000000
        // Filter 60928L to 0xff  11101110 11111111 00000000


        CAN.init_Mask(0,1,0xf9ff0);
        CAN.init_Mask(1,1,0xf9ff0);
        CAN.init_Filt(0,1,0xE8FF); // broadcasts
        CAN.init_Filt(0,1,0xE800 || deviceAddress); // thisDevice
}

void SNMEA2000::clearRXFilter() {
        CAN.init_Mask(0,1,0x0);
        CAN.init_Mask(1,1,0x0);
}




void SNMEA2000::startPacket(MessageHeader *messageHeader) {
    packetMessageHeader = messageHeader;
    ob = 0;
    fastPacket = false;
} 
void SNMEA2000::finishPacket() {
    if ( !fastPacket ) {
        if ( ob > 0) {
            sendMessage(packetMessageHeader, buffer, ob);
        }
    } else {
        Serial.println("Error: Not a Single Packet");
    }
}


void SNMEA2000::startFastPacket(MessageHeader *messageHeader, int length) {
    packetMessageHeader = messageHeader;
    fastPacket = true;
    frame = 0;
    ob = 0;
    buffer[ob++] = frame++;
    buffer[ob++] = length;
} 

void SNMEA2000::finishFastPacket() {
    if (fastPacket ) {
        if (ob > 1) {
            // send remaining frame
            sendMessage(packetMessageHeader, buffer, ob);
        }
    } else {
        Serial.println("Error: Not a FastPacket");
    }
}

void SNMEA2000::outputByte(byte opb) {
    if ( ob < 8 ) {
        buffer[ob++] = opb;
        if( fastPacket && ob == 8) {
            sendMessage(packetMessageHeader, &buffer[0], 8);
            ob = 0;
            buffer[ob++] = frame++;
        }
    } else {
        Serial.println("Error: Frame > 8 bytes");
    }
}


void SNMEA2000::sendFastPacket(MessageHeader *messageHeader, byte *message, int len, bool progmem) {
    startFastPacket(messageHeader, len);
    int ib = 0;
    while(ib < len) {
        if ( progmem ) {
            outputByte(pgm_read_byte(&message[ib++]));
        } else {
            outputByte(message[ib++]);
        }
    }
    finishFastPacket();
}

void SNMEA2000::sendIsoAcknowlegement(MessageHeader *requestMessageHeader, unsigned char control, unsigned char groupFunction) {
    MessageHeader messageHeader(59392L, 6, deviceAddress, requestMessageHeader->source);
    startPacket(&messageHeader);
    outputByte(control);
    outputByte(groupFunction);
    outputByte(0xff);
    outputByte(0xff);
    outputByte(0xff);
    output3ByteInt(requestMessageHeader->pgn);
    finishPacket();
}

void SNMEA2000::sendMessage(MessageHeader *messageHeader, byte * message, int length) {
    Serial.print(freeMemory());
    Serial.print(",");
    messageHeader->print(message, length);
    byte res = CAN.sendMsgBuf(messageHeader->id, 1, length, message);
    Serial.println(res);
}


void EngineMonitor::sendRapidEngineDataMessage(
    byte engineInstance, 
    double engineSpeed, 
    double engineBoostPressure, 
    byte engineTiltTrim) {
    MessageHeader messageHeader(127488L, 2, getAddress(), SNMEA2000::broadcastAddress);
    startPacket(&messageHeader);
    outputByte(engineInstance);
    output2ByteDouble(engineSpeed,0.25);
    output2ByteDouble(engineBoostPressure,100);
    outputByte(engineTiltTrim);
    outputByte(0xff);
    outputByte(0xff);
    finishPacket();
}

void EngineMonitor::sendEngineDynamicParamMessage(
    byte engineInstance,
    double engineHours,
    double engineCoolantTemperature,
    double alternatorVoltage,
    uint16_t status1,
    uint16_t status2,
    double engineOilPressure,
    double engineOilPTemperature,
    double fuelRate,
    double engineCoolantPressure,
    double engineFuelPressure,
    byte engineLoad,
    byte engineTorque
    
) {
    MessageHeader messageHeader(127489L, 2, getAddress(), SNMEA2000::broadcastAddress);
    startFastPacket(&messageHeader, 26);
    outputByte(engineInstance);
    output2ByteUDouble(engineOilPressure,100);
    output2ByteUDouble(engineOilPTemperature,0.1);
    output2ByteUDouble(engineCoolantTemperature,0.01);
    output2ByteDouble(alternatorVoltage,0.01);
    output2ByteDouble(fuelRate,0.1);
    output4ByteUDouble(engineHours,1);
    output2ByteUDouble(engineCoolantPressure,100);
    output2ByteUDouble(engineFuelPressure,1000);
    outputByte(0xff); // reserved
    output2ByteUInt(status1);
    output2ByteUInt(status2);
    outputByte(engineLoad); 
    outputByte(engineTorque);
    finishFastPacket();

}
void EngineMonitor::sendDCBatterStatusMessage(
    byte batteryInstance, 
    byte sid,
    double batteryVoltage,
    double batteryTemperature,
    double batteryCurrent
    ) {
    MessageHeader messageHeader(127508L, 6, getAddress(), SNMEA2000::broadcastAddress);
    startPacket(&messageHeader);
    outputByte(batteryInstance);
    output2ByteDouble(batteryVoltage,0.01);
    output2ByteDouble(batteryCurrent,0.1);
    output2ByteDouble(batteryTemperature,0.01);
    outputByte(sid);
    finishPacket();
}
void EngineMonitor::sendFluidLevelMessage(
    byte type,
    byte instance,
    double level, 
    double capacity) {
    MessageHeader messageHeader(127505L, 6, getAddress(), SNMEA2000::broadcastAddress);
    startPacket(&messageHeader);
    byte b = ((type<<4)&0xff)|(instance&0xff);
    outputByte(b);
    output2ByteDouble(level,0.0004);
    output4ByteUDouble(capacity,0.1);
    outputByte(0xff);
    finishPacket();
}
void EngineMonitor::sendTemperatureMessage(
    byte sid, 
    byte instance,
    byte source,
    double actual,
    double requested) {
    MessageHeader messageHeader(130312L, 5, getAddress(), SNMEA2000::broadcastAddress);
    startPacket(&messageHeader);
    outputByte(sid);
    outputByte(instance);
    outputByte(source);
    output2ByteUDouble(actual,0.01);
    output2ByteUDouble(requested,0.01);
    outputByte(0xff);
    finishPacket();
}

