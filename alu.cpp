#include <iostream>
#include <stdint.h> 

using namespace std;

class RegisterFile {
private:
    uint32_t registers[32] = {0}; 
public:
    void write_reg(uint8_t address, uint32_t data) {
        if (address < 32) registers[address] = data;
    }
    uint32_t read_reg(uint8_t address) {
        if (address < 32) return registers[address];
        return 0;
    }
};

class SimpleALU {
public:
    uint32_t execute(uint8_t opcode, uint32_t operand_a, uint32_t operand_b) {
        switch (opcode) {
            case 0: return operand_a + operand_b; 
            case 1: return operand_a - operand_b; 
            case 2: return operand_a & operand_b; 
            case 3: return operand_a | operand_b; 
            default: return 0; 
        }
    }
};

// --- NEW: Instruction RAM ---
class InstructionMemory {
private:
    // This perfectly models an L1 Instruction Cache (64 slots)
    uint32_t memory[64] = {0}; 
public:
    void load_program(uint32_t address, uint32_t instruction) {
        if (address < 64) memory[address] = instruction;
    }
    uint32_t fetch(uint32_t address) {
        if (address < 64) return memory[address];
        return 0;
    }
};

// --- UPDATED: The CPU Core ---
class CPU_Core {
private:
    SimpleALU alu;
    RegisterFile regs;
    InstructionMemory i_mem;
    
    uint32_t pc; // NEW: The Program Counter

public:
    CPU_Core() {
        pc = 0; // The CPU boots up pointing to memory address 0
    }

    // Backdoor injection for testing
    void load_data(uint8_t reg, uint32_t val) { regs.write_reg(reg, val); }
    void flash_rom(uint32_t address, uint32_t instruction) { i_mem.load_program(address, instruction); }

    // THE CLOCK CYCLE
    void tick() {
        // 1. FETCH
        uint32_t instruction = i_mem.fetch(pc);
        
        // Safety: If the CPU reads an empty memory slot, stop the clock.
        if (instruction == 0) {
            cout << "HALT: End of program reached at PC=" << pc << endl;
            return;
        }

        // 2. DECODE
        uint8_t opcode   = (instruction >> 24) & 0xFF; 
        uint8_t dest_reg = (instruction >> 16) & 0xFF;
        uint8_t src_a    = (instruction >> 8)  & 0xFF;
        uint8_t src_b    = (instruction >> 0)  & 0xFF;

        // 3. EXECUTE
        uint32_t val_a = regs.read_reg(src_a);
        uint32_t val_b = regs.read_reg(src_b);
        uint32_t alu_out = alu.execute(opcode, val_a, val_b);

        // 4. WRITE-BACK
        regs.write_reg(dest_reg, alu_out);

        // Print the clock cycle results
        cout << "Clock Tick [PC " << pc << "]: "
             << "Opcode " << (int)opcode 
             << " | Wrote " << alu_out << " to Reg " << (int)dest_reg << endl;

        // 5. INCREMENT
        pc++; 
    }

    // Run the CPU automatically for a set number of cycles
    void run(int cycles) {
        for (int i = 0; i < cycles; i++) {
            if (i_mem.fetch(pc) == 0) break; // Auto-halt
            tick();
        }
    }
};

int main() {
    CPU_Core my_cpu;

    cout << "--- Starting Day 4: Automated CPU Loop ---" << endl;

    // 1. Setup Data Memory
    my_cpu.load_data(1, 100); 
    my_cpu.load_data(2, 25);

    // ==========================================
    // YOUR ASSIGNMENT: Flash the Program Memory
    // ==========================================
    // Replace the 0x00000000 with the correct 32-bit hex instructions:
    
    // Instr 0: ADD Reg 1 and Reg 2, save in Reg 3
    my_cpu.flash_rom(0, 0x00030102); 
    
    // Instr 1: SUB Reg 3 and Reg 2, save in Reg 4
    my_cpu.flash_rom(1, 0x01040302); 
    
    // Instr 2: Bitwise OR Reg 4 and Reg 1, save in Reg 5
    my_cpu.flash_rom(2, 0x03050401); 

    // Turn on the clock!
    my_cpu.run(10); // Let the CPU run (it will automatically halt at PC 3)

    return 0;
}