#ifndef UD1_VIEWER_IMGUI_TCPSERVER_HPP
#define UD1_VIEWER_IMGUI_TCPSERVER_HPP

#include <boost/asio.hpp>
#include <thread>
#include "format.hpp"
#include "Settings.hpp"
#include "StateHolder.hpp"

class AsioTcpServer;
class TcpServer;

class TcpConnection: public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(boost::asio::ip::tcp::socket socket, AsioTcpServer *asioTcpServer)
            : socket_(std::move(socket)), asioTcpServer(asioTcpServer)
    {
    }

    ~TcpConnection()
    {
        LOG("~TcpConnection");
    }

    void start()
    {
        doReadSize();
    }

    void doReadSize();

    void doReadBody(uint32_t size);

    void sendCmd(const std::string &cmd);

    boost::asio::ip::tcp::socket socket_;
    uint32_t msgSize;
    std::string data;

    AsioTcpServer *asioTcpServer;
};

class AsioTcpServer
{
public:
    AsioTcpServer(boost::asio::io_service &io_service, const boost::asio::ip::tcp::endpoint &endpoint,
                  TcpServer &tcpServer)
            : acceptor_(io_service, endpoint),
              socket_(io_service), tcpServer(tcpServer) {
        do_accept();
    }

    ~AsioTcpServer()
    {
        LOG("~AsioTcpServer");
    }

    void processData(const std::string &data);

    void processDisconnect()
    {
        connection.reset();
    }

    void sendCmd(const std::string &cmd)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (connection)
        {
            connection->sendCmd(cmd);
        }
    }

private:
    TcpServer &tcpServer;
    void do_accept();

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;

    std::mutex mutex;
    std::shared_ptr<TcpConnection> connection;
};


class TcpServer {
public:
    TcpServer(Settings &settings, StateHolder &stateHolder);

    ~TcpServer();

    void start();
    void stop();
    void newConnection();
    void disconnected();
    void settingsChanged();
    void sendKeyEvent(const Obj &obj);
    void onNewData(const std::string &data);

private:
    Settings &settings;
    StateHolder &stateHolder;

    std::ofstream outputFile;
    boost::asio::io_service io_service;
    std::thread thread;

    std::unique_ptr<AsioTcpServer> asioTcpServer;
};


#endif //UD1_VIEWER_IMGUI_TCPSERVER_HPP
