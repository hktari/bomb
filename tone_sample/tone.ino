void setup(){
    // pinMode(A5)
    Serial.begin(115200);

    Serial.println("BEGIN");
}

void loop(){

    Serial.println("TONE");
    tone(6, 500, 1000);

    delay(5000);
}