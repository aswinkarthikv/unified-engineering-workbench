#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "http_server.hpp"
#include "fea_solver.hpp"
#include "cad_ansys_parser.hpp"
#include "multicore_engine.hpp"
#include "embedded_mocks.hpp"
#include "backprop_engine.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace workbench::server {

static inline bool has_suffix(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string get_mime_type(const std::string& path) {
    if (has_suffix(path, ".html") || has_suffix(path, ".htm")) return "text/html; charset=utf-8";
    if (has_suffix(path, ".css")) return "text/css; charset=utf-8";
    if (has_suffix(path, ".js")) return "application/javascript; charset=utf-8";
    if (has_suffix(path, ".json")) return "application/json; charset=utf-8";
    if (has_suffix(path, ".png")) return "image/png";
    if (has_suffix(path, ".jpg") || has_suffix(path, ".jpeg")) return "image/jpeg";
    if (has_suffix(path, ".svg")) return "image/svg+xml";
    if (has_suffix(path, ".ico")) return "image/x-icon";
    if (has_suffix(path, ".csv")) return "text/csv; charset=utf-8";
    if (has_suffix(path, ".stl")) return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

HttpServer::HttpServer(int port, const std::string& static_root) 
    : server_port(port), static_directory(static_root) 
{
    register_default_routes();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::add_route(const std::string& method, const std::string& path, RouteHandler handler) {
    routes[method + " " + path] = handler;
}

void HttpServer::register_default_routes() {
    // Status / Hardware info
    add_route("GET", "/api/status", [](const HttpRequest&) -> HttpResponse {
        auto hw = multicore::MulticoreEngine::detect_hardware();
        std::ostringstream ss;
        ss << "{\"status\":\"healthy\",\"server\":\"Unified Engineering Workbench C++17 Core\",";
        ss << "\"cpu\":\"" << hw.cpu_brand << "\",\"logical_cores\":" << hw.logical_cores 
           << ",\"physical_cores\":" << hw.physical_cores << ",\"ram_mb\":" << hw.total_ram_mb << "}";
        return {200, "application/json", ss.str()};
    });

    // FEA solve
    add_route("POST", "/api/fea/solve", [](const HttpRequest& req) -> HttpResponse {
        fea::ThermalRodParams p;
        // Parse simple JSON or query params
        auto extract_val = [&](const std::string& key, double def) -> double {
            size_t pos = req.body.find("\"" + key + "\":");
            if (pos == std::string::npos) return def;
            size_t start = pos + key.size() + 3;
            size_t end = req.body.find_first_of(",}", start);
            try { return std::stod(req.body.substr(start, end - start)); } catch (...) { return def; }
        };

        p.length = extract_val("length", 1.0);
        p.area = extract_val("area", 1.0e-4);
        p.thermal_conductivity = extract_val("thermal_conductivity", 10.0);
        p.heat_generation = extract_val("heat_generation", 10000.0);
        p.t_start = extract_val("t_start", 100.0);
        p.t_end = extract_val("t_end", 20.0);
        p.num_elements = static_cast<int>(extract_val("num_elements", 10.0));

        try {
            auto sol = fea::ThermalRodSolver::solve(p);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6);
            ss << "{\"success\":true,\"num_elements\":" << sol.params.num_elements << ",";
            ss << "\"max_midpoint_error\":" << sol.max_midpoint_error << ",";
            ss << "\"l2_error\":" << sol.l2_error << ",";
            ss << "\"theoretical_midpoint_error\":" << (p.heat_generation * (p.length/p.num_elements)*(p.length/p.num_elements) / (8.0 * p.thermal_conductivity)) << ",";
            
            ss << "\"x\":[";
            for (size_t i = 0; i < sol.x_coords.size(); ++i) {
                if (i > 0) ss << ",";
                ss << sol.x_coords[i];
            }
            ss << "],\"t_fe\":[";
            for (size_t i = 0; i < sol.t_fe_nodes.size(); ++i) {
                if (i > 0) ss << ",";
                ss << sol.t_fe_nodes[i];
            }
            ss << "],\"t_exact\":[";
            for (size_t i = 0; i < sol.t_exact_nodes.size(); ++i) {
                if (i > 0) ss << ",";
                ss << sol.t_exact_nodes[i];
            }
            ss << "],\"midpoints\":[";
            for (size_t i = 0; i < sol.midpoints.size(); ++i) {
                if (i > 0) ss << ",";
                const auto& m = sol.midpoints[i];
                ss << "{\"elem\":" << m.element_index << ",\"x\":" << m.x_mid << ",\"t_fe\":" << m.t_fe_interp 
                   << ",\"t_exact\":" << m.t_exact << ",\"error\":" << m.abs_error << "}";
            }
            ss << "]}";
            return {200, "application/json", ss.str()};
        } catch (const std::exception& e) {
            return {400, "application/json", std::string("{\"success\":false,\"error\":\"") + e.what() + "\"}"};
        }
    });

    // FEA convergence study for N = 10, 20, 40, 80
    add_route("GET", "/api/fea/convergence", [](const HttpRequest&) -> HttpResponse {
        auto cases = fea::ThermalRodSolver::run_convergence_study({10, 20, 40, 80});
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        ss << "{\"cases\":[";
        for (size_t i = 0; i < cases.size(); ++i) {
            if (i > 0) ss << ",";
            const auto& c = cases[i];
            ss << "{\"N\":" << c.num_elements << ",\"h\":" << c.element_size_h 
               << ",\"max_error\":" << c.max_midpoint_error << ",\"th_error\":" << c.theoretical_error 
               << ",\"order\":" << c.observed_order << "}";
        }
        ss << "]}";
        return {200, "application/json", ss.str()};
    });

    // Sample ASCII STL
    add_route("GET", "/api/cad/stl/sample", [](const HttpRequest&) -> HttpResponse {
        parsers::StlModel model;
        std::string path = "data/cad/bracket_ascii.stl";
        auto res = parsers::StlAsciiParser::parse_file(path, model);
        if (!res.success) {
            return {400, "application/json", "{\"success\":false,\"error\":\"" + res.error_message + "\",\"action\":\"" + res.actionable_fix + "\"}"};
        }
        return {200, "application/json", parsers::StlAsciiParser::to_json(model)};
    });

    // Parse uploaded STL content
    add_route("POST", "/api/cad/stl/parse", [](const HttpRequest& req) -> HttpResponse {
        parsers::StlModel model;
        auto res = parsers::StlAsciiParser::parse_string(req.body, model);
        if (!res.success) {
            return {400, "application/json", "{\"success\":false,\"error\":\"" + res.error_message + "\",\"action\":\"" + res.actionable_fix + "\"}"};
        }
        return {200, "application/json", parsers::StlAsciiParser::to_json(model)};
    });

    // Sample ANSYS CSV
    add_route("GET", "/api/ansys/sample", [](const HttpRequest&) -> HttpResponse {
        parsers::AnsysResults results;
        std::string path = "data/ansys/synthetic_rod_ansys.csv";
        auto res = parsers::AnsysCsvParser::parse_file(path, results);
        if (!res.success) {
            return {400, "application/json", "{\"success\":false,\"error\":\"" + res.error_message + "\",\"action\":\"" + res.actionable_fix + "\"}"};
        }
        return {200, "application/json", parsers::AnsysCsvParser::to_json(results)};
    });

    // Parse uploaded ANSYS CSV content
    add_route("POST", "/api/ansys/parse", [](const HttpRequest& req) -> HttpResponse {
        parsers::AnsysResults results;
        auto res = parsers::AnsysCsvParser::parse_string(req.body, results);
        if (!res.success) {
            return {400, "application/json", "{\"success\":false,\"error\":\"" + res.error_message + "\",\"action\":\"" + res.actionable_fix + "\"}"};
        }
        return {200, "application/json", parsers::AnsysCsvParser::to_json(results)};
    });

    // Multicore Benchmark
    add_route("POST", "/api/multicore/benchmark", [](const HttpRequest& req) -> HttpResponse {
        multicore::BatchSolveConfig cfg;
        auto extract_int = [&](const std::string& key, int def) -> int {
            size_t pos = req.body.find("\"" + key + "\":");
            if (pos == std::string::npos) return def;
            size_t start = pos + key.size() + 3;
            size_t end = req.body.find_first_of(",}", start);
            try { return std::stoi(req.body.substr(start, end - start)); } catch (...) { return def; }
        };
        cfg.case_count = extract_int("case_count", 3000);
        cfg.mesh_size_n = extract_int("mesh_size_n", 50);

        auto report = multicore::MulticoreEngine::run_benchmark(cfg, 45.2);
        return {200, "application/json", multicore::MulticoreEngine::to_json(report)};
    });

    // Embedded Telemetry & Discovery
    add_route("GET", "/api/embedded/telemetry", [](const HttpRequest&) -> HttpResponse {
        return {200, "application/json", embedded::EmbeddedWorkbenchManager::instance().get_all_json()};
    });

    // Embedded Actions (Connect, Disconnect, Inject Fault, Recover)
    add_route("POST", "/api/embedded/action", [](const HttpRequest& req) -> HttpResponse {
        std::string body = req.body;
        auto get_str = [&](const std::string& key) -> std::string {
            size_t pos = body.find("\"" + key + "\":\"");
            if (pos == std::string::npos) return "";
            size_t start = pos + key.size() + 4;
            size_t end = body.find("\"", start);
            return body.substr(start, end - start);
        };

        std::string dev = get_str("device");
        std::string action = get_str("action");

        auto& mgr = embedded::EmbeddedWorkbenchManager::instance();
        if (action == "connect") mgr.connect_device(dev);
        else if (action == "disconnect") mgr.disconnect_device(dev);
        else if (action == "inject_timeout") mgr.inject_fault(dev, embedded::FaultMode::TIMEOUT_ON_NEXT_READ);
        else if (action == "inject_crc") mgr.inject_fault(dev, embedded::FaultMode::CHECKSUM_ERROR);
        else if (action == "recover") mgr.recover_device(dev);

        return {200, "application/json", mgr.get_all_json()};
    });

    // Backprop Calculation
    add_route("GET", "/api/backprop/calculate", [](const HttpRequest&) -> HttpResponse {
        backprop::ModelParameters p0{0.5, 0.0, 1.0, 0.0};
        auto fwd = backprop::BackpropEngine::forward(p0, 1.0, 2.0);
        auto grads = backprop::BackpropEngine::compute_gradients(p0, fwd);
        auto checks = backprop::BackpropEngine::run_gradient_check(p0, 1.0, 2.0, 1e-6);

        backprop::ModelParameters p1 = p0;
        double lr = 0.1;
        p1.w -= lr * grads.dJ_dw;
        p1.b -= lr * grads.dJ_db;
        p1.v -= lr * grads.dJ_dv;
        p1.c -= lr * grads.dJ_dc;

        auto history = backprop::BackpropEngine::train(p0, 1.0, 2.0, lr, 100);

        return {200, "application/json", backprop::BackpropEngine::to_json(fwd, grads, p1, checks, history)};
    });
}

HttpResponse HttpServer::serve_static_file(const std::string& rel_path) {
    std::string safe_path = rel_path;
    if (safe_path.empty() || safe_path == "/") {
        safe_path = "index.html";
    }
    if (safe_path.front() == '/') {
        safe_path = safe_path.substr(1);
    }

    // Check primary static_directory
    fs::path p1 = fs::path(static_directory) / safe_path;
    fs::path target_path;
    if (fs::exists(p1)) {
        target_path = p1;
    } else {
        // Fallback checks
        fs::path p2 = fs::path("src/frontend") / safe_path;
        fs::path p3 = fs::path("./src/frontend") / safe_path;
        if (fs::exists(p2)) target_path = p2;
        else if (fs::exists(p3)) target_path = p3;
        else {
            return {404, "text/plain", "404 Not Found: " + rel_path};
        }
    }

    std::ifstream file(target_path, std::ios::binary);
    if (!file.is_open()) {
        return {404, "text/plain", "Cannot open static file: " + target_path.string()};
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return {200, get_mime_type(target_path.string()), content};
}

HttpResponse HttpServer::handle_request(const HttpRequest& req) {
    // OPTIONS for CORS preflight
    if (req.method == "OPTIONS") {
        HttpResponse res{204, "text/plain", ""};
        res.headers["Access-Control-Allow-Origin"] = "*";
        res.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
        res.headers["Access-Control-Allow-Headers"] = "Content-Type";
        return res;
    }

    std::string route_key = req.method + " " + req.path;
    auto it = routes.find(route_key);
    HttpResponse res;
    if (it != routes.end()) {
        res = it->second(req);
    } else if (req.method == "GET" || req.method == "HEAD") {
        res = serve_static_file(req.path);
    } else {
        res = {404, "application/json", "{\"error\":\"Route not found: " + route_key + "\"}"};
    }

    res.headers["Access-Control-Allow-Origin"] = "*";
    return res;
}

bool HttpServer::start() {
    if (running.load()) return true;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return false;
    }
#endif

    running.store(true);
    listener_thread = std::thread(&HttpServer::run_listener, this);
    return true;
}

void HttpServer::stop() {
    if (!running.load()) return;
    running.store(false);

    // Connect dummy client to unblock accept()
#ifdef _WIN32
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s != INVALID_SOCKET) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(server_port));
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(s, (sockaddr*)&addr, sizeof(addr));
        closesocket(s);
    }
