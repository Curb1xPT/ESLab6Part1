#include "mbed.h"
#include "LCDi2c.h"

#define CODE_LENGTH 100

LCDi2c lcd(LCD20x4);

char str[32];
char correctCode[CODE_LENGTH] = {'1', '3', '5', '9'};
char enterCode[CODE_LENGTH];
int enterDigits = 0;
int eventsIndex = 0;

AnalogIn potentiometer(A0);
AnalogIn lm35(A1);
AnalogIn mq2(A3);
DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
PwmOut buzzer(D9);
UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

void uartTask();
void availableCommands();

float lm35Reading = 0.0;
float lm35TempC = 0.0;

float analogReadingScaledWithTheLM35Formula(float analogReading);

float potentiometerReading = 0.00f;

bool overTempDetect = false;
bool overTempDetectState = false;
bool gasDetectState = false;

DigitalOut keypadRow[4] = {PB_3, PB_5, PC_7, PA_15};
DigitalIn keypadCol[4] = {PB_12, PB_13, PB_15, PC_6};

char matrixKeypadIndexToCharArray[] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

typedef struct systemEvent {
    time_t seconds;
    char typeOfEvent[14];
} systemEvent_t;

systemEvent_t arrayOfStoredEvents[100];

void inputsInit() {
    for(int i = 0; i < 4; i++) {
        keypadCol[i].mode(PullUp);
    }
}

void outputsInit() {
    alarmLed = 0;
    incorrectCodeLed = 0;
    buzzer = 0;
}

char matrixKeypadScan() {
    for(int row = 0; row < 4; row++) {
        for(int i = 0; i < 4; i++) {
            keypadRow[i] = 1;
        }
        keypadRow[row] = 0;

        for(int col = 0; col < 4; col++) {
            if(keypadCol[col] == 0) {
                return matrixKeypadIndexToCharArray[row * 4 + col];
            }
        }
    }
    return '\0';
}

char matrixKeypadUpdate() {
    static char lastKey = '\0';
    static Timer debounceTimer;
    static bool debounceTimerStarted = false;
    char currentKey = matrixKeypadScan();

    if(currentKey != '\0' && lastKey == '\0' && !debounceTimerStarted) {
        debounceTimer.reset();
        debounceTimer.start();
        debounceTimerStarted = true;
    }

    if(debounceTimerStarted && debounceTimer.elapsed_time().count() > 20000) {
        debounceTimer.stop();
        debounceTimerStarted = false;
        lastKey = currentKey;
        if(currentKey != '\0') {
            return currentKey;
        }
    }

    if(currentKey == '\0') {
        lastKey = '\0';
    }

    return '\0';
}

int main() {

    lcd.cls();

    inputsInit();
    outputsInit();

    uartUsb.write("\nSystem is on.\r\n", 16);
    uartUsb.write("\nPress 'q' to list available commands.\r\n", 40);

    while (true) {

        uartTask();

        float potentiometerReading = potentiometer.read();

        lm35Reading = lm35.read();
        lm35TempC = analogReadingScaledWithTheLM35Formula(lm35Reading);

        char key = matrixKeypadUpdate();

/*
        if (key != '\0') {
        lcd.cls();
        lcd.locate(0, 0);
        sprintf(str, "Key: %c", key);
        lcd.printf("%s", str);
        ThisThread::sleep_for(500ms);
        }
*/
        if (key != '\0') {
            if (key == '2') {
                    str[0] = '\0';
                    lcd.locate(0, 0);
                    sprintf(str, "LM35:%.2f C\n", lm35TempC);
                    lcd.printf("%s \n", str);
                    lcd.locate(10, 0);
                    lcd.printf("%c", (char)223);
                    ThisThread::sleep_for(1000ms);
                    lcd.locate(0, 2);
                    str[0] = '\0';
            } else if (key == '3') {
                    float gasLevel = mq2.read(); 
                    str[0] = '\0';
                    lcd.locate(0, 2);
                    if (gasLevel > 0.8f) {
                    sprintf(str, "Gas: OFF");
                    lcd.printf("%s \n", str);
                    ThisThread::sleep_for(1000ms);
                    lcd.locate(0, 0);
                    str[0] = '\0';
                    } else {
                        str[0] = '\0';
                        lcd.locate(0, 2);
                        sprintf(str, "Gas: ON");
                        lcd.printf("%s \n", str);
                        ThisThread::sleep_for(1000ms);
                        lcd.locate(0, 0);
                        str[0] = '\0';
                    }

            }
        }
    
        ThisThread::sleep_for(1ms);

        uartTask();
    }
}


void uartTask() {
    char receivedChar = '\0';
    char str[100];
    int stringLength;
    if(uartUsb.readable()) {
        uartUsb.read(&receivedChar, 1);
        switch (receivedChar) {

            case 'q':
            availableCommands();
            break;

        default:
            availableCommands();
            break;

        }
    }
}

void availableCommands()
{
    uartUsb.write("Available commands:\r\n", 21);
    uartUsb.write("Press '2' on the keypad to display state of gas detector\r\n\r\n", 61);
    uartUsb.write("Press '3' on the keypad to display state of temp. detector\r\n\r\n", 63);
}

float analogReadingScaledWithTheLM35Formula(float analogReading)
{
    return analogReading * 330.0;
}
