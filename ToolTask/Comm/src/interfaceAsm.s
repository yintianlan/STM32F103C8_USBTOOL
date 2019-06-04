
    RSEG	CODE:CODE(2)
    thumb

	PUBLIC	vJumpToWhere

/*-----------------------------------------------------------*/
/**
  * @brief Jump to program start address and excute
  * @param Program start address
  * @retval None
  */
vJumpToWhere
    cpsid i 				//Disable interrupt
	ldr		r1, [r0]		//Get MSP location
	ldr		r2, [r0, #4]	//Get resethanler address
	movs	r0, #0
	msr 	CONTROL, r0		//Use MSP
	msr 	MSP, r1
	bx		r2 				//Jump
     
    END