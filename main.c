#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>



#define JOY_LIMIT_UP 15000
#define JOY_LIMIT_DOWN 200
/********************************************
 * Global Variables shared between handlers
 ********************************************/

#define PERIOD  60000


/********************************************************
 * DISPLAY Section
 ********************************************************/
// Color parameters for drawing on the screen - see grlib.h
#define TEXTCOL GRAPHICS_COLOR_WHITE
#define BACKCOL GRAPHICS_COLOR_BLACK


//variables
volatile unsigned int state = 0;
volatile unsigned int score = 0;
volatile unsigned int save = 0;
volatile int instruction;

volatile unsigned int counter;
volatile unsigned int debounce_counter =0;
volatile unsigned int diff;
volatile unsigned int disp_len;
volatile unsigned int high_score;
int level = 0;

/* ADC results buffer */
uint16_t resultsBuffer[3];           // latest readings from the analog inputs


volatile unsigned char last_button1;
volatile unsigned char last_button2;    // this button2 is either 0 or BIT5 (P3.5)
volatile unsigned char last_joy;    // this button2 is either 0 or BIT5 (P3.5)
char str_score[10];
char str_level[10];
char str_name[10];


//flags
uint8_t joy_flag = 0;
uint8_t button1_flag = 0;
uint8_t timer_flag = 0;
uint8_t button2_flag = 0;

// Graphics Globals (used by ADC14 handler)
Graphics_Context g_sContext;    // graphics context for grlib
uint16_t xscreen, yscreen;      // current screen location coordinates

//what to add in
//Switch to the flash data section
#pragma SET_DATA_SECTION(".myinfo")
volatile const unsigned int Highest_Score=0;  // causes the loader to place constant 0 at this flash address

// switch back to the default data section
#pragma SET_DATA_SECTION()




//headers

void check_button1();

/*******************************************************
 * Button/LED Initialization
 *******************************************************/

void init_button(){ // All GPIO's are already inputs if we are coming in after a reset
    //Button 1 (5.1)
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5,GPIO_PIN1);
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P5,GPIO_PIN1,GPIO_HIGH_TO_LOW_TRANSITION);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P5,GPIO_PIN1);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P5,GPIO_PIN1);

    P5->DIR &= ~BIT1;                // Clear Direction
    last_button1 = P5->IN & BIT1;   // Initialize the previous state of the button1

    //Button 2 (3.5)
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P3,GPIO_PIN5);
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P3,GPIO_PIN5,GPIO_HIGH_TO_LOW_TRANSITION);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P3,GPIO_PIN5);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P3,GPIO_PIN5);

    //Joystick Button (4.1)
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4,GPIO_PIN1);
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P4,GPIO_PIN1,GPIO_HIGH_TO_LOW_TRANSITION);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4,GPIO_PIN1);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P4,GPIO_PIN1);

    P4->DIR &= ~BIT1;
    last_joy = P4->IN & BIT1;
}

void init_LED() {
    //green
    GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN4);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4);

    //red
    GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN6);

    //blue
    GPIO_setAsOutputPin(GPIO_PORT_P5,GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN6);
}

void init_ADC(){ // initialize the ADC handler and the joystick and the accelerometer

     /* Configure Pin 6.0 (A15) and 4.4 (A9) and 4.2 (A11) to be analog inputs ('tertiary function') */
     /* see the port 4 and port 6 pinout details of the MSP432p401r */
     MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
     MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);
     MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN2, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Initializing ADC (ADCOSC/64/8)
     * drive from the internal ASD oscillator
     * with predivider 64 and divider 8, no routing to internal pins */

    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, ADC_NOROUTE);
    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat)
     * Basic operation is sequence mode with
     *   ADC-MEM0 -- A15
     *   ADC_MEM1 -- A9
     *
     *   NO automatic repeats */

    MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM2, false); // use MEM...MEM1 channels
    // configure each memory channel:

    //4.4 - Joystick Vertical - MEMO
    MAP_ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);
    //4.2 - Accelerometer Z - MEM1
    MAP_ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A11, ADC_NONDIFFERENTIAL_INPUTS);
    //6.0 - Joystick Horizontal - MEM2
    MAP_ADC14_configureConversionMemory(ADC_MEM2,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

    /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
     * is complete and enabling conversions */
    MAP_ADC14_enableInterrupt(ADC_INT1);
    /* Setting up the sample timer to automatically step through the sequence
     * convert.*/
    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);
    /* Enable conversions (must be triggered using ASC14_toggle_conversion()) */
    MAP_ADC14_enableConversion();
    //MAP_ADC14_toggleConversionTrigger();
}

void init_WDT(){
    MAP_WDT_A_initIntervalTimer(WDT_A_CLOCKSOURCE_SMCLK,WDT_A_CLOCKITERATIONS_8192);
    MAP_WDT_A_startTimer(); // start the timer
}


const Timer_A_UpModeConfig upConfig =
{
     TIMER_A_CLOCKSOURCE_SMCLK,
     TIMER_A_CLOCKSOURCE_DIVIDER_64,
     PERIOD,
     TIMER_A_TAIE_INTERRUPT_DISABLE,
     TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
     TIMER_A_SKIP_CLEAR
};

