#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <asio.hpp>
#include <nlohmann/json.hpp>

namespace streaming {
using Json = nlohmann::json;
using Tcp = asio::ip::tcp;

// All network operations belong to one io_context thread. Cancellation does not
// depend on another message arriving. Robot work lives on a separate thread.
class Network {
public:
    Network(asio::io_context& io, const Tcp::endpoint& endpoint,
            std::function<Json(const Json&)> request, std::function<void()> disconnect)
        : acceptor_(io, endpoint), request_(std::move(request)), disconnect_(std::move(disconnect)) {
        accept();
    }

    unsigned short port() const { return acceptor_.local_endpoint().port(); }

    void shutdown() {
        std::error_code ignored;
        acceptor_.close(ignored);
        drop(client_);
    }

private:
    struct Client {
        explicit Client(Tcp::socket socket) : socket(std::move(socket)) {}
        Tcp::socket socket;
        asio::streambuf input{4096};
        std::string output;
    };

    void accept() {
        acceptor_.async_accept([this](std::error_code error, Tcp::socket socket) {
            if (!acceptor_.is_open()) return;
            if (!error && !client_) {
                socket.set_option(Tcp::no_delay(true), error);
                if (!error) {
                    client_ = std::make_shared<Client>(std::move(socket));
                    read(client_);
                }
            }
            // An additional connection is closed, never allowed to replace the owner.
            accept();
        });
    }

    void drop(std::shared_ptr<Client> client) {
        if (!client || client != client_) return;
        std::error_code ignored;
        client->socket.close(ignored);
        client_.reset();
        disconnect_();
    }

    void read(std::shared_ptr<Client> client) {
        asio::async_read_until(client->socket, client->input, '\n',
            [this, client](std::error_code error, std::size_t) {
                if (client != client_) return;
                if (error) { drop(client); return; }
                std::string line;
                std::istream stream(&client->input);
                std::getline(stream, line);
                Json response;
                try {
                    response = request_(Json::parse(line));
                    response["ok"] = true;
                } catch (const std::exception& exc) {
                    response = {{"ok", false}, {"error", exc.what()}};
                }
                client->output = response.dump() + "\n";
                asio::async_write(client->socket, asio::buffer(client->output),
                    [this, client](std::error_code error, std::size_t) {
                        if (client != client_) return;
                        if (error) drop(client);
                        else read(client);
                    });
            });
    }

    Tcp::acceptor acceptor_;
    std::shared_ptr<Client> client_;
    std::function<Json(const Json&)> request_;
    std::function<void()> disconnect_;
};
} // namespace streaming
