#pragma once

//Functions exported from DLL.
//For easy inclusion is user projects.
//Original InpOut32 function support
void	_stdcall Out32(short PortAddress, short data);
short	_stdcall Inp32(short PortAddress);

//My extra functions for making life easy
int	_stdcall IsInpOutDriverOpen();  //Returns TRUE if the InpOut driver was opened successfully
int	_stdcall IsXP64Bit();			//Returns TRUE if the OS is 64bit (x64) Windows.
