#ifndef DIFFDRIVE_ESP32__ESP32_COMMS_HPP
#define DIFFDRIVE_ESP32__ESP32_COMMS_HPP

// #include <cstring>
#include <sstream>
// #include <cstdlib>
#include <libserial/SerialPort.h>
#include <iostream>
#include<string>
#include<algorithm>


LibSerial::BaudRate convert_baud_rate(int baud_rate)
{
  // Just handle some common baud rates
  switch (baud_rate)
  {
    case 1200: return LibSerial::BaudRate::BAUD_1200;
    case 1800: return LibSerial::BaudRate::BAUD_1800;
    case 2400: return LibSerial::BaudRate::BAUD_2400;
    case 4800: return LibSerial::BaudRate::BAUD_4800;
    case 9600: return LibSerial::BaudRate::BAUD_9600;
    case 19200: return LibSerial::BaudRate::BAUD_19200;
    case 38400: return LibSerial::BaudRate::BAUD_38400;
    case 57600: return LibSerial::BaudRate::BAUD_57600;
    case 115200: return LibSerial::BaudRate::BAUD_115200;
    case 230400: return LibSerial::BaudRate::BAUD_230400;
    default:
      std::cout << "Error! Baud rate " << baud_rate << " not supported! Default to 57600" << std::endl;
      return LibSerial::BaudRate::BAUD_57600;
  }
}

class ArduinoComms
{

public:

  ArduinoComms() = default;

  void connect(const std::string &serial_device, int32_t baud_rate, int32_t timeout_ms)
  {  
    timeout_ms_ = timeout_ms;
    serial_conn_.Open(serial_device);
    serial_conn_.SetBaudRate(convert_baud_rate(baud_rate));
  }

  void disconnect()
  {
    serial_conn_.Close();
  }

  bool connected() const
  {
    return serial_conn_.IsOpen();
  }


  std::string send_msg(const std::string &msg_to_send, bool print_output = true)
{
    //serial_conn_.FlushIOBuffers(); // Just in case
    serial_conn_.Write(msg_to_send);

    std::string response;

     try
    {
        serial_conn_.ReadLine(response, '\n', timeout_ms_);
        std::cout<<"Received response: "<<response<<std::endl;
    }
    catch (const LibSerial::ReadTimeout&)
    {

        std::cerr << "ReadByte() timed out, using partial data" << std::endl;
        std::cerr<<"Partial data is: "<<response<<std::endl;
    }
     

    if (print_output)
    {
      std::string display_msg_to_send = msg_to_send;
      display_msg_to_send.erase(std::remove(display_msg_to_send.begin(), display_msg_to_send.end(), '\r'), display_msg_to_send.end());
      std::cout<<"Sent: "<<display_msg_to_send<<"  Recv: " << response<< std::endl;
    }

    return response;
}



  void send_empty_msg()
  {
    std::string response = send_msg("\r");
  }

  void read_encoder_values(int64_t &val_1, int64_t &val_2)
  {
    std::string response = send_msg("e\r");

    std::string delimiter = " ";
    size_t del_pos = response.find(delimiter);

    if(del_pos == std::string::npos)
    {
      std::cerr << "Invalid encoder response : " << response << std::endl;
      return;
    }

    try
    {  
      std::string token_1 = response.substr(0, del_pos);

      if(token_1.empty())
      {
        std::cerr << "Invalid encoder response, first token is empty: " << response << std::endl;
        return;
      }

      if (response.length() <= (del_pos + delimiter.length())) 
      {
        std::cerr << "Invalid encoder response, second token is missing: " << response << std::endl;
        return;
      }

      std::string token_2 = response.substr(del_pos + delimiter.length());

      val_1 = std::atoll(token_1.c_str());
      val_2 = std::atoll(token_2.c_str());
      std::cout<<"Encoder Counts: "<<val_1<<" "<<val_2<<std::endl;

    }

    catch (const std::exception& e) 
    {
      // Manual throttle: only print once every 50 calls (~once per second at 50Hz)
      static int error_count = 0;
      if (error_count++ % 50 == 0) 
      {
          std::cerr << "[Comms Error] Parsing failed: " << e.what() << " | Raw Response: " << response << std::endl;
      }
      return; 
    }
    catch (...) 
    {
      std::cerr << "[Comms Error] Unknown critical failure in parsing." << std::endl;
      return;
    }
  }

  void set_motor_values(float val_1, float val_2)
  {
    std::stringstream ss;
    ss << "m " << val_1 << " " << val_2 << "\r";
    send_msg(ss.str());
  }

  void set_pid_values(float k_p, float k_d, float k_i, float k_o)
  {
    std::stringstream ss;
    ss << "u " << k_p << ":" << k_d << ":" << k_i << ":" << k_o << "\r";
    send_msg(ss.str());
  }

private:
    LibSerial::SerialPort serial_conn_;
    int timeout_ms_;
};

#endif // DIFFDRIVE_ESP32__ESP32_COMMS_HPP