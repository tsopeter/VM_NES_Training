//
//  Serial.hpp
//  ONN-Model
//
//  Created by Peter Tso on 11/8/24.
//

#ifndef Serial_hpp
#define Serial_hpp

#include <iostream>
#include <termios.h>    // POSIX terminal control definitions

class Serial {
public:
    /**
     *  @brief Serial is a class wrapper that handles all communication over a serial port. Serial ports on macOS can be found in /dev/. In the Terminal, use `ls -al /dev/tty.usbmodem*` to find all USB serial devices. Use this as the port name. You also must check to make sure that baudrates match up, but it is typically 9600 bps or 115200 bps.
     *
     */
    Serial(const std::string, int baud_rate = 115200);
    
    /**
     *  @brief Serial is a class wrapper that handles all communication over a serial port. Serial ports on macOS can be found in /dev/. In the Terminal, use `ls -al /dev/tty.usbmodem*` to find all USB serial devices. Use this as the port name. You also must check to make sure that baudrates match up, but it is typically 9600 bps or 115200 bps.
     *
     */
    Serial(const char [], int baud_rate = 115200);

    /**
     *  @brief Serial is a class wrapper that handles all communication over a serial port. Serial ports on macOS can be found in /dev/. In the Terminal, use `ls -al /dev/tty.usbmodem*` to find all USB serial devices. Use this as the port name. You also must check to make sure that baudrates match up, but it is typically 9600 bps or 115200 bps.
     *
     */
    Serial ();
    
    /**
     *  @brief Handles closing up Serial.
     */
    ~Serial();
    
    /**
     *  @brief Handles opening the serial port. Make sure that this is used before calling Send and Signal. Calling the `Serial` constructor is typically not enough. You MUST call `Open()` to open up a serial connection.
     */
    void Open();
    
    /**
     *  @brief Handles closing up the serial port. If you want to open it up again, call `Open()`. The `Serial` destructor `~Serial()` also calls close so explicitly calling this function, for most cases, is completely optional.
     */
    void Close();
    
    /**
     *  @brief Sends some data to the serial port. This is  the default method of sending data to the serial port.
     */
    int Send(const std::string &);
    
    /**
     *  @brief Calls
     */
    int Send(const char *);
    void Signal();


    /* Setters */
    void set_port_name (const std::string&);
    void set_baud_rate (int);
private:
    std::string port_name = "";
    int baud_rate;
    int port;
};

#endif /* Serial_hpp */
