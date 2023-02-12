#include "main.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>
//#include <stdio.h>
//#include <errno.h>

void SystemClock_Config(void);

extern volatile uint32_t delayCounter[2];

#define MAX_ADC 			4096	// maksimi ADC lukema 12 bit
#define MAX_RUNNING_TIME 	30000L	// maksimi pumppausaika ms
#define MAX_FLUSHING_TIME	60000L	// maksimi huuhteluaika
#define RETRACTING_TIME		3000L	// maksimi huuhteluaika

#define BLINK_ON_TIME_MS 	200
#define BLINK_OFF_TIME_MS 	150

void setRunDelay( uint32_t msDelay ) {
	delayCounter[0] = msDelay;
}

uint32_t getRunDelay() {
	return delayCounter[0];
}

void setBlinkDelay( uint32_t msDelay ) {
	delayCounter[1] = msDelay;
}

uint32_t getBlinkDelay() {
	return delayCounter[1];
}

enum _runState { RS_IDLE, RS_RUNNING, RS_RETRACTING, RS_FLUSHING, RS_ESTOP };
enum _ledState { L_OFF, L_ON, L_BLINK };
enum _ledColor { C_GREEN, C_BLUE, C_ALL };
static bool blinking = false;
static bool ledIsOn = false;
static _ledColor blinker;

static _runState runState;
static bool  in_flush, in_start, in_estop;
static bool out_run_fwd, out_run_rev;

/* extern "C" {

#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

int _read(int fd, char *ptr, int len) {
	HAL_StatusTypeDef hstatus;
	if (fd == STDIN_FILENO ) {
		hstatus = HAL_UART_Receive(&huart2, (uint8_t *) ptr, 1, HAL_MAX_DELAY);
		hstatus = HAL_UART_Transmit(&huart2, (uint8_t *) ptr, 1, HAL_MAX_DELAY);
	}
	return 1;
}

int _write(int fd, char* ptr, int len) {
	HAL_StatusTypeDef hstatus;

	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		hstatus = HAL_UART_Transmit(&huart2, (uint8_t *) ptr, len, HAL_MAX_DELAY);
		if (hstatus == HAL_OK)
			return len;
		else
			return EIO;
	}
	errno = EBADF;
	return -1;
}

}
*/
void print( const char *_txt ) {
	HAL_UART_Transmit(&huart2, (uint8_t *)_txt, strlen(_txt), HAL_MAX_DELAY);
}

void println( const char *_txt ) {
	print( _txt );
	print( "\r\n");
}

void getInputs() {
	in_flush = ( HAL_GPIO_ReadPin(run_flush_GPIO_Port, run_flush_Pin) ) ? 0 : 1;
	in_start = ( HAL_GPIO_ReadPin(start_GPIO_Port, start_Pin) ) ? 0 : 1;
	in_estop = ( HAL_GPIO_ReadPin(estop_GPIO_Port, estop_Pin) ) ? 1 : 0;
}

void putOutputs() {
	HAL_GPIO_WritePin( run_fwd_GPIO_Port, run_fwd_Pin, ( (out_run_fwd) ? GPIO_PIN_RESET : GPIO_PIN_SET ) );
	HAL_GPIO_WritePin( run_rev_GPIO_Port, run_rev_Pin, ( (out_run_rev) ? GPIO_PIN_RESET : GPIO_PIN_SET ) );
}

void ledsOff() {
	HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, GPIO_PIN_RESET );
	HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, GPIO_PIN_RESET );
	ledIsOn = false;
}

void ledOn( _ledColor _col ) {
	HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, ( _col == C_GREEN ) ? GPIO_PIN_SET : GPIO_PIN_RESET );
	HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, ( _col == C_GREEN ) ? GPIO_PIN_RESET : GPIO_PIN_SET );
	ledIsOn = true;
}

