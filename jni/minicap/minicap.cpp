#include <iostream>
#include <stdexcept>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "util/capster.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

class capster_server {
public:
  capster_server() : m_capster(0) {
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
    m_capster.capture(0, 0);

    try {
      m_server.send(hdl, m_capster.get_data(), m_capster.get_size(),
        websocketpp::frame::opcode::BINARY);
    }
    catch (websocketpp::exception& e) {
      if (e.code() == websocketpp::error::bad_connection) {
        // The connection doesn't exist anymore. We should get an
        // UNSUBSCRIBE message later, so let's just ignore the
        // exception here.
      }
      else {
        std::cout << e.what() << std::endl;
      }
    }
  }

private:
  server m_server;
  capster m_capster;
};

int main() {
  try {
    capster_server server_instance;

    // Run the asio loop with the main thread
    server_instance.run(9002);

  } catch (std::exception & e) {
    std::cout << e.what() << std::endl;
  }
}
