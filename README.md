# arduino-hub08-counter
A small counter project on Arduino

## Setup

1. Download ZIP of [this](https://github.com/PaulStoffregen/TimerOne )  and [this](https://github.com/emgoz/HUB08SPI) libraries
2. Open sketch in Arduino IDE
3. go to ```Sketch -> include library -> add .ZIP library``` and add downloaded libs
4. compile and upload sketch
5. find ```#define FIRST_RUN true // false``` in the code and replace it with ```#define FIRST_RUN false```
6. compile and upload again

Optionally, you can change the font to ```Font_RUS1.h``` or ```Font_RUS3.h``` in the line ```#include "Font_RUS2.h```

**Thank [Him](https://www.drive2.ru/c/490697746899009800/) for making this possible**
