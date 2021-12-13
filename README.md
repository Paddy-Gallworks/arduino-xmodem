# Note about Licence #

This repo was forked from user **[mgk](https://github.com/mgk)** 's copy. It was published under GPLv3 and so is this version.

# Modifications Since Fork #

## Hardware Interface Class

The intent was to replace the call back functions with an interface class in order to permit the XModem class to be flexible enough to be a member of another object. The callback functions used to configure the input and out of the library were replaced with a 'callback' interface class called 'XModemHardwareInterface'. The functions declared in the interface are the same as the call back functions. Their implementation can be changes to suit the specific of the current project and then passed all together to the XModem class through the 'setInterface' function.

## Two Data Handlers, Transmit and Receive

The second change was to split the dataHandler function into two specific functions specific to transmit and receive. This gives the programmer complete freedom to add specific steps that only exist for one or the other operations.

# Usage #

## Send and Receive Characters

The code is organized around singe C++ class: XModem. The usage is quite simple - just make instance of XModem and call methods transmit() or receive(). XMomdem doesn't know which function to use in order to send or receive data. These actions need to be declared in a class which implements the XModemHardwareInterface interface class.
```
// Implement the interface
class XModemArduinoInterface : public XModemHardwareInterface {
    ...
};
// Define the behaviour under the interface
int XModemArduinoInterface::recvChar(int msDelay)
{
       ...
}
void XModemArduinoInterface::sendChar(char sym)
{
       ...
}
// Assign the interface to the modem instance
XModemArduinoInterface hardware;
XModem modem;
modem.setInterface(&hardware);

```
In order to utilize Serial library of Arduino the functions recvChar and sendChar can look like this:
```
int XModemArduinoInterface::recvChar(int msDelay)
{
        int cnt = 0;
        while(cnt < msDelay)
        {
	     if(Serial.available() > 0)
                 return Serial.read();
             delay(1);
             cnt++;
        }
        return -1;
}
void XModemArduinoInterface::sendChar(char sym)
{
       Serial.write(sym);
}

```

## Transmit and Receive

The transmit function is quite straightforward, but in receive route there is little complication. XMODEM protocol uses predefined timeouts when reading data from serial connection. So we need routine that must read symbol if it is available or wait for it specified amount of time. If no symbol is received within this period, the function must return -1.
Much better approach is to use serialEvent() handler to point when symbol is available.
Data transmission is accomplished by using XModem's methods receive() or transmit()

```
modem.transmit();
...
modem.receive();
```

## Data Handler Functions

Of course someone should handle actual data. This is done by using two data handler functions which are also members of the XModemHardwareInterface class and can be implemented in a very similar way as the recvChar and sendChar functions shown above:

```
bool XModemArduinoInterface::rDataHandler(unsigned long no, char* data, int size)
{
      ...
}
bool XModemArduinoInterface::tDataHandler(unsigned long no, char* data, int size)
{
      ...
}
```

dataHandler is called in following cases:
  * when XModem is used as receiver i.e. receive() method is called, then dataHandler is called when there is available data. The first parameter holds the sequence number of data chunk received. The first block is with number 1, next is 2 and so on. The data is pointer to a buffer(_char `*`_) with length of 128 holding received data. The size is actual size of receive data. If data reception is successful, the dataHandler must return _true_, otherwise _false_.

  * when XModem is used as transmitter i.e. transmit method is called, then dataHandler is called before sending data. It is responsible for providing data to be send. The first parameter indicate the number of block which is going to be transmitted. Note that currently all block are with fixed size of 128. The _data_ parameter is pointer to buffer (_char `*`_) which dataHandler must fill. Be aware that buffer is only 128 bytes long! The third parameter contains number of requested bytes. Currently it is fixed to 128. End of data transmission is indicated by returning _false_.
