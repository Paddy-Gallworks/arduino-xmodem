#ifndef XMODEM_H
	#define XMODEM_H

	typedef enum {
		Crc,
		ChkSum
	} transfer_t;

	class XModemHardwareInterface {
		virtual int recvChar(int) = 0;
		virtual void sendChar(char) = 0;
		virtual bool rDataHandler(unsigned long , char *, int ) = 0;
		virtual bool tDataHandler(unsigned long , char *, int ) = 0;
	};
	class XModem {
		private:
			//delay when receive bytes in frame - 7 secs
			static const int receiveDelay;
			//retry limit when receiving
			static const int rcvRetryLimit;
			//holds readed byte (due to dataAvail())
			int byte;
			//expected block number
			unsigned char blockNo;
			//extended block number, send to dataHandler()
			unsigned long blockNoExt;
			//retry counter for NACK
			int retries;
			//buffer
			char buffer[128];
			//repeated block flag
			bool repeatedBlock;
			// Declare Call Back Object
			XModemHardwareInterface* hardware;
			unsigned short crc16_ccitt(char *buf, int size);
			bool dataAvail(int delay);
			int dataRead(int delay);
			void dataWrite(char symbol);
			bool receiveFrameNo(void);
			bool receiveData(void);
			bool checkCrc(void);
			bool checkChkSum(void);
			bool receiveFrames(transfer_t transfer);
			bool sendNack(void);

			void init(void);
			bool transmitFrames(transfer_t);
			unsigned char generateChkSum(void);

		public:
			static const unsigned char NACK;
			static const unsigned char ACK;
			static const unsigned char SOH;
			static const unsigned char EOT;
			static const unsigned char CAN;

			XModem(); //Blank!
			void begin(XModemHardwareInterface*);
			bool receive();
			bool transmit();



	};
#endif //XMODEM_H
