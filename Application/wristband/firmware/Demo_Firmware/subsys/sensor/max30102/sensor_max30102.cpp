#include "CH58x_common.h"
#include "MAX30105.h"
#include "heartRate.h"
extern "C"{
    #include "spo2_algorithm.h"
}



MAX30105 particleSensor;

uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

uint8_t pulseLED = 11; //Must be on PWM pin
uint8_t readLED = 13; //Blinks with each data read


bool setupMax30102(void)
{
    //Initialize sensor
   if (particleSensor.begin() == false)
   {
     LOG_DEBUG("MAX30105 was not found. Please check wiring/power. \n");
     return false;
   }

   return true;
}

void shutMax30102(void)
{
    LOG_INFO("shut MAX30102");
    particleSensor.shutDown();

}

void WakeMax30102(void)
{
    LOG_INFO("wake MAX30102");
    particleSensor.wakeUp();
}


//__attribute__((section(".highcode")))
int read_SPO2(void)
{
    static bool is_SPinit = false;

    if(!is_SPinit) {
        //Setup to sense up to 18 inches, max LED brightness
        uint8_t ledBrightness = 60; //Options: 0=Off to 255=50mA
        uint8_t sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
        uint8_t ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
        int sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
        int pulseWidth = 411; //Options: 69, 118, 215, 411
        int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

        particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

        bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

        //read the first 100 samples, and determine the signal range
        for (uint8_t i = 0 ; i < bufferLength ; i++)
        {
          while (particleSensor.available() == false) //do we have new data?
            particleSensor.check(); //Check the sensor for new data

          redBuffer[i] = particleSensor.getRed();
          irBuffer[i] = particleSensor.getIR();
          particleSensor.nextSample(); //We're finished with this sample so move to next sample

    //      PRINT("red=");
    //      PRINT("%d",redBuffer[i]);
    //      PRINT(",ir=");
    //      PRINT("%d",irBuffer[i]);
    //      PRINT("\n");
        }

        //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

        is_SPinit = true;
    }

    //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
//    while (1)
    {
      //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
      for (uint8_t i = 25; i < 100; i++)
      {
        redBuffer[i - 25] = redBuffer[i];
        irBuffer[i - 25] = irBuffer[i];
      }

      //take 25 sets of samples before calculating the heart rate.
      for (uint8_t i = 75; i < 100; i++)
      {
        while (particleSensor.available() == false) //do we have new data?
          particleSensor.check(); //Check the sensor for new data

//            digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample(); //We're finished with this sample so move to next sample

        //send samples and calculation result to terminal program through UART
        PRINT("red=");
        PRINT("%d", redBuffer[i]);
//            Serial.print(redBuffer[i], DEC);
        PRINT(",ir=");
        PRINT("%d ", irBuffer[i]);
//            Serial.print(irBuffer[i], DEC);

        PRINT(",HR=");
        PRINT("%d", heartRate);
//            Serial.print(heartRate, DEC);

        PRINT(",HRvalid=");
        PRINT("%d", validHeartRate);
//            Serial.print(validHeartRate, DEC);

        PRINT(",SPO2=");
        PRINT("%d", spo2);
//            Serial.print(spo2, DEC);

        PRINT(",SPO2Valid=");
        PRINT("%d", validSPO2);
//            Serial.println(validSPO2, DEC);
        PRINT("\n");
      }

      //After gathering 25 new samples recalculate HR and SP02
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    }


    return 0;

}



__attribute__((section(".highcode")))
int read_HeartRate(float *beatsPerMinute, int *beatAvg)
{

    const uint8_t RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
    static uint8_t rates[RATE_SIZE]; //Array of heart rates
    static uint8_t rateSpot = 0;
    static long lastBeat = 0; //Time at which the last beat occurred
//    static float beatsPerMinute;
//    static int beatAvg;

    static bool is_HRinit = false;

    if(is_HRinit != true)
    {
        // Initialize sensor
        if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
        {
          PRINT("MAX30105 was not found. Please check wiring/power. \n");
//          while (1);
        }
        PRINT("Place your index finger on the sensor with steady pressure.\n");

        particleSensor.setup(); //Configure sensor with default settings
        particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
        particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
        is_HRinit = true;
    }

    long irValue = particleSensor.getIR();

    if (checkForBeat(irValue) == true)
    {
      //We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();

      *beatsPerMinute = 60 / (delta / 1000.0);

      if (*beatsPerMinute < 255 && *beatsPerMinute > 20)
      {
        rates[rateSpot++] = (uint8_t)*beatsPerMinute; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable

        //Take average of readings
        *beatAvg = 0;
        for (uint8_t x = 0 ; x < RATE_SIZE ; x++)
          (*beatAvg) += rates[x];
        (*beatAvg) /= RATE_SIZE;
      }
    }

//    PRINT("IR=");
//    PRINT("%d", irValue);
//    PRINT(", BPM=");
//    PRINT("%.2f", *beatsPerMinute);
//    PRINT(", Avg BPM=");
//    PRINT("%d", *beatAvg);


    if (irValue < 50000){
//        PRINT(" No finger?\n");
        return 0;
    }

//    PRINT("\n");

    return 1;

}

