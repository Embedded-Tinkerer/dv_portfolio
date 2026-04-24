#include <iostream>
#include <stdint.h>

using namespace std;

// --- The FPGA Hardware (AXI4-Lite Slave) ---
class AXI_Hardware_Accelerator {
private:
    // The 4 standard AXI4-Lite Registers
    uint32_t slv_reg0 = 0; // 0x00: Control Register (Bit 0 = Start)
    uint32_t slv_reg1 = 0; // 0x04: Status Register  (Bit 0 = Done)
    uint32_t slv_reg2 = 0; // 0x08: Data In
    uint32_t slv_reg3 = 0; // 0x0C: Data Out

    // The actual "Silicon" Logic
    void execute_hardware() {
        slv_reg1 = 0; // Clear Status (Hardware is busy)
        
        // The combinatorial logic (Squaring the input)
        slv_reg3 = slv_reg2 * slv_reg2; 
        
        slv_reg1 = 1; // Set Status (Hardware is done)
        slv_reg0 = 0; // Auto-clear the start bit so it doesn't loop
    }

public:
    // The AXI Write Channel
    void axi_write(uint32_t offset, uint32_t data) {
        switch (offset) {
            case 0x00: 
                slv_reg0 = data; 
                // Hardware Trigger: If ARM writes '1' to Control, fire the logic
                if (data == 1) { execute_hardware(); }
                break;
            case 0x04: slv_reg1 = data; break; 
            case 0x08: slv_reg2 = data; break; 
            case 0x0C: slv_reg3 = data; break; 
            default: cout << "AXI Write Error: Invalid Offset" << endl;
        }
    }

    // The AXI Read Channel
    uint32_t axi_read(uint32_t offset) {
        switch (offset) {
            case 0x00: return slv_reg0;
            case 0x04: return slv_reg1;
            case 0x08: return slv_reg2;
            case 0x0C: return slv_reg3;
            default: 
                cout << "AXI Read Error: Invalid Offset" << endl; 
                return 0;
        }
    }
};

// --- The ARM Processor (AXI Master) ---
int main() {
    AXI_Hardware_Accelerator custom_ip;

    cout << "--- Starting AXI4-Lite PS-PL Transaction ---" << endl;

    // 1. ARM writes raw data to the IP
    uint32_t test_val = 15;
    custom_ip.axi_write(0x08, test_val);
    cout << "ARM: Wrote " << test_val << " to Data In (0x08)" << endl;

    // 2. ARM tells the IP to start executing
    custom_ip.axi_write(0x00, 1);
    cout << "ARM: Asserted Start Bit in Control Reg (0x00)" << endl;

    // 3. ARM polls the Status Register waiting for the 'Done' signal
    // In real hardware, this prevents the CPU from reading garbage data before the math finishes
    while (custom_ip.axi_read(0x04) != 1) {
        // Spinning/Waiting...
    }
    cout << "ARM: Hardware asserted Done Bit in Status Reg (0x04)" << endl;

    // 4. ARM reads the final computed result
    uint32_t final_answer = custom_ip.axi_read(0x0C);
    cout << "ARM: Read Result " << final_answer << " from Data Out (0x0C)" << endl;

    return 0;
}