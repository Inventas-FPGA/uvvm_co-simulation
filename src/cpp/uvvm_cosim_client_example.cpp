#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include <jsonrpccxx/client.hpp>
#include <jsonrpccxx/iclientconnector.hpp>
#include <cpphttplibconnector.hpp>
#include "uvvm_cosim_client.hpp"
#include "uvvm_cosim_types.hpp"

void print_received_data(const std::vector<uint8_t>& data)
{
  std::cout << "data = [";
  bool first = true;
  for (auto &b : data) {
    if (!first) {
      std::cout << ",";
    }
    std::cout << std::hex << (int)b;
    first = false;
  }
  std::cout << "]" << std::endl << std::dec;
}

void print_receive_result(const JsonResponse& response, const std::string& vvc_name)
{
  if (!response.success) {
    std::cout << vvc_name << ": Receive failed: " << response.result["error"] << std::endl;
  } else {
    std::cout << vvc_name << ": Got " << response.result["data"].size() << " bytes." << std::endl;
    print_received_data(response.result["data"]);
  }
}

void print_vvc(const VvcInstance& vvc)
{
  std::cout << "Type: " << vvc.vvc_type << ", ";
  std::cout << "Channel: " << vvc.vvc_channel << ", ";
  std::cout << "Instance ID: " << vvc.vvc_instance_id << std::endl;

  std::cout << "Config: ";
  for (auto &cfg : vvc.bfm_cfg) {
    std::cout << cfg.first << "=" << cfg.second << " ";
  }
  std::cout << "listen_enable: "
            << (vvc.listen_enable ? "true" : "false") << std::endl;
}

void print_vvc_list(const JsonResponse& vvc_list_json)
{
  if (vvc_list_json.success && vvc_list_json.result.is_array()) {
    std::cout << "VVC List:" << std::endl;
    std::cout << "-----------------------------------------------------" << std::endl;
    for (auto &vvc_json : vvc_list_json.result) {
      VvcInstance vvc = vvc_json;
      print_vvc(vvc);
      std::cout << std::endl << std::endl;
    }
  } else {
    std::cout << "Failed to get VVC list: " << vvc_list_json.result["error"] << std::endl;
  }
}

// Test connecting, transmitting and receiving some bytes
int main(int argc, char** argv)
{
  using namespace std::chrono_literals;

  CppHttpLibClientConnector http_connector("localhost", 8484);
  UvvmCosimClient client(http_connector);

  std::cout << "Wait a bit...." << std::endl;

  std::this_thread::sleep_for(1.0s);

  std::cout << "Start sim...." << std::endl;

  client.StartSim();

  std::this_thread::sleep_for(0.5s);

  std::cout << "Get VVC list...." << std::endl << std::endl;
  auto vvc_list = client.GetVvcList();
  print_vvc_list(vvc_list);

  std::cout << "Enable cosim listening on AXI-S VVC 1 and UART VVC 1" << std::endl;
  client.SetVvcListenEnable("UART_VVC", 1, true);
  client.SetVvcListenEnable("AXISTREAM_VVC", 1, true);
  std::this_thread::sleep_for(0.5s);

  std::cout << "Get VVC list again...." << std::endl << std::endl;
  vvc_list = client.GetVvcList();
  print_vvc_list(vvc_list);

  std::this_thread::sleep_for(0.5s);

  std::cout << "AXI-Stream: Transmit some data..." << std::endl;

  client.TransmitBytes("AXISTREAM_VVC", 0, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
  client.TransmitBytes("AXISTREAM_VVC", 0, {0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C});
  client.TransmitBytes("AXISTREAM_VVC", 0, {0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12});

  // Assume data has been transmitted/received after 1.0 seconds
  std::this_thread::sleep_for(1.0s);

  std::cout << "AXI-Stream: Request to receive 6 bytes..." << std::endl;
  {
    auto res = client.ReceiveBytes("AXISTREAM_VVC", 1, 6, false);
    print_receive_result(res, "AXI-Stream");
  }

  std::cout << "AXI-Stream: Request to receive 12 bytes..." << std::endl;
  {
    auto res = client.ReceiveBytes("AXISTREAM_VVC", 1, 12, false);
    print_receive_result(res, "AXI-Stream");
  }

  std::cout << "AXI-Stream: Request to receive 10 bytes..." << std::endl;
  {
    auto res = client.ReceiveBytes("AXISTREAM_VVC", 1, 10, false);
    print_receive_result(res, "AXI-Stream");
  }

  std::cout << "Pause sim" << std::endl;
  client.PauseSim();

  std::cout << "UART: Transmit some data..." << std::endl;
  client.TransmitBytes("UART_VVC", 0, {0xCA, 0xFE, 0xAA, 0x12, 0x34, 0x55});

  std::cout << "Resume sim" << std::endl;
  client.StartSim();

  // Assume data has been transmitted/received after 1.0 seconds
  std::this_thread::sleep_for(1.0s);

  std::cout << "UART: Request to receive 5 bytes..." << std::endl;
  {
    auto res = client.ReceiveBytes("UART_VVC", 1, 5, false);
    print_receive_result(res, "UART");
  }

  std::cout << "UART: Request to receive 5 bytes (more than available)..." << std::endl;
  {
    auto res = client.ReceiveBytes("UART_VVC", 1, 5, true);
    print_receive_result(res, "UART");
  }

  std::cout << "UART: Request to receive 5 bytes or whatever is available..." << std::endl;
  {
    auto res = client.ReceiveBytes("UART_VVC", 1, 5, false);
    print_receive_result(res, "UART");
  }

  // Transmit some more data
  client.TransmitBytes("UART_VVC", 0, {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC});
  client.TransmitBytes("UART_VVC", 0, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06});

  // Assume data has been transmitted/received after 1.0 seconds
  std::this_thread::sleep_for(1.0s);

  std::cout << "UART: Request to receive 12 bytes or whatever is available..." << std::endl;
  {
    auto res = client.ReceiveBytes("UART_VVC", 1, 12, false);
    print_receive_result(res, "UART");
  }

  // Transmit some more data
  client.TransmitBytes("UART_VVC", 0, {0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C});
  client.TransmitBytes("UART_VVC", 0, {0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12});

  // Assume data has been transmitted/received after 1.0 seconds
  std::this_thread::sleep_for(1.0s);

  std::cout << "UART: Request to receive 12 bytes or whatever is available..." << std::endl;
  {
    auto res = client.ReceiveBytes("UART_VVC", 1, 12, false);
    print_receive_result(res, "UART");
  }

  std::cout << "Terminating simulation." << std::endl;

  client.TerminateSim();

  std::cout << "DONE." << std::endl;
}
