/*
 * test.cpp
 *
 *  Created on: Nov 18, 2022
 *      Author: tj
 */

//test the logic of C++ code

extern "C" {
#include "VCP_UART.h"
}

//trace counter
int c;

class MyClass {
public:
	MyClass()
	{
		//this CANNOT work!: UART is not initialized!
		//print_log(UART_OUT, "MyClass object created\r\n");
		var = 1;
		c = 1;
	}

	int UseFunc(void)
	{
		c++;
		//even here we CANNOT use yet UART! not yet initialized
		return var;
	}

	void InitFunc(void)
	{
		c++;
		//this is actually needed to initialize, before we call UseFunc()
		var = 2;
	}

private:
	int var;
};

MyClass *GObj;

void testOOP()
{
	//we use before we do our actual configuration, via InitFunc() needed to call first!
	static int lStaticVar = GObj->UseFunc();

	print_log(UART_OUT, "c=%d | lStaticVar = %x\r\n", c, lStaticVar);
	print_log(UART_OUT, "call InitFunc()\r\n");
	GObj = new MyClass;
	GObj->InitFunc();

	lStaticVar = GObj->UseFunc();
	print_log(UART_OUT, "c=%d | lStaticVar = %x\r\n", c, lStaticVar);
}

extern "C" {
    void testOOP_CCall(void)
	{
    	testOOP();
	}
}

