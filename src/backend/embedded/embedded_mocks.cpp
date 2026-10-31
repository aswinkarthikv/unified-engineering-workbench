#include "embedded_mocks.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace workbench::embedded {

// MCU Mock Adapter
McuMockAdapter::McuMockAdapter() = default;

DeviceCapabilities McuMockAdapter::get_capabilities() const {
    return DeviceCapabilities{
        "STM32H753ZI-Nucleo",
        DeviceType::MICROCONTROLLER,
        "USB-VID:0483-PID:374E",
        "v2.4.1-rc3-rtos",
        "Micro-USB / Morpho ST Headers",
        "USB CDC-ACM (Virtual COM @ 921600 baud)",
        "3.3V Logic (VDD), 5.0V VBUS, 1.2V Core LDO",
        {"FreeRTOS Kernel 10.4", "ADC DMA Telemetry (16-bit)", "SPI Sensor Bus Master", "Hardware CRC32"}
    };
}

bool McuMockAdapter::connect() {
    state = ConnectionState::CONNECTED;
    active_fault = FaultMode::NONE;
    return true;
}

bool McuMockAdapter::disconnect() {
    state = ConnectionState::DISCONNECTED;
    return true;
}

ConnectionState McuMockAdapter::get_state() const { return state; }

void McuMockAdapter::inject_fault(FaultMode fault) {
    active_fault = fault;
    if (fault != FaultMode::NONE) {
        state = ConnectionState::ERROR_STATE;
        errors++;
    }
}

bool McuMockAdapter::trigger_recovery() {
    state = ConnectionState::RECOVERING;
    active_fault = FaultMode::NONE;
    state = ConnectionState::CONNECTED;
    return true;
}

DeviceTelemetry McuMockAdapter::poll_telemetry() {
    sim_time_sec += 0.5;
    DeviceTelemetry t;
    t.device_name = "STM32H753ZI-Nucleo";
    t.type = DeviceType::MICROCONTROLLER;
    t.is_simulated = true;
    t.state = state;
    t.role_explanation = "Primary edge compute node: Runs FreeRTOS sensor fusion, PID thermal control loop, and telemetry acquisition.";

    if (state == ConnectionState::DISCONNECTED) {
        t.status_message = "Device disconnected. Link idle.";
        return t;
    }

    if (active_fault == FaultMode::TIMEOUT_ON_NEXT_READ) {
        t.status_message = "ERROR: Command timeout (3000 ms elapsed without CDC ACK). Link stalled.";
        t.error_count = ++errors;
        state = ConnectionState::ERROR_STATE;
        t.state = state;
        return t;
    } else if (active_fault == FaultMode::CHECKSUM_ERROR) {
        t.status_message = "ERROR: Packet CRC32 checksum mismatch (expected 0x8F21, got 0x1102). Dropping corrupt frame.";
        t.error_count = ++errors;
        state = ConnectionState::ERROR_STATE;
        t.state = state;
        return t;
    }

    tx_count += 4;
    rx_count += 4;
    t.packets_sent = tx_count;
    t.packets_received = rx_count;
    t.error_count = errors;
    t.primary_voltage_v = 3.295 + 0.015 * std::sin(sim_time_sec * 0.4);
    t.secondary_voltage_v = 1.201 + 0.005 * std::cos(sim_time_sec * 0.2);
    t.internal_temp_c = 38.2 + 1.2 * std::sin(sim_time_sec * 0.1);
    t.status_message = "Nominal RTOS execution. 480 MHz PLL lock OK. ADC DMA streaming active.";

    return t;
}

// FPGA Mock Adapter
FpgaMockAdapter::FpgaMockAdapter() = default;

DeviceCapabilities FpgaMockAdapter::get_capabilities() const {
    return DeviceCapabilities{
        "Xilinx Artix-7 XC7A35T (Basys 3)",
        DeviceType::FPGA,
        "USB-VID:0403-PID:6010",
        "Bitstream SHA:0x8E19B240",
        "USB-B FT2232HQ / 4x 12-pin PMOD",
        "FTDI FT2232H High-Speed USB-to-UART/FIFO",
        "1.0V VCCINT (Core), 1.8V VCCAUX, 3.3V VCCO Banks",
        {"33,280 Logic Cells", "90 DSP48E1 Slices", "Dual 12-bit SYSMON ADC", "SPI Flash Configuration"}
    };
}

bool FpgaMockAdapter::connect() {
    state = ConnectionState::CONNECTED;
    active_fault = FaultMode::NONE;
    return true;
}

bool FpgaMockAdapter::disconnect() {
    state = ConnectionState::DISCONNECTED;
    return true;
}

ConnectionState FpgaMockAdapter::get_state() const { return state; }

