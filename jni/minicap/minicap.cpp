#include <getopt.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <json_spirit.h>

#ifndef JSON_SPIRIT_MVALUE_ENABLED
#error Please define JSON_SPIRIT_MVALUE_ENABLED for the mValue type to be enabled
#endif

#include "util/capster.hpp"

#define DEFAULT_DISPLAY_ID 0
#define DEFAULT_WEBSOCKET_PORT 9002

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

static void usage(const char* pname)
{
  fprintf(stderr,
    "Usage: %s [-h] [-d <display>] [-i] [-p <port>]\n"
    "  -d <display>:  Display ID. (%d)\n"
    "  -p <port>:     WebSocket server port. (%d)\n"
    "  -i:            Get display information in JSON format.\n"
    "  -h:            Show help.\n",
    pname, DEFAULT_DISPLAY_ID, DEFAULT_WEBSOCKET_PORT
  );
}

const json_spirit::mValue& find_value(const json_spirit::mObject& obj, const std::string& name)
{
    json_spirit::mObject::const_iterator i = obj.find(name);

    assert(i != obj.end());
    assert(i->first == name);

    return i->second;
}

class capster_server {
public:
  capster_server(uint32_t display_id) : m_capster(display_id) {
    // Don't log
    m_server.set_access_channels(websocketpp::log::alevel::none);

    // Initialize Asio Transport
    m_server.init_asio();
    m_server.set_reuse_addr(true);

    // Register handler callbacks
    m_server.set_socket_init_handler(bind(&capster_server::on_socket_init, this, ::_1, ::_2));
    m_server.set_message_handler(bind(&capster_server::on_message, this, ::_1, ::_2));
  }

  void run(uint16_t port) {
    // listen on specified port
    m_server.listen(port);

    // Start the server accept loop
    m_server.start_accept();

    // Start the ASIO io_service run loop
    try {
      m_server.run();
    } catch (const std::exception & e) {
      std::cout << e.what() << std::endl;
    } catch (websocketpp::lib::error_code e) {
      std::cout << e.message() << std::endl;
    } catch (...) {
      std::cout << "other exception" << std::endl;
    }
  }

  void on_socket_init(connection_hdl, boost::asio::ip::tcp::socket &s) {
    boost::asio::ip::tcp::no_delay option(true);
    s.set_option(option);
  }

  void on_message(connection_hdl hdl, server::message_ptr msg) {
    std::istringstream in(msg->get_payload());

    json_spirit::mValue value;

    read(in, value);

    const json_spirit::mObject& root = value.get_obj();

    std::string op = find_value(root, "op").get_str();

    if (op.compare("jpeg") == 0) {
      unsigned int width, height;

      width = find_value(root, "w").get_int();
      height = find_value(root, "h").get_int();

      m_capster.capture(width, height);

      try {
        m_server.send(hdl, m_capster.get_data(), m_capster.get_size(),
          websocketpp::frame::opcode::BINARY);
      }
      catch (websocketpp::exception& e) {
        if (e.code() == websocketpp::error::bad_connection) {
          // The connection doesn't exist anymore.
        }
        else {
          std::cout << e.what() << std::endl;
        }
      }
    }
  }

private:
  server m_server;
  capster m_capster;
};

int main(int argc, char* argv[]) {
  const char* pname = argv[0];
  uint32_t display_id = DEFAULT_DISPLAY_ID;
  uint16_t port = DEFAULT_WEBSOCKET_PORT;
  bool show_info = false;

  int opt;
  while ((opt = getopt(argc, argv, "d:p:ih")) != -1) {
    switch (opt) {
      case 'd':
        display_id = atoi(optarg);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'i':
        show_info = true;
        break;
      case '?':
        usage(pname);
        return EXIT_FAILURE;
      case 'h':
        usage(pname);
        return EXIT_SUCCESS;
    }
  }

  if (show_info) {
    try {
      capster::display_info info = capster::get_display_info(display_id);

      json_spirit::Object json;
      json.push_back(json_spirit::Pair("id", static_cast<int>(display_id)));
      json.push_back(json_spirit::Pair("width", static_cast<int>(info.width)));
      json.push_back(json_spirit::Pair("height", static_cast<int>(info.height)));
      json.push_back(json_spirit::Pair("xdpi", static_cast<double>(info.xdpi)));
      json.push_back(json_spirit::Pair("ydpi", static_cast<double>(info.ydpi)));
      json.push_back(json_spirit::Pair("size", static_cast<double>(info.size)));

      std::cout << json_spirit::write(json,
        json_spirit::remove_trailing_zeros | json_spirit::pretty_print);

      return EXIT_SUCCESS;
    }
    catch (std::exception & e) {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  }

  try {
    capster_server server_instance(display_id);

    // Run the asio loop with the main thread
    server_instance.run(port);

    return EXIT_SUCCESS;
  }
  catch (std::exception & e) {
    std::cout << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
