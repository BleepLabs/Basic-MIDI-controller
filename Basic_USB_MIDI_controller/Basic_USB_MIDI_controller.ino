#include <Bounce2.h>  // Bounce library makes button change detection easy
#define BOUNCE_LOCK_OUT //this tells it what mode to be in. I think it's the better one for music

#define button1_pin 3
#define pot1_pin A0
#define pot2_pin A1

//midi notes to piano notes https://docs.google.com/spreadsheets/d/1t9yLnGfvo_sVzb5nU9dRVeX2SL9OWO1yOi0GPU40YDM/edit?usp=sharing
#define button1_note 60
#define pot1_cc 20
#define pot2_cc 21


int channel = 1; // MIDI channel to use while sending

Bounce button1 = Bounce(button1_pin, 5);  // 5 = 5 ms debounce time

int pot_pins[8] = {A0, A1};
int prev_pot_reading[8];
int pot_reading[8];
uint32_t current_time;
uint32_t prev_time[6];
uint32_t midiblink;

void setup() {
  pinMode(button1_pin, INPUT_PULLUP);

  pinMode(13, OUTPUT);
  analogReadResolution(12); //AnalogReads will return 0-4095
  analogReadAveraging(64); //smooth analogreads
}

void loop() {
  current_time = millis();
  button1.update();

  // Note On messages when each button is pressed
  if (button1.fell()) {
    usbMIDI.sendNoteOn(button1_note, 127, channel);
    midiblink = current_time;

  }
  if (button1.rose()) {
    usbMIDI.sendNoteOn(button1_note, 0, channel);
    midiblink = current_time;

  }

  if (current_time - prev_time[1] > 2) {
    prev_time[1] = current_time;

    prev_pot_reading[1] = pot_reading[1];
    int raw_reading1 = analogRead(pot1_pin) >> 5; //12 bits to 7
    pot_reading[1] = smooth(1, 17, raw_reading1);

    if (prev_pot_reading[1] != pot_reading[1]) {
      midiblink = current_time;
      usbMIDI.sendControlChange(pot1_cc, pot_reading[1], channel);
    }
    //Serial.println(raw_reading1);

    prev_pot_reading[2] = pot_reading[2];
    int raw_reading2 = analogRead(pot2_pin) >> 5; //12 bits to 7
    pot_reading[2] = smooth(2, 17, raw_reading2);

    if (prev_pot_reading[2] != pot_reading[2]) {
      midiblink = current_time;
      usbMIDI.sendControlChange(pot2_cc, pot_reading[2], channel);
    }
  }

  if (midiblink > 0) {
    digitalWrite(13, 1);
    if (millis() - midiblink > 10) {
      midiblink = 0;
      digitalWrite(13, 0);
    }
  }

  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {
  }
}



//based on https://playground.arduino.cc/Main/DigitalSmooth/
// This function continuously samples an input and puts it in an array that is "samples" in length.
// This array has a new "raw_in" value added to it each time "smooth" is called and an old value is removed
// It throws out the top and bottom 15% of readings and averages the rest

#define maxarrays 10 //max number of different variables to smooth
#define maxsamples 30 //max number of points to sample and 
//reduce these numbers to save RAM

unsigned int smoothArray[maxarrays][maxsamples];

// sel should be a unique number for each occurrence
// samples should be an odd number greater that 7. It's the length of the array. The larger the more smooth but less responsive
// raw_in is the input. positive numbers in and out only.

unsigned int smooth(byte sel, unsigned int samples, unsigned int raw_in) {
  int j, k, temp, top, bottom;
  long total;
  static int i[maxarrays];
  static int sorted[maxarrays][maxsamples];
  boolean done;

  i[sel] = (i[sel] + 1) % samples;    // increment counter and roll over if necessary. -  % (modulo operator) rolls over variable
  smoothArray[sel][i[sel]] = raw_in;                 // input new data into the oldest slot

  for (j = 0; j < samples; j++) { // transfer data array into anther array for sorting and averaging
    sorted[sel][j] = smoothArray[sel][j];
  }

  done = 0;                // flag to know when we're done sorting
  while (done != 1) {      // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (samples - 1); j++) {
      if (sorted[sel][j] > sorted[sel][j + 1]) {    // numbers are out of order - swap
        temp = sorted[sel][j + 1];
        sorted[sel] [j + 1] =  sorted[sel][j] ;
        sorted[sel] [j] = temp;
        done = 0;
      }
    }
  }

  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((samples * 15)  / 100), 1);
  top = min((((samples * 85) / 100) + 1  ), (samples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j < top; j++) {
    total += sorted[sel][j];  // total remaining indices
    k++;
  }
  return total / k;    // divide by number of samples
}
