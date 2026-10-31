#ifndef WORKBENCH_HTTP_SERVER_HPP
#define WORKBENCH_HTTP_SERVER_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <thread>

namespace workbench::server {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code = 200;
    std::string content_type = "application/json";
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    HttpServer(int port = 48123, const std::string& static_root = "src/frontend");
    ~HttpServer();

    void add_route(const std::string& method, const std::string& path, RouteHandler handler);
    bool start();
    void stop();
    bool is_running() const { return running.load(); }
    int get_port() const { return server_port; }

private:
    int server_port;
    std::string static_directory;
    std::atomic<bool> running{false};
    std::thread listener_thread;
    std::unordered_map<std::string, RouteHandler> routes;

    void register_default_routes();
    void run_listener();
    HttpResponse handle_request(const HttpRequest& req);
    HttpResponse serve_static_file(const std::string& relative_path);
};

} // namespace workbench::server

#endif // WORKBENCH_HTTP_SERVER_HPP