const Timer_A_CompareModeConfig ccr0_Config =
{
     TIMER_A_CAPTURECOMPARE_REGISTER_0,
     TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
     TIMER_A_OUTPUTMODE_OUTBITVALUE,
     PERIOD
};


void TA0_0_Handler(){
    state = 3;
        MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);

}




void init_display(){
    /*  All init code for the display */
    /* Initializes display */
    Crystalfontz128x128_Init();
    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,&g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, TEXTCOL);
    Graphics_setBackgroundColor(&g_sContext, BACKCOL);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);

    xscreen=0;
    yscreen=0;  // just use origin, first write is a background
}


/*******************************************************
 * Interupt handlers
 *******************************************************/


void WDT_A_IRQHandler(void)
{

    check_button1();

    if (state == 0) //BEGINING
    {

      GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN6); //red off
      GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);//green light on

      Graphics_drawStringCentered(&g_sContext,
                                      "WELCOME TO BOP-IT!",
                                      AUTO_STRING_LENGTH,
                                      64,
                                      64,
                                      OPAQUE_TEXT);

      Graphics_drawStringCentered(&g_sContext,
                                      "Press Start to Play",
                                      AUTO_STRING_LENGTH,
                                      64,
                                      84,
                                      OPAQUE_TEXT);
      //Reset
      score = 0;
      diff = 0;
      level = 0;


    }
    else if(state == 1)//GAME init
    {
          GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN6); //blue on
          GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4);//green light off

          counter = 0;
          //Level stuff
          if(score%5 == 0)
          {
              if (diff < 1350)
              {
              diff += 70
                      ;
              }
              level++;
          }

          char* command;
          Graphics_clearDisplay  (&g_sContext);

          //reset
          resultsBuffer[0] = 8000;
          resultsBuffer[1] = 11111;
          resultsBuffer[2] = 8000;
          joy_flag = 0;
          button2_flag = 0;

          instruction = rand()%7;//random instruction

          switch(instruction)//CASSES FOR THE
          {
          case 0:
              command = "BOP-IT!";
              break;
          case 1:
              command = "UP";
              break;
          case 2:
              command = "RIGHT";
              break;
          case 3:
              command = "DOWN";
              break;
          case 4:
              command = "LEFT";
              break;
          case 5:
              command = "BUTTON";
              break;
          case 6:
              command = "SHAKE IT";
              break;
          default:
              command = "Default";
              break;

          }
          Graphics_drawStringCentered(&g_sContext,
                                      command,
                                      AUTO_STRING_LENGTH,
                                      64,
                                      64,
                                      OPAQUE_TEXT);



          sprintf(str_score, "Score: %d", score);

          Graphics_drawStringCentered(&g_sContext,
                                      str_score,
                                      AUTO_STRING_LENGTH,
                                      28,
                                      116,
                                      OPAQUE_TEXT);

          sprintf(str_level, "Level: %d", level);
          Graphics_drawStringCentered(&g_sContext,
                                      str_level,
                                      AUTO_STRING_LENGTH,
                                      100,
                                      116,
                                      OPAQUE_TEXT);

          state = 2;

    }

    else if(state == 2)//
    {

        GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN6); //blue on
        MAP_ADC14_toggleConversionTrigger(); // trigger the next conversion
        counter++;

        check_button2();
        check_joy();

        if((counter + diff) == 1500)
        {
            state = 3;
            Graphics_clearDisplay  (&g_sContext);
        }

        else if(instruction == 0)//BOB IT
          {
              if(joy_flag == 1)
              {
                  score++;
                  state = 1;
              }
              else if(button2_flag == 1||resultsBuffer[2] < 0||resultsBuffer[0] < 500
                      ||resultsBuffer[2] > 15500||resultsBuffer[0] > 15500)
              {
                  state = 3;
                  Graphics_clearDisplay  (&g_sContext);
              }

          }
        else if(instruction == 1)//Push up
        {
            if(resultsBuffer[0] > JOY_LIMIT_UP)
            {
                score++;
                state = 1;
            }
            else if(button2_flag == 1||resultsBuffer[2] < 500||resultsBuffer[0] < 500
                    ||resultsBuffer[2] > 15500||joy_flag == 1)
            {
                state = 3;
                Graphics_clearDisplay  (&g_sContext);
            }
        }
        else if(instruction == 2)//Push right
        {
            if(resultsBuffer[2] > JOY_LIMIT_UP)
            {
                score++;
                state = 1;
            }
            else if(button2_flag == 1||resultsBuffer[2] < 500||resultsBuffer[0] < 500
                    ||resultsBuffer[0] > 15500||joy_flag == 1)
            {
                state = 3;
                Graphics_clearDisplay  (&g_sContext);
            }
        }

  else if(instruction == 3)//Push down
        {
            if(resultsBuffer[0] < JOY_LIMIT_DOWN )
            {
                score++;
                state = 1;
            }
            else if(button2_flag == 1||resultsBuffer[2] < 500
                    ||resultsBuffer[2] > 15500||resultsBuffer[0] > 15500||joy_flag == 1)
            {
                state = 3;
                Graphics_clearDisplay  (&g_sContext);
            }
        }
        else if(instruction == 4)//Push left
        {
            if(resultsBuffer[2] < JOY_LIMIT_DOWN)
            {
                score++;
                state = 1;
            }
            else if(button2_flag == 1||resultsBuffer[0] < 500||resultsBuffer[2] > 15500||
                    resultsBuffer[0] > 15500||joy_flag == 1)
            {
                state = 3;
                Graphics_clearDisplay  (&g_sContext);
            }
        }
        else if(instruction == 5)//Button
          {
              if(button2_flag == 1)
              {
                  score++;
                  state = 1;
              }
              else if(resultsBuffer[2] < 500||resultsBuffer[0] < 500
                      ||resultsBuffer[2] > 15500||resultsBuffer[0] > 15500||joy_flag == 1)
              {
                  state =3;
                  Graphics_clearDisplay  (&g_sContext);
              }
          }
        else if(instruction == 6)//Shake
        {
            if(resultsBuffer[1] > 12000)
            {
                score++;
                state = 1;
            }
        }

        disp_len = 120*((double)counter/(1500-diff));
        if ( disp_len % 5 == 0 ) {
        Graphics_drawStringCentered(&g_sContext,
                                    ".",
                                    1,
                                    disp_len,
                                    10,
                                    OPAQUE_TEXT);
        }


    }

    else if(state == 3)//END
    {
        if(score > Highest_Score)
        {
            high_score = score;

            //update boot count in flash
            MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
            MAP_FlashCtl_eraseSector((uint32_t) &Highest_Score);
            MAP_FlashCtl_programMemory( (void *) &high_score, (void *) &Highest_Score, sizeof(Highest_Score));
            MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
        }

        //Timer_A_stopTimer (TIMER_A0_BASE);
        GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN6); //blue off
        GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN6);//red

        char h_score[20];
        sprintf(h_score, "HighScore: %d",Highest_Score);
        Graphics_drawStringCentered(&g_sContext,
                                        h_score,
                                        AUTO_STRING_LENGTH,
                                        64,
                                        44,
                                        OPAQUE_TEXT);

        Graphics_drawStringCentered(&g_sContext,
                                        "YOU LOST!",
                                        AUTO_STRING_LENGTH,
                                        64,
                                        54,
                                        OPAQUE_TEXT);

        Graphics_drawStringCentered(&g_sContext,
                                    str_score,
                                    AUTO_STRING_LENGTH,
                                    64,
                                    84,
                                    OPAQUE_TEXT);

        Graphics_drawStringCentered(&g_sContext,
                                    str_level,
                                    AUTO_STRING_LENGTH,
                                    64,
                                    94,
                                    OPAQUE_TEXT);
    }
}