void FpgaMockAdapter::inject_fault(FaultMode fault) {
    active_fault = fault;
    if (fault != FaultMode::NONE) {
        state = ConnectionState::ERROR_STATE;
        errors++;
    }
}

bool FpgaMockAdapter::trigger_recovery() {
    state = ConnectionState::RECOVERING;
    active_fault = FaultMode::NONE;
    state = ConnectionState::CONNECTED;
    return true;
}

DeviceTelemetry FpgaMockAdapter::poll_telemetry() {
    sim_time_sec += 0.5;
    DeviceTelemetry t;
    t.device_name = "Xilinx Artix-7 XC7A35T";
    t.type = DeviceType::FPGA;
    t.is_simulated = true;
    t.state = state;
    t.role_explanation = "High-throughput hardware accelerator: Implements parallel DSP filter pipes and fast trigger logic.";

    if (state == ConnectionState::DISCONNECTED) {
        t.status_message = "FPGA board disconnected. FTDI channel closed.";
        return t;
    }

    if (active_fault == FaultMode::TIMEOUT_ON_NEXT_READ) {
        t.status_message = "ERROR: FT2232H FIFO buffer timeout. No RX burst detected.";
        t.error_count = ++errors;
        state = ConnectionState::ERROR_STATE;
        t.state = state;
        return t;
    }

    tx_count += 32;
    rx_count += 32;
    t.packets_sent = tx_count;
    t.packets_received = rx_count;
    t.error_count = errors;
    t.primary_voltage_v = 1.002 + 0.008 * std::sin(sim_time_sec * 0.3); // Core 1.0V
    t.secondary_voltage_v = 1.801 + 0.004 * std::cos(sim_time_sec * 0.2); // Aux 1.8V
    t.internal_temp_c = 43.5 + 2.1 * std::sin(sim_time_sec * 0.15); // SYSMON die temp
    t.status_message = "SYSMON die telemetry nominal. FPGA bitstream loaded. 34% LUT utilization.";

    return t;
}

// JTAG Debug Adapter Mock Adapter
JtagMockAdapter::JtagMockAdapter() = default;

DeviceCapabilities JtagMockAdapter::get_capabilities() const {
    return DeviceCapabilities{
        "ST-Link V3 Modular Debug Probe",
        DeviceType::JTAG_ADAPTER,
        "USB-VID:0483-PID:374F",
        "STLINK-V3 firmware v3J8M3",
        "0.05\" 10-pin Cortex Debug Header (Samtec FTSH)",
        "USB 2.0 High Speed (480 Mbps) Bulk Endpoints",
        "1.65V - 3.60V Target VREF Sensing (Passive Monitor)",
        {"JTAG / SWD Debug Master", "Virtual COM Port (VCP)", "SWO Trace Capture", "OpenOCD / GDB Compatible"}
    };
}

bool JtagMockAdapter::connect() {
    state = ConnectionState::CONNECTED;
    active_fault = FaultMode::NONE;
    return true;
}

bool JtagMockAdapter::disconnect() {
    state = ConnectionState::DISCONNECTED;
    return true;
}

ConnectionState JtagMockAdapter::get_state() const { return state; }

void JtagMockAdapter::inject_fault(FaultMode fault) {
    active_fault = fault;
    if (fault != FaultMode::NONE) {
        state = ConnectionState::ERROR_STATE;
        errors++;
    }
}

bool JtagMockAdapter::trigger_recovery() {
    state = ConnectionState::RECOVERING;
    active_fault = FaultMode::NONE;
    state = ConnectionState::CONNECTED;
    return true;
}

DeviceTelemetry JtagMockAdapter::poll_telemetry() {
    sim_time_sec += 0.5;
    DeviceTelemetry t;
    t.device_name = "ST-Link V3 Debug Adapter";
    t.type = DeviceType::JTAG_ADAPTER;
    t.is_simulated = true;
    t.state = state;
    t.role_explanation = "Dedicated hardware debug/programming probe: Controls IEEE 1149.1 boundary-scan TAP state machines and SWD. NOT a generic compute board.";

    if (state == ConnectionState::DISCONNECTED) {
        t.status_message = "JTAG Probe disconnected. Target bus released to tri-state.";
        return t;
    }

    if (active_fault == FaultMode::TIMEOUT_ON_NEXT_READ) {
        t.status_message = "ERROR: SWD/JTAG ACK timeout: Target core unresponsive (halt failure or low target voltage).";
        t.error_count = ++errors;
        state = ConnectionState::ERROR_STATE;
        t.state = state;
        return t;
    }

    tx_count += 2;
    rx_count += 2;
    t.packets_sent = tx_count;
    t.packets_received = rx_count;
    t.error_count = errors;
    t.primary_voltage_v = 3.284 + 0.006 * std::sin(sim_time_sec * 0.1); // Target VREF sensed
    t.secondary_voltage_v = 5.012; // Host USB 5V VBUS
    t.internal_temp_c = 34.1 + 0.6 * std::sin(sim_time_sec * 0.05);
    t.status_message = "TAP State: Run-Test/Idle. TCK Clock: 4.00 MHz. Target Core: Cortex-M7 HALTED. IDCODE: 0x6BA02477.";

    return t;
}

