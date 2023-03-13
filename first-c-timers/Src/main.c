/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>
#include "stm32f303xc.h"


#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif


// enable the clocks for desired peripherals (GPIOA, C and E)
void enable_clocks() {

	RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOEEN;

	/*
	@ enable the clocks for peripherals (GPIOA, C and E)
	LDR R0, =RCC  @ load the address of the RCC address boundary (for enabling the IO clock)
	LDR R1, [R0, #AHBENR]  @ load the current value of the peripheral clock registers
	ORR R1, 1 << GPIOA_ENABLE | 1 << GPIOC_ENABLE | 1 << GPIOE_ENABLE  @ 21st bit is enable GPIOE clock, 17 is GPIOA clock
	STR R1, [R0, #AHBENR]  @ store the modified register back to the submodule
	BX LR @ return from function call
	*/


	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	/*
	LDR R0, =RCC	@ load the base address for the timer
	LDR R1, [R0, APB1ENR] 	@ load the peripheral clock control register
	ORR R1, 1 << TIM2EN @ store a 1 in bit for the TIM2 enable flag
	STR R1, [R0, APB1ENR] @ enable the timer
	BX LR @ return
	*/
}

// initialise the discovery board I/O (just outputs: inputs are selected by default)
void initialise_board() {

	// get a pointer to the second half word of the MODER register (for outputs pe8-15)
	uint16_t *led_output_registers = ((uint16_t *)&(GPIOE->MODER)) + 1;
	*led_output_registers = 0x5555;

	/*
	LDR R0, =GPIOE 	@ load the address of the GPIOE register into R0
	LDR R1, =0x5555 @ load the binary value of 01 (OUTPUT) for each port in the upper two bytes
					@ as 0x5555 = 01010101 01010101
	STRH R1, [R0, #MODER + 2]   @ store the new register values in the top half word representing
								@ the MODER settings for pe8-15
	BX LR @ return from function call
	*/
}


void trigger_prescaler() {

	TIM2->ARR = 0x01;
	TIM2->CNT = 0x00;
	asm("NOP");
	asm("NOP");
	asm("NOP");
	TIM2->ARR = 0xffffffff;
	/*
	@ Use (TIMx_EGR) instead (to reset the clock)

	@ This is a hack to get the prescaler to take affect
	@ the prescaler is not changed until the counter overflows
	@ the TIMx_ARR register sets the count at which the overflow
	@ happens. Here, the reset is triggered and the overflow
	@ occurs to make the prescaler take effect.

	@ In your code, you should be using the ARR register to
	@ set the maximum count for the timer

	@ store a value for the prescaler
	LDR R0, =TIM2	@ load the base address for the timer

	LDR R1, =0x1 @ make the timer overflow after counting to only 1
	STR R1, [R0, TIM_ARR] @ set the ARR register

	LDR R8, =0x00
	STR R8, [R0, TIM_CNT] @ reset the clock
	NOP
	NOP

	LDR R1, =0xffffffff @ set the ARR back to the default value
	STR R1, [R0, TIM_ARR] @ set the ARR register

	BX LR
	*/
}



//  general purpose timer registers page 647
int main(void)
{

	uint32_t on_time = 0x10000;
	uint32_t off_time = 0x80000;


	enable_clocks();
	initialise_board();

	/*
	BL enable_timer2_clock
	BL enable_peripheral_clocks
	BL initialise_discovery_board
	*/


	TIM2->CR1 |= TIM_CR1_CEN;
	/*
	@ start the counter running
	LDR R0, =TIM2	@ load the base address for the timer

	MOV R1, #0b1 @ store a 1 in bit zero for the CEN flag
	STR R1, [R0, TIM_CR1] @ enable the timer
	*/

	TIM2->PSC = 0x04;
	trigger_prescaler();
	/*
	@ store a value for the prescaler
	LDR R0, =TIM2	@ load the base address for the timer
	MOV R1, #0x04 @ put a prescaler in R1
	STR R1, [R0, TIM_PSC] @ set the prescaler register

	BL trigger_prescaler

	@ questions for timed_loop
	@  what can make it run faster/slower (there are multiple ways)
	*/

	uint8_t led_mask_pattern = 0b01010101;
	uint8_t *led_output_register = ((uint8_t*)&(GPIOE->ODR)) + 1;

	/* Loop forever */
	for(;;) {

		// turn on the LEDs
		*led_output_register = led_mask_pattern;

		/*
		pwm_loop:
			@ store the current light pattern (binary mask) in R7
			LDR R7, =0b01010101 @ load a pattern for the set of LEDs (every second one is on)

			LDR R1, =on_time
			LDR R1, [R1]

			LDR R2, =off_time
			LDR R2, [R2]

		*/


		while (TIM2->CNT < on_time) {} ;
		/*
		pwm_start:

			@ reset the counter
			LDR R0, =TIM2
			LDR R8, =0x00
			STR R8, [R0, TIM_CNT]

			LDR R0, =GPIOE  @ load the address of the GPIOE register into R0
			STRB R7, [R0, #ODR + 1]   @ store this to the second byte of the ODR (bits 8-15)

		pwm_loop_inner:

			@ load the current time from the counter
			LDR R0, =TIM2  @ load the address of the timer 2 base address
			LDR R6, [R0, TIM_CNT]

			CMP R6, R1
			BGT pwm_turned_off

			B pwm_loop_inner

		*/

		// turn off the LEDs
		*led_output_register = 0x00;

		while (TIM2->CNT < off_time) {};

		/*
		pwm_turned_off:

			LDR R0, =GPIOE  @ load the address of the GPIOE register into R0
			MOV R3, 0x00
			STRB R3, [R0, #ODR + 1]   @ store this to the second byte of the ODR (bits 8-15)

			CMP R6, R2
			BGT pwm_start

			B pwm_loop_inner
		*/

	}
}


