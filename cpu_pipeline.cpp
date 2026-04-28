#include <iostream>
#include <stdint.h> 

using namespace std;

// ==========================================
// 1. THE HARDWARE BLOCKS (From Days 1-6)
// ==========================================

class RegisterFile {
private:
    uint32_t registers[32] = {0}; 
public:
    void write_reg(uint8_t address, uint32_t data) {
        if (address < 32 && address != 0) registers[address] = data; // Reg 0 is usually hardwired to 0 in RISC!
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

class DataMemory {
private:
    uint32_t memory[256] = {0}; 
public:
    void write_mem(uint32_t address, uint32_t data) {
        if (address < 256) memory[address] = data;
    }
    uint32_t read_mem(uint32_t address) {
        if (address < 256) return memory[address];
        return 0;
    }
};

// ==========================================
// 2. THE PIPELINE LATCHES (Structs)
// ==========================================

struct IF_ID_Reg {
    uint32_t pc;
    uint32_t instruction;
};

struct ID_EX_Reg {
    uint8_t  opcode;
    uint8_t  dest_reg;
    uint32_t val_a;
    uint32_t val_b;
    uint8_t src_a;
    uint8_t src_b;
};

struct EX_MEM_Reg {
    uint8_t  opcode;
    uint8_t  dest_reg;
    uint32_t alu_out;
};

struct MEM_WB_Reg {
    uint8_t  dest_reg;
    uint32_t final_data;
};

// ==========================================
// 3. THE PIPELINED CPU CORE
// ==========================================

class Pipelined_CPU {
private:
    // Instantiate the hardware blocks
    SimpleALU alu;
    RegisterFile regs;
    InstructionMemory i_mem;
    DataMemory d_mem;
    uint32_t pc;

    // Instantiate the Pipeline Latches (Current & Next for the clock edge)
    IF_ID_Reg  if_id_current,  if_id_next;
    ID_EX_Reg  id_ex_current,  id_ex_next;
    EX_MEM_Reg ex_mem_current, ex_mem_next;
    MEM_WB_Reg mem_wb_current, mem_wb_next;

public:
    Pipelined_CPU() { 
        pc = 0; 
        if_id_current  = {0, 0};      if_id_next  = {0, 0};
        id_ex_current  = {0, 0, 0, 0, 0, 0}; id_ex_next  = {0, 0, 0, 0, 0, 0};
        ex_mem_current = {0, 0, 0};   ex_mem_next = {0, 0, 0};
        mem_wb_current = {0, 0};      mem_wb_next = {0, 0};
    }

    // Helper functions for the testbench
    void load_data(uint8_t reg, uint32_t val) { regs.write_reg(reg, val); }
    void flash_rom(uint32_t address, uint32_t instruction) { i_mem.load_program(address, instruction); }

    void tick() {
        // ==========================================
        // 5. WRITEBACK (WB)
        // ==========================================
        uint8_t  wb_dest = mem_wb_current.dest_reg;
        uint32_t wb_data = mem_wb_current.final_data;
        
        if (wb_dest != 0) { // Don't write to Reg 0 (common RISC architecture rule)
            regs.write_reg(wb_dest, wb_data);
             cout << "WB Stage  -> Wrote " << wb_data << " to Reg " << (int)wb_dest << endl;
        }

        // ==========================================
        // 4. MEMORY (MEM)
        // ==========================================
        uint8_t  mem_opcode = ex_mem_current.opcode;
        uint8_t  mem_dest   = ex_mem_current.dest_reg;
        uint32_t mem_alu    = ex_mem_current.alu_out;
        
        uint32_t final_data_to_write = 0;

        if (mem_opcode == 6) { 
            // LDR: Load Data from RAM
            final_data_to_write = d_mem.read_mem(mem_alu);
        } else {
            // Standard ALU Math
            final_data_to_write = mem_alu;
        }

        mem_wb_next.dest_reg   = mem_dest;
        mem_wb_next.final_data = final_data_to_write;

        // ==========================================
        // 3. EXECUTE (EX)
        // ==========================================
        uint8_t ex_opcode = id_ex_current.opcode;
        uint32_t val_a    = id_ex_current.val_a;
        uint32_t val_b    = id_ex_current.val_b;
        uint8_t ex_dest   = id_ex_current.dest_reg;

        // ==========================================
        // FORWARDING UNIT (Data Hazard Bypass)
        // ==========================================
        
        // 1. Check Source A
        if (ex_mem_current.dest_reg != 0) {
            // Does the address MEM is writing to match the address EX needs?
            if (ex_mem_current.dest_reg == id_ex_current.src_a) {
                // Hazard detected! Overwrite the stale 'val_a' with the fresh math answer.
                val_a = ex_mem_current.alu_out; 
                cout << "  [FORWARDING] Snagged " << val_a << " for Source A!" << endl;
            }
        }

        // 2. Check Source B
        if (ex_mem_current.dest_reg != 0) {
            // Does the address MEM is writing to match the address EX needs?
            if (ex_mem_current.dest_reg == id_ex_current.src_b) {
                // Hazard detected! Overwrite the stale 'val_b' with the fresh math answer.
                val_b = ex_mem_current.alu_out; 
                cout << "  [FORWARDING] Snagged " << val_b << " for Source B!" << endl;
            }
        }      

        uint32_t alu_result = alu.execute(ex_opcode, val_a, val_b);

        if (ex_opcode == 0 || ex_opcode == 1) 
            cout << "  [EX] ALU calculated: " << alu_result << endl;

        ex_mem_next.alu_out  = alu_result;
        ex_mem_next.opcode   = ex_opcode;
        ex_mem_next.dest_reg = ex_dest;

        // ==========================================
        // 2. INSTRUCTION DECODE (ID)
        // ==========================================
        uint32_t raw_instruction = if_id_current.instruction;
        
        uint8_t id_opcode = (raw_instruction >> 24) & 0xFF;
        uint8_t id_dest   = (raw_instruction >> 16) & 0xFF;
        uint8_t src_a     = (raw_instruction >> 8)  & 0xFF;
        uint8_t src_b     = (raw_instruction >> 0)  & 0xFF;

        uint32_t read_a = regs.read_reg(src_a);
        uint32_t read_b = regs.read_reg(src_b);


        id_ex_next.opcode   = id_opcode;
        id_ex_next.dest_reg = id_dest;
        id_ex_next.val_a    = read_a;
        id_ex_next.val_b    = read_b;
        id_ex_next.src_a = src_a; // Pass the address forward!
        id_ex_next.src_b = src_b;
        // ==========================================
        // 1. INSTRUCTION FETCH (IF)
        // ==========================================
        uint32_t fetched_instruction = i_mem.fetch(pc);
        
        if_id_next.instruction = fetched_instruction;
        if_id_next.pc = pc; 
        
        pc++; // Increment for the next cycle

        // ==========================================
        // CLOCK EDGE: Update Latches
        // ==========================================
        mem_wb_current = mem_wb_next;
        ex_mem_current = ex_mem_next;
        id_ex_current  = id_ex_next;
        if_id_current  = if_id_next;

    }

    void run(int cycles) {
        for (int i = 0; i < cycles; i++) {
            cout << "\n--- Cycle " << i + 1 << " ---" << endl;
            tick();
        }
    }
};
int main() {
    Pipelined_CPU my_cpu;
    cout << "--- Day 8: Triggering a Data Hazard ---" << endl;

    // 1. Pre-load Reg 2 and Reg 3
    my_cpu.load_data(2, 10);
    my_cpu.load_data(3, 20);
    my_cpu.load_data(5, 5);

    // 2. Load the HAZARDOUS instructions
    
    // PC 0: ADD Reg 1, Reg 2, Reg 3  (10 + 20 = 30) -> Saves to R1
    my_cpu.flash_rom(0, 0x00010203); 

    // PC 1: SUB Reg 4, Reg 1, Reg 5  (Should be 30 - 5 = 25)
    // HAZARD: It will read R1 before the ADD finishes writing to it!
    my_cpu.flash_rom(1, 0x01040105);

    // Run 7 cycles
    my_cpu.run(7);

    return 0;
}
;
