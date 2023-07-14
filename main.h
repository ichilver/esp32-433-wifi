// Define buffer size for storing received signal state and duration
#define RAWBITBUFFERSIZE   150

#define BASEPROTOCOL433_RX_PREAMPLE_THRESOLD_MIN	8000
#define BASEPROTOCOL433_RX_PREAMPLE_THRESOLD_MAX 25000

// Declare states that we can be in
#define	BASEPROTOCOL433_RX_LISTENMODE             0x00		//  Listening for a message to start on the RX
#define	BASEPROTOCOL433_RX_RECEIVEMODE            0x01		//  Currently receiving a message over RX
#define BASEPROTOCOL433_RX_MESSAGERECEIVED        0x02		//  Received a complete message
#define BASEPROTOCOL433_RX_MESSAGEDECODED         1			//  Message has been decoded, Boolean Flag
#define BASEPROTOCOL433_SETUP                     0xF0

// Struct which is used to store received pulses in a buffer
typedef struct {
  bool state;                       // state 0 or 1 (Boolean)
  unsigned int duration;            // duration of the state in ms
} PULSE;

void loop2(void * parameter);
void loop3(void * parameter);

#ifdef USE_WIFI
  void setup_wifi();
  void displayWifiDetails();
#endif