void setLED( _ledState _st, _ledColor _col ) {
	switch ( _st ) {
	case L_OFF: {
		blinking = false;
		ledsOff();
		break;
	}
	case L_ON: {
		blinking = false;
		ledOn( _col );
		break;
	}
	case L_BLINK: {
		setBlinkDelay( BLINK_ON_TIME_MS );
		blinking = true;
		blinker = _col;
		ledOn( _col );
		break;
	}
	default: break;
	}
}

void doBlink() {
	if ( blinking ) {
		if ( getBlinkDelay() == 0 ) {
			if ( ledIsOn ) {
				ledsOff();
				setBlinkDelay( BLINK_OFF_TIME_MS );
			}
			else {
				ledOn( blinker );
				setBlinkDelay( BLINK_ON_TIME_MS );
			}
		}
	}
}

uint32_t getTimeSetpoint(uint32_t runningTime) {
	uint32_t result;
	HAL_ADC_Start( &hadc );
	HAL_ADC_PollForConversion( &hadc, HAL_MAX_DELAY );
	result = HAL_ADC_GetValue( &hadc );
	HAL_ADC_Stop( &hadc );
	return (result * runningTime ) / MAX_ADC;
}

int main(void) {

  HAL_Init();

 SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC_Init();
  MX_USART2_UART_Init();

  runState = RS_IDLE;
  out_run_fwd = out_run_rev = false;
  putOutputs();

  print("Maston multiFill controller v 1.0.0 build ");
  print(__DATE__);
  print( "  ");
  println(__TIME__ );

  while (1) {
	  getInputs();
	  // tsekataan hätäseis riippumatta missä ajotilassa ollaan
	  // jos aktivoitu, päädytään heti hätäseis-tilaan.
	  if ( in_estop ) {
		  ledsOff();
		  out_run_fwd = out_run_rev = false;
		  runState = RS_ESTOP;
	  }
	  switch ( runState ) {
	  	  // odottaa käynnistyskäskyä normaalissa ajotilassa
		  case RS_IDLE : {
			  if ( in_start ) {
				  out_run_fwd = true;
				  if ( in_flush ) {
					  setLED(L_ON, C_GREEN );
					  setRunDelay( MAX_FLUSHING_TIME );
					  runState = RS_FLUSHING;
				  }
				  else {
					  setLED( L_BLINK, C_BLUE );
					  setRunDelay( getTimeSetpoint( MAX_RUNNING_TIME ) );
					  runState = RS_RUNNING;
				  }
				  do {  // odotetaan että starttinappi päästetään
					  getInputs();
					  if ( !getRunDelay() ) {	// varmistus jos käyttäjä roikkuu napissa tai se hirttää
						  out_run_fwd = false;
					  }
				  } while ( in_start );
			  }
			  break;
		  }
		  // pumppaus käynnissä
		  case RS_RUNNING : {
			  if ( !getRunDelay() ) {
				  out_run_fwd = false;
				  out_run_rev = true;
				  setLED( L_OFF, C_ALL );
				  setRunDelay( RETRACTING_TIME );
				  runState = RS_RETRACTING;
			  }
			  break;
		  }
		  // litkun takaisinimaisu pumppauksen päätteeksi, estetään tipan valuminen suutimesta
		  case RS_RETRACTING : {
			  if ( !getRunDelay() ) {
				  out_run_fwd = false;
				  out_run_rev = false;
				  setLED( L_OFF, C_ALL );
				  runState = RS_IDLE;
			  }
			  break;
		  }
		  // huuhtelukäyttö
		  case RS_FLUSHING : {
			  if ( in_start || !getRunDelay() ) {
				  out_run_fwd = false;
				  putOutputs();
				  do {
					  getInputs();
				  } while ( in_start );
				  runState = RS_IDLE;
			  }
			  break;
		  }
		  case RS_ESTOP : {
			  if ( !in_estop ) {
				  setLED(L_ON, C_GREEN );
				  runState = RS_IDLE;
				  println("RUNSTATE ESTOP -> IDLE");
			  }
			  break;
		  }
		  default : {
			  // virhe. Tänne ei pitäisi joutua koskaan
			  break;
		  }
	  }
	  doBlink();
	  putOutputs();
  }


}



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
