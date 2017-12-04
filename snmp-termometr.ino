/*
 * Agentuino SNMP termometr.  
 *
 * A0,A1 - Input sensor pins
 * 2 pin - oneWire Dallas Temperature sensor.
 * 3 pin - relay max temperature
 * 4 pin - relay min temperature   
*/

#include <Ethernet.h>
#include <SPI.h>
#include "Agentuino.h" 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>

#define ONE_WIRE_BUS 2

static int max_temperature = 40; 
static int min_tempetature = 0;

static byte mac[] = { 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x11 };
static byte ip[] = { 10, 0, 0, 1 };
// static byte gateway[] = { 192, 168, 100, 17 };
// static byte subnet[] = { 255, 255, 255, 0 };

// Arduino defined OIDs. Pins A0, A1
// .iso.org.dod.internet.private.enterprises.arduino.value.valA0-A5 (.1.3.6.1.4.1.36582.3.1-6) 
static const char valA0[] PROGMEM     = "1.3.6.1.4.1.36582.3.2.0";  // read-only  (Integer)
static const char valA1[] PROGMEM     = "1.3.6.1.4.1.36582.3.3.0";  // read-only  (Integer)

// Temperature sensor
static const char valTemperature[] PROGMEM     = "1.3.6.1.4.1.36582.3.7.0";  // read-only  (Integer)

// RFC1213-MIB OIDs
// .iso.org.dod.internet.mgmt.mib-2.system.sysDescr (.1.3.6.1.2.1.1.1)
static const char sysDescr[] PROGMEM      = "1.3.6.1.2.1.1.1.0";  // read-only  (DisplayString)
// .iso.org.dod.internet.mgmt.mib-2.system.sysObjectID (.1.3.6.1.2.1.1.2)
//static char sysObjectID[] PROGMEM   = "1.3.6.1.2.1.1.2.0";  // read-only  (ObjectIdentifier)
// .iso.org.dod.internet.mgmt.mib-2.system.sysUpTime (.1.3.6.1.2.1.1.3)
static const char sysUpTime[] PROGMEM     = "1.3.6.1.2.1.1.3.0";  // read-only  (TimeTicks)
// .iso.org.dod.internet.mgmt.mib-2.system.sysContact (.1.3.6.1.2.1.1.4)
static const char sysContact[] PROGMEM    = "1.3.6.1.2.1.1.4.0";  // read-write (DisplayString)
// .iso.org.dod.internet.mgmt.mib-2.system.sysName (.1.3.6.1.2.1.1.5)
static const char sysName[] PROGMEM       = "1.3.6.1.2.1.1.5.0";  // read-write (DisplayString)
// .iso.org.dod.internet.mgmt.mib-2.system.sysLocation (.1.3.6.1.2.1.1.6)
static const char sysLocation[] PROGMEM   = "1.3.6.1.2.1.1.6.0";  // read-write (DisplayString)
// .iso.org.dod.internet.mgmt.mib-2.system.sysServices (.1.3.6.1.2.1.1.7)
static const char sysServices[] PROGMEM   = "1.3.6.1.2.1.1.7.0";  // read-only  (Integer)



// RFC1213 local values 
static const char locDescr[]      PROGMEM   = "suxorabovka";                     // read-only (static)
static uint32_t locUpTime                   = 0;                                 // read-only (static)
static const char locContact[20]  PROGMEM   = "Dima";                            // should be stored/read from EEPROM - read/write (not done for simplicity)
static const char locName[20]     PROGMEM   = "Arduino";                         // should be stored/read from EEPROM - read/write (not done for simplicity)
static const char locLocation[20] PROGMEM   = "suxorabovka";                     // should be stored/read from EEPROM - read/write (not done for simplicity)
static int32_t locServices                  = 12;                                // read-only (static)

uint32_t prevMillis = millis();
char oid[SNMP_MAX_OID_LEN];

SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

static float a0_value = 0.0;
static float a1_value = 0.0;

static int a0 = 0;
static int a1 = 0;

 
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

static int temperature = 0; 

