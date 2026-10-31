#ifndef WORKBENCH_EMBEDDED_MOCKS_HPP
#define WORKBENCH_EMBEDDED_MOCKS_HPP

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace workbench::embedded {

enum class DeviceType {
    MICROCONTROLLER,
    FPGA,
    JTAG_ADAPTER
};

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR_STATE,
    RECOVERING
};

enum class FaultMode {
    NONE,
    TIMEOUT_ON_NEXT_READ,
    CHECKSUM_ERROR,
    DISCONNECT_TRANSIENT
};

struct DeviceCapabilities {
    std::string device_name;
    DeviceType type;
    std::string hardware_id;
    std::string firmware_version;
    std::string connector_type;
    std::string host_protocol;
    std::string voltage_domains;
    std::vector<std::string> supported_features;
};

struct DeviceTelemetry {
    std::string timestamp_iso;
    std::string device_name;
    DeviceType type;
    ConnectionState state;
    bool is_simulated = true;
    double primary_voltage_v = 0.0;
    double secondary_voltage_v = 0.0;
    double internal_temp_c = 0.0;
    uint64_t packets_sent = 0;
    uint64_t packets_received = 0;
    uint32_t error_count = 0;
    std::string status_message;
    std::string role_explanation; // Specific explanation of device purpose
};

class IDeviceAdapter {
public:
    virtual ~IDeviceAdapter() = default;
    virtual DeviceCapabilities get_capabilities() const = 0;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual ConnectionState get_state() const = 0;
    virtual DeviceTelemetry poll_telemetry() = 0;
    virtual void inject_fault(FaultMode fault) = 0;
    virtual bool trigger_recovery() = 0;
};

class McuMockAdapter : public IDeviceAdapter {
public:
    McuMockAdapter();
    DeviceCapabilities get_capabilities() const override;
    bool connect() override;
    bool disconnect() override;
    ConnectionState get_state() const override;
    DeviceTelemetry poll_telemetry() override;
    void inject_fault(FaultMode fault) override;
    bool trigger_recovery() override;

private:
    ConnectionState state = ConnectionState::CONNECTED;
    FaultMode active_fault = FaultMode::NONE;
    uint64_t tx_count = 1420;
    uint64_t rx_count = 1420;
    uint32_t errors = 0;
    double sim_time_sec = 0.0;
};

class FpgaMockAdapter : public IDeviceAdapter {
public:
    FpgaMockAdapter();
    DeviceCapabilities get_capabilities() const override;
    bool connect() override;
    bool disconnect() override;
    ConnectionState get_state() const override;
    DeviceTelemetry poll_telemetry() override;
    void inject_fault(FaultMode fault) override;
    bool trigger_recovery() override;

private:
    ConnectionState state = ConnectionState::CONNECTED;
    FaultMode active_fault = FaultMode::NONE;
    uint64_t tx_count = 5890;
    uint64_t rx_count = 5890;
    uint32_t errors = 0;
    double sim_time_sec = 0.0;
};

class JtagMockAdapter : public IDeviceAdapter {
public:
    JtagMockAdapter();
    DeviceCapabilities get_capabilities() const override;
    bool connect() override;
    bool disconnect() override;
    ConnectionState get_state() const override;
    DeviceTelemetry poll_telemetry() override;
    void inject_fault(FaultMode fault) override;
    bool trigger_recovery() override;

private:
    ConnectionState state = ConnectionState::CONNECTED;
    FaultMode active_fault = FaultMode::NONE;
    uint64_t tx_count = 320;
    uint64_t rx_count = 320;
    uint32_t errors = 0;
    double sim_time_sec = 0.0;
};

class EmbeddedWorkbenchManager {
public:
    static EmbeddedWorkbenchManager& instance();

    std::vector<DeviceCapabilities> discover_all();
    std::vector<DeviceTelemetry> poll_all();
    bool connect_device(const std::string& name);
    bool disconnect_device(const std::string& name);
    bool inject_fault(const std::string& name, FaultMode fault);
    bool recover_device(const std::string& name);
    std::string get_all_json();

private:
    EmbeddedWorkbenchManager();
    std::unique_ptr<McuMockAdapter> mcu;
    std::unique_ptr<FpgaMockAdapter> fpga;
    std::unique_ptr<JtagMockAdapter> jtag;
};

} // namespace workbench::embedded

#endif // WORKBENCH_EMBEDDED_MOCKS_HPP
