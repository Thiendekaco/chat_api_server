#include <unordered_map>
#include <string>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include "ThreadPool.h"


using json = nlohmann::json;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpServer {
public:
    TcpServer(boost::asio::io_context& io_context, short port, ThreadPool& threadPool);
    void doAccept();
    void handleClient(std::shared_ptr<tcp::socket> socket);
    void processMessage(const std::string& message, std::shared_ptr<tcp::socket> socket);
    void handleConnect(const json& message, std::shared_ptr<tcp::socket> socket);
    void handleDisconnect(const json& message, std::shared_ptr<tcp::socket> socket);
    void handleMessage(const json& message, std::shared_ptr<tcp::socket> socket);
    void handleTyping(const json& message, std::shared_ptr<tcp::socket> socket);
    void handleStopTyping(const json& message, std::shared_ptr<tcp::socket> socket);
    void handleUserStatus(const json& message, std::shared_ptr<tcp::socket> socket);
    void handleMessageReceipt(const json& message, std::shared_ptr<tcp::socket> socket);
    bool isTokenValid(const std::string& token, std::string& email);

    // New methods to send messages
    void sendMessageToClient(const std::string& clientId, const std::string& message);
    void sendMessageToMultipleClients(const std::vector<std::string>& clientIds, const std::string& message);
    void broadcastMessage(const std::string& message);

private:
    tcp::acceptor acceptor_;
    std::shared_ptr<tcp::socket> socket_;
    ThreadPool& threadPool_;

    // Store connected clients

    std::unordered_map<std::string, std::vector<std::shared_ptr<tcp::socket>>> clients_;
    std::mutex clientsMutex_;
};