#endif

    if (listener_thread.joinable()) {
        listener_thread.join();
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::run_listener() {
#ifdef _WIN32
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) {
        running.store(false);
        return;
    }

    BOOL opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(static_cast<u_short>(server_port));

    if (bind(listen_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        // Try fallback port
        server_port++;
        server_addr.sin_port = htons(static_cast<u_short>(server_port));
        if (bind(listen_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(listen_sock);
            running.store(false);
            return;
        }
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listen_sock);
        running.store(false);
        return;
    }

    while (running.load()) {
        sockaddr_in client_addr{};
        int client_len = sizeof(client_addr);
        SOCKET client_sock = accept(listen_sock, (sockaddr*)&client_addr, &client_len);
        if (client_sock == INVALID_SOCKET) continue;
        if (!running.load()) {
            closesocket(client_sock);
            break;
        }

        std::thread([this, client_sock]() {
            std::vector<char> buffer(65536);
            int bytes_received = recv(client_sock, buffer.data(), static_cast<int>(buffer.size()) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::string raw(buffer.data(), bytes_received);

                HttpRequest req;
                std::istringstream stream(raw);
                std::string line;
                if (std::getline(stream, line)) {
                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                    std::istringstream line_stream(line);
                    line_stream >> req.method >> req.path;
                    size_t qpos = req.path.find('?');
                    if (qpos != std::string::npos) {
                        req.query = req.path.substr(qpos + 1);
                        req.path = req.path.substr(0, qpos);
                    }
                }

                size_t content_length = 0;
                while (std::getline(stream, line) && line != "\r" && !line.empty()) {
                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                    size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        std::string k = line.substr(0, colon);
                        std::string v = line.substr(colon + 1);
                        size_t start = v.find_first_not_of(" \t");
                        if (start != std::string::npos) v = v.substr(start);
                        req.headers[k] = v;
                        if (k == "Content-Length" || k == "content-length") {
                            try { content_length = std::stoul(v); } catch (...) {}
                        }
                    }
                }

                // Read body
                size_t header_end = raw.find("\r\n\r\n");
                if (header_end != std::string::npos) {
                    req.body = raw.substr(header_end + 4);
                }
                while (req.body.size() < content_length) {
                    int extra = recv(client_sock, buffer.data(), static_cast<int>(buffer.size()) - 1, 0);
                    if (extra <= 0) break;
                    req.body.append(buffer.data(), extra);
                }

                HttpResponse res = handle_request(req);

                std::ostringstream response_stream;
                response_stream << "HTTP/1.1 " << res.status_code << " OK\r\n";
                response_stream << "Content-Type: " << res.content_type << "\r\n";
                response_stream << "Content-Length: " << res.body.size() << "\r\n";
                response_stream << "Connection: close\r\n";
                for (const auto& [hk, hv] : res.headers) {
                    response_stream << hk << ": " << hv << "\r\n";
                }
                response_stream << "\r\n";
                response_stream << res.body;

                std::string full_res = response_stream.str();
                const char* send_ptr = full_res.data();
                int total_to_send = static_cast<int>(full_res.size());
                while (total_to_send > 0) {
                    int sent = send(client_sock, send_ptr, total_to_send, 0);
                    if (sent <= 0) break;
                    send_ptr += sent;
                    total_to_send -= sent;
                }
            }
            closesocket(client_sock);
        }).detach();
    }

    closesocket(listen_sock);
#endif
}

} // namespace workbench::server