void check_button1()
{
    /* Handles Button 1 */
        unsigned char button1;
        button1 = P5->IN & BIT1;
        if((button1 == 0) && last_button1)
        {
            /* Switch states */
            state = (state+1)%4;
            Graphics_clearDisplay  (&g_sContext);

        }
        last_button1 = button1;
}

void check_button2()
{
    /* Handles Button 2 */
        unsigned char button2;
        button2 = P3->IN & BIT5;
        if((button2 == 0) && last_button2)
        {
            /* Switch states */
            button2_flag = 1;
        }
        last_button2 = button2;
}

void check_joy()
{
    unsigned char joy;
    joy = P4->IN & BIT1;
    if((joy == 0) && last_joy)
    {
        /* Switch states */
        joy_flag = 1;
    }
    last_joy = joy;
}

void counter_reset()
{
    init_timer();
    Timer_A_startCounter (TIMER_A0_BASE, TIMER_A_UP_MODE);
}

void ADC14_IRQHandler(void)
 {
     uint64_t status;
     status = MAP_ADC14_getEnabledInterruptStatus();
     MAP_ADC14_clearInterruptFlag(status);

     /* ADC_MEM1 conversion completed */
     if(status & ADC_INT1)
     {
         /* Store ADC14 conversion results */
         resultsBuffer[0] = MAP_ADC14_getResult(ADC_MEM0); //4.4 - Joystick Vertical - MEMO
         resultsBuffer[1] = MAP_ADC14_getResult(ADC_MEM1); //4.2 - Accelerometer Z - MEM1
         resultsBuffer[2] = MAP_ADC14_getResult(ADC_MEM2); //6.0 - Joystick Horizontal - MEM2
     }
 }
//
void main(void)
 {

    // Initializing
    init_WDT(); // setup the WatchDog Timer
    //  init_timer(); // setup Timer A
    init_button(); // setup the Buttons
    init_display(); // setup the display
    init_LED(); // setup the LEDs
    init_ADC(); //setup the ADC

    state = 0;

    MAP_Interrupt_disableSleepOnIsrExit();   // Specify that after an interrupt, the CPU wakes up
    MAP_Interrupt_enableMaster();

    /* Enable Interrupts at the NVIC level */
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableInterrupt(INT_WDT_A);

    while(1)
    {

        MAP_PCM_gotoLPM0();
        __no_operation(); //  For debugger
    }

}


