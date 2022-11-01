/* mbed Microcontroller Library
 * Copyright (c) 2006-2019 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include <cstdint>
#include <events/mbed_events.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include <AdvertisingDataSimpleBuilder.h>
#include <ble/gatt/GattCharacteristic.h>
#include <ble/gatt/GattService.h>
#include "att_uuid.h"

#include <USBSerial.h>

USBSerial ser;

Thread tempthread;
const UUID uuid = 0x78;

static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

BLE &bleinit= BLE::Instance();
Gap& gap = bleinit.gap();
GattServer& gattServe = bleinit.gattServer();
GattClient& gattClient = bleinit.gattClient();
GattAttribute::Handle_t Hvalue;


int16_t TOUT =0;

using namespace ble;

/**
 * Event handler struct
 */
struct GapEventHandler : Gap::EventHandler{
    /* 
	 * Implement the functions here that you think you'll need. These are defined in the GAP EventHandler:
     * https://os.mbed.com/docs/mbed-os/v6.6/mbed-os-api-doxy/structble_1_1_gap_1_1_event_handler.html
     */
     //once advertising starts print out that it has started
    void onAdvertisingStart (const AdvertisingStartEvent &event) {
        ser.printf("Advertising started\r\n");
    }

    // once advertising has ending get the status of why it stopped and print why 
    void onAdvertisingEnd (const AdvertisingEndEvent &event){
        int Status = event.getStatus();
        if (Status == BLE_ERROR_TIMEOUT){
            ser.printf("Advertising ended because couldn't find a device\r\n");
        }
        else if(Status == BLE_ERROR_NONE){
            ser.printf("Advertising ended because device connected\r\n");
        }
        else if(Status == BLE_ERROR_NOT_FOUND){
            ser.printf("Advertising ended unknown error\r\n");
        }
        

    }
    
    // once the bluetooth is connected, print that is has been connected and turn off advertising
    void onConnectionComplete (const ConnectionCompleteEvent &event){
        ser.printf("Bluetooth Connected\r\n");
        gap.stopAdvertising(LEGACY_ADVERTISING_HANDLE);

    }

    // once bluetooth is disconnected, turn on advertising
    void onDisconnectionComplete (const DisconnectionCompleteEvent &event){
        gap.startAdvertising(LEGACY_ADVERTISING_HANDLE,adv_duration_t::forever(),0);
    }
};


GapEventHandler THE_gap_EvtHandler;


void setupAdvertisingData(){
    using namespace ble;
    // set up advertising and if there is an error in set up print that there was an error
    if (BLE_ERROR_NONE != gap.setAdvertisingPayload(
        LEGACY_ADVERTISING_HANDLE,
        AdvertisingDataSimpleBuilder<LEGACY_ADVERTISING_MAX_SIZE>()
            .setFlags()
            .setName("Clur's chip", true)
            .setAppearance(adv_data_appearance_t::GENERIC_THERMOMETER)
            .setLocalService(ATT_UUID_HEALTH_THERM_SERVICE)
            .getAdvertisingData())){
                ser.printf("Error in Advertising\r\n");
            }
    else{
        // if the advertising was set up correctly, then start advertising and set the event handler
        gap.startAdvertising(LEGACY_ADVERTISING_HANDLE,adv_duration_t::forever(),0);	
        gap.setEventHandler(&THE_gap_EvtHandler);
    }

}


void measure_temp(){

    // measuring temperature thread given to us
    I2C sensor_bus(p31, p2);

    const int readaddr = 0xEF;
    const int writeaddr = 0xEE;
    uint8_t whoamiaddr[] = {0xD0};
    int resp=4;

    char readData[] ={0, 0};
    resp = sensor_bus.write(writeaddr, (const char *) whoamiaddr, 1, true);
    
    if(  resp != 0 ){
        ser.printf("I failed to talk at the temp sensor. (Returned: %d)\n\r", resp);            
    }
              
    if( sensor_bus.read(readaddr, readData, 1)  != 0 ){
        ser.printf("I failed to listen to the temp sensor.\n\r");        
    }
    
    ser.printf("Who Am I? %d\n", readData[0] );
    if( readData[0] != 0x55 ){
        ser.printf("Who are are you?\n\r");
    }

    readData[0] = 0x20; // Control Reg 1
    readData[1] = 0x84; // Turn on our temp sensor, and ensure that we read high to low on our values.
    resp = sensor_bus.write(readaddr, readData, 2);    


    uint8_t databuf[2];
    while(1){        
        readData[0] = 0xF4; // Control Reg 2
        readData[1] = 0x2E; // Signal a one shot temp reading.
        resp = sensor_bus.write(readaddr, readData, 2);

		thread_sleep_for(5);
        
        readData[0] = 0xF6; // MSB Temperature
        sensor_bus.write(writeaddr, (const char *) readData, 1, true);
        sensor_bus.read(readaddr, readData, 1);
        databuf[0] = ((uint8_t)readData[0]);

        readData[0] = 0xF7; // LSB Temperature
        sensor_bus.write(writeaddr, (const char *) readData, 1, true);
        sensor_bus.read(readaddr, readData, 1);
        databuf[1] = readData[0];

        TOUT = (databuf[0]<<8) | databuf[1];
        ser.printf("Uncalibrated temperature: %d\n\r",TOUT);

        // Sleep for a while.
        thread_sleep_for(5000);
        uint8_t x;
        uint8_t databuf2[2];

        // if the handle has been set than write temperature values to it 
        if(Hvalue){
            databuf2[0] = databuf[1];
            databuf2[1] = databuf[0];
            x = gattServe.write(Hvalue, databuf2, 2);
            // if there is an error n writing to the handle then print that there has been an error
            if(x != BLE_ERROR_NONE){
           ser.printf("Error writing the Temperature to the chip!!"); 
            }
        }
        

    }
}

void on_init_complete(BLE::InitializationCompleteCallbackContext *params){
    // if initialization successful print that and if error print that 
    if(params->error == BLE_ERROR_NONE){
        ser.printf("Initialization complete\r\n");
    }
    else{
        ser.printf("Error with Initialization\r\n");
    }

    // call function to start advertising 
    setupAdvertisingData();

    // create characteristic with random UUID and random starting value and to be adle to read and indicate
    int test[1];	
    test[0]=1234;
    ReadOnlyGattCharacteristic<int> tempc(uuid, test, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ|GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE);
    // create GATT service and add GATT characteristic to the GATT service
    GattCharacteristic *character[] = {&tempc};
    GattService gservice(ATT_UUID_HEALTH_THERM_SERVICE, character, 1);
    // add the service to the chips GATT server
    gattServe.addService(gservice);

    // get handle once the characteristic is added to the service
    Hvalue = tempc.getValueHandle();

}
/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context){
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));

}


int main(){
    DigitalOut i2cbuspull(P1_0); // Pull up i2C. resistor.
    i2cbuspull.write(1);
    DigitalOut sensor_pwr(P0_22); // Supply power to all of the sensors (VCC)
    sensor_pwr.write(1);

    bleinit.init(on_init_complete);

    bleinit.onEventsToProcess(schedule_ble_events);
    // start the reading temperature thread 
    tempthread.start(measure_temp);
    // This will never return...
    event_queue.dispatch_forever();
}


