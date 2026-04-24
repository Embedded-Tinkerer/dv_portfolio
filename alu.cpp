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

// --- UPDATED: CPU Core with Branching ---
class CPU_Core {
private:
    SimpleALU alu;
    RegisterFile regs;
    InstructionMemory i_mem;
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
        uint8_t dest_reg = (instruction >> 16) & 0xFF;
        uint8_t src_a    = (instruction >> 8)  & 0xFF;
        uint8_t src_b    = (instruction >> 0)  & 0xFF;

        uint32_t val_a = regs.read_reg(src_a);
        uint32_t val_b = regs.read_reg(src_b);

        bool branch_taken = false;

        // NEW: Hardware Branching Logic
        if (opcode == 4) { 
            if (val_a == val_b) {
                pc = dest_reg; // Overwrite the Program Counter!
                branch_taken = true;
                cout << "Clock Tick: BRANCH TAKEN -> Jumping to PC " << pc << endl;
            } else {
                cout << "Clock Tick: Branch condition false (continuing normally)" << endl;
            }
        } 
        // Normal Math Logic
        else {
            uint32_t alu_out = alu.execute(opcode, val_a, val_b);
            regs.write_reg(dest_reg, alu_out);
            cout << "Clock Tick [PC " << pc << "]: Math Opcode " << (int)opcode 
                 << " | Wrote " << alu_out << " to Reg " << (int)dest_reg << endl;
        }

        // Only increment PC if we didn't just jump to a new address
        if (!branch_taken) {
            pc++; 
        }
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
    cout << "--- Starting Day 5: Hardware Branching (While Loop) ---" << endl;

    // 1. Setup Data Memory for the Loop
    my_cpu.load_data(1, 0); // Reg 1: The Counter (Starts at 0)
    my_cpu.load_data(2, 1); // Reg 2: The Step (Always 1)
    my_cpu.load_data(3, 3); // Reg 3: The Target (Count to 3)

    // 2. Flash the Program Memory
    
    // PC 0: ADD Reg 1 and Reg 2, save in Reg 1 (Counter = Counter + 1)
    my_cpu.flash_rom(0, 0x00010102); 

    // PC 1: BEQ -> If Reg 1 == Reg 3, jump to Target PC 3 (Exit the loop)
    my_cpu.flash_rom(1, 0x04030103); 

    // ==========================================
    // YOUR ASSIGNMENT: Write the Unconditional Jump
    // ==========================================
    // PC 2: BEQ -> If Reg 2 == Reg 2, jump to Target PC 0 (Loop back to start)
    // Reg 2 always equals Reg 2, so this will ALWAYS jump back.
    my_cpu.flash_rom(2, 0x00000000); // REPLACE THIS HEX CODE

    // Run the CPU for 15 cycles (it should exit the loop safely before that)
    my_cpu.run(15); 

    return 0;
}