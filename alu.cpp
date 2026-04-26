#include <iostream>
#include <stdint.h> 

using namespace std;

// --- EXISTING HARDWARE ---
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
            case 2: return operand_a & operand_b; // Restored Bitwise AND
            case 3: return operand_a | operand_b; // Restored Bitwise OR
            default: return 0; 
        }
    }
};

class InstructionMemory {
private:
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

// --- NEW: Data RAM (SRAM) ---
class DataMemory {
private:
    uint32_t memory[256] = {0}; // 256 slots of main memory
public:
    void write_mem(uint32_t address, uint32_t data) {
        if (address < 256) memory[address] = data;
    }
    uint32_t read_mem(uint32_t address) {
        if (address < 256) return memory[address];
        return 0;
    }
};

// --- UPDATED: CPU Core ---
class CPU_Core {
private:
    SimpleALU alu;
    RegisterFile regs;
    InstructionMemory i_mem;
    DataMemory d_mem; 
    uint32_t pc; 

public:
    CPU_Core() { pc = 0; }
    void load_data(uint8_t reg, uint32_t val) { regs.write_reg(reg, val); }
    void flash_rom(uint32_t address, uint32_t instruction) { i_mem.load_program(address, instruction); }

    void tick() {
        uint32_t instruction = i_mem.fetch(pc);
        if (instruction == 0) {
            cout << "\nHALT: End of program reached at PC=" << pc << endl;
            return;
        }

        uint8_t opcode   = (instruction >> 24) & 0xFF; 
        uint8_t reg_a    = (instruction >> 16) & 0xFF;
        uint8_t reg_b    = (instruction >> 8)  & 0xFF;

        bool branch_taken = false;

        // Route the instruction based on Opcode (The Multiplexer Logic)
        if (opcode == 5) { 
            // STR: Store Data to RAM
            uint32_t data_to_store = regs.read_reg(reg_a);
            uint32_t ram_address   = regs.read_reg(reg_b);
            d_mem.write_mem(ram_address, data_to_store);
            cout << "Clock Tick [PC " << pc << "]: STORE -> Wrote " << data_to_store 
                 << " to RAM Address " << ram_address << endl;
        } 
        else if (opcode == 6) { 
            // LDR: Load Data from RAM
            uint32_t ram_address = regs.read_reg(reg_b);
            uint32_t loaded_data = d_mem.read_mem(ram_address);
            regs.write_reg(reg_a, loaded_data);
            cout << "Clock Tick [PC " << pc << "]: LOAD  -> Read " << loaded_data 
                 << " from RAM Address " << ram_address << " into Reg " << (int)reg_a << endl;
        }
        else if (opcode == 4) { 
            // BEQ: Branch logic
            pc = reg_a; 
            branch_taken = true;
        } 
        else {
            // ALU Math
            uint32_t val_a = regs.read_reg(reg_b);
            uint32_t val_b = regs.read_reg(instruction & 0xFF);
            uint32_t alu_out = alu.execute(opcode, val_a, val_b);
            regs.write_reg(reg_a, alu_out);
            cout << "Clock Tick [PC " << pc << "]: MATH  -> Opcode " << (int)opcode 
                 << " | Result " << alu_out << " into Reg " << (int)reg_a << endl;
        }

        if (!branch_taken) pc++; 
    }

    void run(int cycles) {
        for (int i = 0; i < cycles; i++) {
            if (i_mem.fetch(pc) == 0) break; 
            tick();
        }
    }
};

int main() {
    CPU_Core my_cpu;
    cout << "--- Starting Day 6: The Load/Store Architecture ---" << endl;

    // Setup: Reg 1 has the payload (999). Reg 2 has the target RAM address (15).
    my_cpu.load_data(1, 999); 
    my_cpu.load_data(2, 15);  

    // PC 0: STR (Opcode 05) -> Store contents of Reg 1 into RAM address held in Reg 2
    my_cpu.flash_rom(0, 0x05010200); 
    
    // PC 1: LDR (Opcode 06) -> Load RAM address held in Reg 2 into Reg 3
    my_cpu.flash_rom(1, 0x06030200); 

    // Let the CPU run!
    my_cpu.run(5); 

    return 0;
}