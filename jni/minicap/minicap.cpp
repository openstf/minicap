#include <iostream>
#include <stdexcept>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/server.hpp>

#include "util/capster.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

/* on_open insert connection_hdl into channel
 * on_close remove connection_hdl from channel
 * on_message queue send to all channels
 */

enum action_type {
  SUBSCRIBE,
  UNSUBSCRIBE,
  MESSAGE,
};

struct action {
  action(action_type t, connection_hdl h) : type(t), hdl(h) {}
  action(action_type t, connection_hdl h, server::message_ptr m)
    : type(t), hdl(h), msg(m) {}

  action_type type;
  websocketpp::connection_hdl hdl;
  server::message_ptr msg;
};

class broadcast_server {
public:
  broadcast_server() : m_capster(0) {
    // Don't log
    m_server.set_access_channels(websocketpp::log::alevel::none);

    // Initialize Asio Transport
    m_server.init_asio();
    m_server.set_reuse_addr(true);

    // Register handler callbacks
    m_server.set_socket_init_handler(bind(&broadcast_server::on_socket_init, this, ::_1, ::_2));
    m_server.set_open_handler(bind(&broadcast_server::on_open, this, ::_1));
    m_server.set_close_handler(bind(&broadcast_server::on_close, this, ::_1));
    m_server.set_message_handler(bind(&broadcast_server::on_message, this, ::_1, ::_2));
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

  void on_open(connection_hdl hdl) {
    unique_lock<mutex> lock(m_action_lock);
    m_actions.push(action(SUBSCRIBE, hdl));
    lock.unlock();
    m_action_cond.notify_one();
  }

  void on_close(connection_hdl hdl) {
    unique_lock<mutex> lock(m_action_lock);
    m_actions.push(action(UNSUBSCRIBE, hdl));
    lock.unlock();
    m_action_cond.notify_one();
  }

  void on_message(connection_hdl hdl, server::message_ptr msg) {
    // queue message up for sending by processing thread
    unique_lock<mutex> lock(m_action_lock);
    m_actions.push(action(MESSAGE, hdl, msg));
    lock.unlock();
    m_action_cond.notify_one();
  }

  void process_messages() {
    while(1) {
      unique_lock<mutex> lock(m_action_lock);

      while(m_actions.empty()) {
        m_action_cond.wait(lock);
      }

      action a = m_actions.front();
      m_actions.pop();

      lock.unlock();

      if (a.type == SUBSCRIBE) {
        unique_lock<mutex> con_lock(m_connection_lock);
        m_connections.insert(a.hdl);
      } else if (a.type == UNSUBSCRIBE) {
        unique_lock<mutex> con_lock(m_connection_lock);
        m_connections.erase(a.hdl);
      } else if (a.type == MESSAGE) {
        unique_lock<mutex> con_lock(m_connection_lock);

        m_capster.capture(0, 0);

        con_list::iterator it;
        try {
          for (it = m_connections.begin(); it != m_connections.end(); ++it) {
            m_server.send(*it, m_capster.get_data(), m_capster.get_size(),
              websocketpp::frame::opcode::BINARY);
          }
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
      } else {
        // undefined.
      }
    }
  }

private:
  typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;

  server m_server;
  con_list m_connections;
  std::queue<action> m_actions;

  mutex m_action_lock;
  mutex m_connection_lock;
  condition_variable m_action_cond;

  capster m_capster;
};

int main() {
  try {
  broadcast_server server_instance;

  // Start a thread to run the processing loop
  thread t(bind(&broadcast_server::process_messages, &server_instance));

  // Run the asio loop with the main thread
  server_instance.run(9002);

  t.join();

  } catch (std::exception & e) {
    std::cout << e.what() << std::endl;
  }
}