// Embedded Workbench Manager
EmbeddedWorkbenchManager::EmbeddedWorkbenchManager() 
    : mcu(std::make_unique<McuMockAdapter>()),
      fpga(std::make_unique<FpgaMockAdapter>()),
      jtag(std::make_unique<JtagMockAdapter>()) {}

EmbeddedWorkbenchManager& EmbeddedWorkbenchManager::instance() {
    static EmbeddedWorkbenchManager inst;
    return inst;
}

std::vector<DeviceCapabilities> EmbeddedWorkbenchManager::discover_all() {
    return {mcu->get_capabilities(), fpga->get_capabilities(), jtag->get_capabilities()};
}

std::vector<DeviceTelemetry> EmbeddedWorkbenchManager::poll_all() {
    return {mcu->poll_telemetry(), fpga->poll_telemetry(), jtag->poll_telemetry()};
}

bool EmbeddedWorkbenchManager::connect_device(const std::string& name) {
    if (name.find("MCU") != std::string::npos || name.find("STM32") != std::string::npos) return mcu->connect();
    if (name.find("FPGA") != std::string::npos || name.find("Artix") != std::string::npos) return fpga->connect();
    if (name.find("JTAG") != std::string::npos || name.find("Link") != std::string::npos) return jtag->connect();
    return false;
}

bool EmbeddedWorkbenchManager::disconnect_device(const std::string& name) {
    if (name.find("MCU") != std::string::npos || name.find("STM32") != std::string::npos) return mcu->disconnect();
    if (name.find("FPGA") != std::string::npos || name.find("Artix") != std::string::npos) return fpga->disconnect();
    if (name.find("JTAG") != std::string::npos || name.find("Link") != std::string::npos) return jtag->disconnect();
    return false;
}

bool EmbeddedWorkbenchManager::inject_fault(const std::string& name, FaultMode fault) {
    if (name.find("MCU") != std::string::npos || name.find("STM32") != std::string::npos) { mcu->inject_fault(fault); return true; }
    if (name.find("FPGA") != std::string::npos || name.find("Artix") != std::string::npos) { fpga->inject_fault(fault); return true; }
    if (name.find("JTAG") != std::string::npos || name.find("Link") != std::string::npos) { jtag->inject_fault(fault); return true; }
    return false;
}

bool EmbeddedWorkbenchManager::recover_device(const std::string& name) {
    if (name.find("MCU") != std::string::npos || name.find("STM32") != std::string::npos) return mcu->trigger_recovery();
    if (name.find("FPGA") != std::string::npos || name.find("Artix") != std::string::npos) return fpga->trigger_recovery();
    if (name.find("JTAG") != std::string::npos || name.find("Link") != std::string::npos) return jtag->trigger_recovery();
    return false;
}

std::string EmbeddedWorkbenchManager::get_all_json() {
    auto caps = discover_all();
    auto tels = poll_all();

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{\"devices\":[";
    for (size_t i = 0; i < caps.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& c = caps[i];
        const auto& t = tels[i];

        ss << "{\"name\":\"" << c.device_name << "\",";
        ss << "\"hw_id\":\"" << c.hardware_id << "\",";
        ss << "\"fw_ver\":\"" << c.firmware_version << "\",";
        ss << "\"connector\":\"" << c.connector_type << "\",";
        ss << "\"protocol\":\"" << c.host_protocol << "\",";
        ss << "\"voltage_domains\":\"" << c.voltage_domains << "\",";
        ss << "\"role\":\"" << t.role_explanation << "\",";
        ss << "\"status_msg\":\"" << t.status_message << "\",";
        ss << "\"state\":\"" << (t.state == ConnectionState::CONNECTED ? "CONNECTED" : 
                                 (t.state == ConnectionState::ERROR_STATE ? "ERROR" : 
                                  (t.state == ConnectionState::RECOVERING ? "RECOVERING" : "DISCONNECTED"))) << "\",";
        ss << "\"is_simulated\":true,";
        ss << "\"primary_v\":" << t.primary_voltage_v << ",";
        ss << "\"secondary_v\":" << t.secondary_voltage_v << ",";
        ss << "\"temp_c\":" << t.internal_temp_c << ",";
        ss << "\"packets_tx\":" << t.packets_sent << ",";
        ss << "\"packets_rx\":" << t.packets_received << ",";
        ss << "\"error_count\":" << t.error_count << "}";
    }
    ss << "]}";

    return ss.str();
}

} // namespace workbench::embedded