void pduReceived()
{
  SNMP_PDU pdu;

  api_status = Agentuino.requestPdu(&pdu);
  
  if ((pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || pdu.type == SNMP_PDU_SET)
    && pdu.error == SNMP_ERR_NO_ERROR && api_status == SNMP_API_STAT_SUCCESS ) {

    pdu.OID.toString(oid);
    
    // Implementation SNMP GET NEXT
    if ( pdu.type == SNMP_PDU_GET_NEXT ) {
      char tmpOIDfs[SNMP_MAX_OID_LEN];
      if ( strcmp_P( oid, sysDescr ) == 0 ) {  
        strcpy_P ( oid, sysUpTime ); 
        strcpy_P ( tmpOIDfs, sysUpTime );        
        pdu.OID.fromString(tmpOIDfs);  
      } else if ( strcmp_P(oid, sysUpTime ) == 0 ) {  
        strcpy_P ( oid, sysContact );  
        strcpy_P ( tmpOIDfs, sysContact );        
        pdu.OID.fromString(tmpOIDfs);          
      } else if ( strcmp_P(oid, sysContact ) == 0 ) {  
        strcpy_P ( oid, sysName );  
        strcpy_P ( tmpOIDfs, sysName );        
        pdu.OID.fromString(tmpOIDfs);                  
      } else if ( strcmp_P(oid, sysName ) == 0 ) {  
        strcpy_P ( oid, sysLocation );  
        strcpy_P ( tmpOIDfs, sysLocation );        
        pdu.OID.fromString(tmpOIDfs);                  
      } else if ( strcmp_P(oid, sysLocation ) == 0 ) {  
        strcpy_P ( oid, sysServices );  
        strcpy_P ( tmpOIDfs, sysServices );        
        pdu.OID.fromString(tmpOIDfs); 
      } else if ( strcmp_P(oid, sysServices ) == 0 ) {  
        strcpy_P ( oid, "1.0" );  
       
      } else {
          int ilen = strlen(oid);
          if ( strncmp_P(oid, sysDescr, ilen ) == 0 ) {
            strcpy_P ( oid, sysDescr ); 
            strcpy_P ( tmpOIDfs, sysDescr );        
            pdu.OID.fromString(tmpOIDfs); 
          } else if ( strncmp_P(oid, sysUpTime, ilen ) == 0 ) {
            strcpy_P ( oid, sysUpTime ); 
            strcpy_P ( tmpOIDfs, sysUpTime );        
            pdu.OID.fromString(tmpOIDfs); 
          } else if ( strncmp_P(oid, sysContact, ilen ) == 0 ) {
            strcpy_P ( oid, sysContact ); 
            strcpy_P ( tmpOIDfs, sysContact );        
            pdu.OID.fromString(tmpOIDfs); 
          } else if ( strncmp_P(oid, sysName, ilen ) == 0 ) {
            strcpy_P ( oid, sysName ); 
            strcpy_P ( tmpOIDfs, sysName );        
            pdu.OID.fromString(tmpOIDfs);   
          } else if ( strncmp_P(oid, sysLocation, ilen ) == 0 ) {
            strcpy_P ( oid, sysLocation ); 
            strcpy_P ( tmpOIDfs, sysLocation );        
            pdu.OID.fromString(tmpOIDfs);    
          } else if ( strncmp_P(oid, sysServices, ilen ) == 0 ) {
            strcpy_P ( oid, sysServices ); 
            strcpy_P ( tmpOIDfs, sysServices );        
            pdu.OID.fromString(tmpOIDfs);  
          }  
      } 
    }
    // End of implementation SNMP GET NEXT / WALK
    
    
    
    if ( strcmp_P(oid, sysDescr ) == 0 ) {
      // handle sysDescr (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locDescr
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locDescr);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysUpTime ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locUpTime
        status = pdu.VALUE.encode(SNMP_SYNTAX_TIME_TICKS, locUpTime);       
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysName ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locName, strlen(locName)); 
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locName
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locName);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysContact ) == 0 ) {
      // handle sysContact (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locContact, strlen(locContact)); 
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locContact
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locContact);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysLocation ) == 0 ) {
      // handle sysLocation (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locLocation, strlen(locLocation)); 
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locLocation
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locLocation);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      
      //
    } else if ( strcmp_P(oid, sysServices) == 0 ) {
      // handle sysServices (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locServices
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, locServices);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      
    // A0 
    } else if ( strcmp_P(oid, valA0) == 0 ) {
      // handle sysServices (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, a0);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    
    // A1
    else if ( strcmp_P(oid, valA1) == 0 ) {
      // handle sysServices (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, a1);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    
    
	// Temperature sensor  
    else if ( strcmp_P(oid, valTemperature) == 0 ) {
      // handle sysServices (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, temperature);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
	
    else {
      // oid does not exist
      pdu.type = SNMP_PDU_RESPONSE;
      pdu.error = SNMP_ERR_NO_SUCH_NAME;
    }
    Agentuino.responsePdu(&pdu);
  }
  Agentuino.freePdu(&pdu);
}

void setup()
{
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  
  // Relay pins setup.
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
 
  Ethernet.begin(mac, ip);
  
  api_status = Agentuino.begin();

  if ( api_status == SNMP_API_STAT_SUCCESS ) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
    return;
  }
  delay(10);
  
  sensors.begin();
  wdt_enable (WDTO_8S); // 8 sec. 
}

void loop()
{	
  Agentuino.listen();

  // sysUpTime
  if ( millis() - prevMillis > 1000 )
  {
    // increment previous milliseconds   
    prevMillis += 1000; 
    locUpTime += 100;
	
	// You can have more than one IC on the same bus. 0 refers to the first IC on the wire 
	sensors.requestTemperatures();
	temperature = (int)sensors.getTempCByIndex(0);
	
    a0_value = ((analogRead(A0) * 5) / 1023) * 10;
    a1_value = ((analogRead(A1) * 5) / 1023) * 10;
	
	a0 = int(a0_value);
	a1 = int(a1_value);
	
    // Enable cooler. 
    if(temperature > max_temperature)
	  digitalWrite(3, HIGH);
    else
	  digitalWrite(3, LOW);
	  
    // Enable heater.
    if(temperature < min_tempetature)
	  digitalWrite(4, HIGH);
    else
      digitalWrite(4, LOW);
  }

  // WatchDog timer reset.
  wdt_reset();   
}
