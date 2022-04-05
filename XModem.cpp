
#include "XModem.h"
#include <string.h>

#ifdef UTEST
#include "CppUTestExt/MockSupport.h"
#endif
const unsigned char XModem::NACK = 21;
const unsigned char XModem::ACK =  6;

const unsigned char XModem::SOH =  1;
const unsigned char XModem::EOT =  4;
const unsigned char XModem::CAN =  0x18;

const int XModem::receiveDelay=10000;
const int XModem::rcvRetryLimit = 10;


XModem::XModem() // New Blank Constructor
{

}

bool XModem::dataAvail(int delay)
{
	if (this->byte != -1)
		return true;
	if ((this->byte = this->hardware->recvChar(delay)) != -1)
		return true;
	else
		return false;

}
int XModem::dataRead(int delay)
{
	int b;
	if(this->byte != -1)
	{
		b = this->byte;
		this->byte = -1;
		return b;
	}
	return this->hardware->recvChar(delay);
}
void XModem::dataWrite(char symbol)
{
	this->hardware->sendChar(symbol);
}
bool XModem::receiveFrameNo()
{
	unsigned char num =
		(unsigned char)this->dataRead(XModem::receiveDelay);
	unsigned char invnum =
		(unsigned char)this-> dataRead(XModem::receiveDelay);
	this->repeatedBlock = false;
	//check for repeated block
	if (invnum == (255-num) && num == this->blockNo-1) {
		this->repeatedBlock = true;
		return true;
	}

	if(num != this-> blockNo || invnum != (255-num))
		return false;
	else
		return true;
}
bool XModem::receiveData()
{
	for(int i = 0; i < 128; i++) {
		int byte = this->dataRead(XModem::receiveDelay);
		if(byte != -1)
			this->buffer[i] = (unsigned char)byte;
		else
			return false;
	}
	return true;
}
bool XModem::checkCrc()
{
	unsigned short frame_crc = ((unsigned char)this->
				dataRead(XModem::receiveDelay)) << 8;

	frame_crc |= (unsigned char)this->dataRead(XModem::receiveDelay);
	//now calculate crc on data
	unsigned short crc = this->crc16_ccitt(this->buffer, 128);

	if(frame_crc != crc)
		return false;
	else
		return true;

}
bool XModem::checkChkSum()
{
	unsigned char frame_chksum = (unsigned char)this->
						dataRead(XModem::receiveDelay);
	//calculate chksum
	unsigned char chksum = 0;
	for(int i = 0; i< 128; i++) {
		chksum += this->buffer[i];
	}
	if(frame_chksum == chksum)
		return true;
	else
		return false;
}
bool XModem::sendNack()
{
	this->dataWrite(XModem::NACK);
	this->retries++;
	if(this->retries < XModem::rcvRetryLimit)
		return true;
	else
		return false;

}
bool XModem::receiveFrames(transfer_t transfer)
{
	this->blockNo = 1;
	this->blockNoExt = 1;
	this->retries = 0;
	while (1) {
		char cmd = this->dataRead(2000);
		switch(cmd){
			case XModem::SOH:
				if (!this->receiveFrameNo()) {
					if (this->sendNack())
						break;
					else
						return false;
				}
				if (!this->receiveData()) {
					if (this->sendNack())
						break;
					else
						return false;

				};
				if (transfer == Crc) {
					if (!this->checkCrc()) {
						if (this->sendNack())
							break;
						else
							return false;
					}
				} else {
					if(!this->checkChkSum()) {
						if (this->sendNack())
							break;
						else
							return false;
					}
				}
				//callback
				if(this->hardware != NULL &&
				   this->repeatedBlock == false)
					if(!this->hardware->rDataHandler(this->blockNoExt,
							  this->buffer, 128)) {
						return false;
					}
				//ack
				this->dataWrite(XModem::ACK);
				if(this->repeatedBlock == false)
				{
					this->blockNo++;
					this->blockNoExt++;
				}

				break;
			case XModem::EOT:
				this->dataWrite(XModem::ACK);
				return true;
			case XModem::CAN:
				//wait second CAN
				if(this->dataRead(XModem::receiveDelay) ==
						XModem::CAN) {
					this->dataWrite(XModem::ACK);
					//this->flushInput();
					return false;
				}
				//something wrong
				this->dataWrite(XModem::CAN);
				this->dataWrite(XModem::CAN);
				this->dataWrite(XModem::CAN);
				return false;
			default:
				//something wrong
				this->dataWrite(XModem::CAN);
				this->dataWrite(XModem::CAN);
				this->dataWrite(XModem::CAN);
				return false;
		}

	}
}
void XModem::begin(XModemHardwareInterface* _hardware) {
	hardware = _hardware;
}
void XModem::init()
{
	//set preread byte
	this->byte = -1;
}
bool XModem::receive()
{
	this->init();

	for (int i =0; i <  16; i++)
	{
		this->dataWrite('C');
		if (this->dataAvail(1000))
			return receiveFrames(Crc);

	}
	for (int i =0; i <  16; i++)
	{
		this->dataWrite(XModem::NACK);
		if (this->dataAvail(1000)) {
			return receiveFrames(ChkSum);
		}
	}
	return false;
}
unsigned short XModem::crc16_ccitt(char *buf, int size)
{
	unsigned short crc = 0;
	while (--size >= 0) {
		int i;
		crc ^= (unsigned short) *buf++ << 8;
		for (i = 0; i < 8; i++)
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc <<= 1;
	}
	return crc;
}
unsigned char XModem::generateChkSum(void)
{
	//calculate chksum
	unsigned char chksum = 0;
	for(int i = 0; i< 128; i++) {
		chksum += this->buffer[i];
	}
	return chksum;

}

