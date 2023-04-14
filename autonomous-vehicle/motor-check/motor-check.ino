// Motor A
#define in1       33       //digital
#define in2       25       //digital
#define en1       32       //analog

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

void setup() {
  // sets the pins as outputs:
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(en1, OUTPUT);
  
  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(en1, pwmChannel);

  Serial.begin(115200);

  // testing
  Serial.print("Testing DC Motor...");
}

void loop() {
  // Move the DC motor forward at maximum speed
  Serial.println("Moving Forward");
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH); 
  delay(2000);

  // Stop the DC motor
  Serial.println("Motor stopped");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  delay(1000);

  // Move DC motor backwards at maximum speed
  Serial.println("Moving Backwards");
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW); 
  delay(2000);

  // Stop the DC motor
  Serial.println("Motor stopped");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  delay(1000);

  // Move DC motor forward with increasing speed
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  while (dutyCycle <= 255){
    ledcWrite(pwmChannel, dutyCycle);   
    Serial.print("Forward with duty cycle: ");
    Serial.println(dutyCycle);
    dutyCycle = dutyCycle + 5;
    delay(500);
  }
  dutyCycle = 200;
}
