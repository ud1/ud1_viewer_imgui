#include "TcpServer.hpp"

#include <memory>
#include "Settings.hpp"
#include <boost/endian/conversion.hpp>
#include <filesystem>
#include <boost/algorithm/string/predicate.hpp>

namespace fs = std::filesystem;

using boost::asio::ip::tcp;

TcpServer::~TcpServer()
{
    settings.tcpServer = nullptr;
    stop();
}

void TcpServer::start()
{
    try
    {
        boost::asio::ip::tcp::endpoint endpoint(tcp::v4(), settings.port);
        asioTcpServer = std::make_unique<AsioTcpServer>(io_service, endpoint, *this);

        thread = std::thread([this](){
            LOG("RUN");
            io_service.run();
            LOG("RUN FINISH");
        });
    }
    catch (std::exception& e)
    {
        std::cerr << "Open TCP server error: " << e.what() << "\n";
    }
}

void TcpServer::stop()
{
    LOG("STOP");
    io_service.stop();

    if (thread.joinable())
        thread.join();
}

void TcpServer::newConnection()
{
    if (outputFile.is_open())
        outputFile.close();

    Settings settings;
    if (settings.saveToFile && !settings.outputDir.empty())
    {
        fs::create_directories(settings.outputDir);

        std::string filePath = settings.outputDir;

        if (!boost::algorithm::ends_with(filePath, "/"))
            filePath += "/";

        filePath += std::to_string(time(0));
        filePath += ".vbin";

        outputFile.open(filePath, std::ofstream::out | std::ofstream::binary);
    }

    stateHolder.onNewConnection();
}

void TcpServer::onNewData(const std::string &data)
{
    std::istringstream iss(data);

    Obj obj = readObj(iss);
    if (iss)
    {
        stateHolder.process(obj);
    }

    if (outputFile.is_open())
    {
        uint32_t size = boost::endian::native_to_big(data.size());
        outputFile.write(reinterpret_cast<const char *>(&size), sizeof(size));
        outputFile.write(&data[0], data.size());
        LOG("WRITTEN " << data.size());
    }
}

void TcpServer::disconnected()
{
    if (outputFile.is_open())
        outputFile.close();

    LOG("DISCONNECTED");
}

void TcpServer::settingsChanged()
{
    if (outputFile.is_open())
        outputFile.close();

    stop();
    start();
}

void TcpServer::sendKeyEvent(const Obj &obj)
{
    std::ostringstream oss;
    writeObj(oss, obj);

    std::string str = oss.str();

    if (asioTcpServer)
        asioTcpServer->sendCmd(str);
}

TcpServer::TcpServer(Settings &settings, StateHolder &stateHolder) : settings(settings), stateHolder(stateHolder) {
    settings.tcpServer = this;
}

void TcpConnection::doReadSize()
{
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(&msgSize, sizeof(msgSize)),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec)
                                {
                                    doReadBody(boost::endian::big_to_native(self->msgSize));
                                }
                                else
                                {
                                    asioTcpServer->processDisconnect();
                                }
                            });
}

void TcpConnection::doReadBody(uint32_t size)
{
    data.resize(size, ' ');
    auto self(shared_from_this());

    boost::asio::async_read(socket_,
                            boost::asio::buffer(data.data(), size),
                            [this, self, size](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec)
                                {
                                    asioTcpServer->processData(data);
                                    doReadSize();
                                }
                                else
                                {
                                    asioTcpServer->processDisconnect();
                                }
                            });
}

void TcpConnection::sendCmd(const std::string &cmd)
{
    uint32_t size = cmd.size();

    size = boost::endian::native_to_big(size);

    try
    {
        boost::asio::write(socket_, boost::asio::buffer(&size, sizeof(size)));
        boost::asio::write(socket_, boost::asio::buffer(cmd, cmd.size()));
    }
    catch (std::exception& e)
    {
        asioTcpServer->processDisconnect();
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

void AsioTcpServer::do_accept()
{
    LOG("ACCEPT");
    acceptor_.async_accept(socket_,
                           [this](boost::system::error_code ec)
                           {
                               if (!ec)
                               {
                                   std::lock_guard<std::mutex> lock(mutex);
                                   connection = std::make_shared<TcpConnection>(std::move(socket_), this);
                                   tcpServer.newConnection();
                                   connection->start();
                               }

                               do_accept();
                           });
}

void AsioTcpServer::processData(const std::string &data)
{
    tcpServer.onNewData(data);
}