bool XModem::transmitFrames(transfer_t transfer)
{
	this->blockNo = 1;
	this->blockNoExt = 1;
	// use this only in unit tetsing
	//memset(this->buffer, 'A', 128);
	while(1)
	{
		//get data
		if (this->hardware != NULL)
		{
			if( false ==
			    this->hardware->tDataHandler(this->blockNoExt, this->buffer,
			    128))
			{
				//end of transfer
				this->hardware->sendChar(XModem::EOT);
				//wait ACK
				if (this->dataRead(XModem::receiveDelay) ==
					XModem::ACK)
					return true;
				else
					return false;

			}

		}
		else
		{
			//cancel transfer - send CAN twice
			this->hardware->sendChar(XModem::CAN);
			this->hardware->sendChar(XModem::CAN);
			//wait ACK
			if (this->dataRead(XModem::receiveDelay) ==
				XModem::ACK)
				return true;
			else
				return false;
		}
		//send SOH
		this->hardware->sendChar(XModem::SOH);
		//send frame number
		this->hardware->sendChar(this->blockNo);
		//send inv frame number
		this->hardware->sendChar((unsigned char)(255-(this->blockNo)));
		//send data
		for(int i = 0; i <128; i++)
			this->hardware->sendChar(this->buffer[i]);
		//send checksum or crc
		if (transfer == ChkSum) {
			this->hardware->sendChar(this->generateChkSum());
		} else {
			unsigned short crc;
			crc = this->crc16_ccitt(this->buffer, 128);

			this->hardware->sendChar((unsigned char)(crc >> 8));
			this->hardware->sendChar((unsigned char)(crc));

		}
		//TO DO - wait NACK or CAN or ACK
		int ret = this->dataRead(XModem::receiveDelay);
		switch(ret)
		{
			case XModem::ACK: //data is ok - go to next chunk
				this->blockNo++;
				this->blockNoExt++;
				continue;
			case XModem::NACK: //resend data
				continue;
			case XModem::CAN: //abort transmision
				return false;

		}

	}
	return false;
}
bool XModem::transmit()
{
	int retry = 0;
	int sym;
	this->init();

	//wait for CRC transfer
	while(retry < 32)
	{
		if(this->dataAvail(1000))
		{
			sym = this->dataRead(1); //data is here - no delay
			if(sym == 'C')
				return this->transmitFrames(Crc);
			if(sym == XModem::NACK)
				return this->transmitFrames(ChkSum);
		}
		retry++;
	}
	return false;
}
