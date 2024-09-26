#include <stdio.h>
#include "MemoryStore.h"
#include "RegisterInfo.h"
#include "EndianHelpers.h"
#include <fstream>
#include <iostream>
using namespace std; 

union REGS 
{
    RegisterInfo reg;
    uint32_t registers[32] {0};
};

union REGS regData;
// TODO: fill in the missing hex values of the OP_IDs (opcodes)
enum OP_IDS
{
    //R-type opcodes...
    OP_ZERO = 0x0, // zero
    //I-type opcodes...
    OP_ADDI = 0x8, // addi
    OP_ADDIU = 0x9, // addiu
    OP_ANDI = 0xc, // andi
    OP_BEQ = 0x4, // beq
    OP_BNE = 0x5, // bne
    OP_LBU = 0x18, // lbu
    OP_LHU = 0x19, // lhu
    OP_LUI = 0xf, // lui
    OP_LW = 0x17, // lw
    OP_ORI = 0xd, // ori
    OP_SLTI = 0xa, // slti
    OP_SLTIU = 0xb, // sltiu
    OP_SB = 0x1c, // sb
    OP_SH = 0x1d, // sh
    OP_SW = 0x2b, // sw
    OP_BLEZ = 0x6, // blez
    OP_BGTZ = 0x7, // bgtz
    //J-type opcodes...
    OP_J = 0x2, // j
    OP_JAL = 0x3 // jal
};

// TODO: fill in the missing hex values of FUNCT_IDs (function IDs)
enum FUNCT_IDS
{
    FUN_ADD = 0x20, // add
    FUN_ADDU = 0x21, // add unsigned (addu)
    FUN_AND = 0x24, // and
    FUN_JR = 0x8, // jump register (jr)
    FUN_NOR = 0x1b, // nor
    FUN_OR = 0x19, // or
    FUN_SLT = 0x2a, // set less than (slt)
    FUN_SLTU = 0x2b, // set less than unsigned (sltu)
    FUN_SLL = 0x00, // shift left logical (sll)
    FUN_SRL = 0x02, // shift right logical (srl)
    FUN_SUB = 0x16, // substract (sub)
    FUN_SUBU = 0x17  // substract unsigned (subu)
};

// extract specific bits [start, end] from a 32 bit instruction
uint extractBits(uint32_t instruction, int start, int end)
{
    int bitsToExtract = start - end + 1;
    uint32_t mask = (1 << bitsToExtract) - 1;
    uint32_t clipped = instruction >> end;
    return clipped & mask;
}

// sign extend smol to a 32 bit unsigned int
uint32_t signExt(uint16_t smol) 
{
    uint32_t x = smol;
    uint32_t extension = 0xffff0000;
    return (smol & 0x8000) ? x ^ extension : x;
}

// dump registers and memory
void dump(MemoryStore *myMem) {

    dumpRegisterState(regData.reg);
    dumpMemoryState(myMem);
}

int main(int argc, char** argv) {

    // open instruction file
    ifstream infile;
    infile.open(argv[1], ios::binary | ios::in);

    // get length of the file and read instruction file into a buffer
    infile.seekg(0, ios::end);
    int length = infile.tellg();
    infile.seekg (0, ios::beg);

    char *buf = new char[length];
    infile.read(buf, length);
    infile.close();

    // initialize memory store with buffer contents
    MemoryStore *myMem = createMemoryStore();
    int memLength = length / sizeof(buf[0]);
    int i;
    for (i = 0; i < memLength; i++) {
        myMem->setMemValue(i * BYTE_SIZE, buf[i], BYTE_SIZE);
    }

    // initialize registers and our program counter
    regData.reg = {};
    uint32_t PC = 0;
    bool err = false;
    
    // variables to handle branch delay slot execution
    bool encounteredBranch = false;
    bool executedDelaySlot = false; 
    uint32_t savedBranch = 0;       // saved (delayed) branch instruction
    uint32_t savedPC = 0;           // PC when the branch wa encountered (PC for the instruction in memory after the branch instruction)
    
    // start simulation
    // TODO: complete simulation loop and implement branch delay logic
    while (!err) {
        // fetch current instruction
        uint32_t instruction;
        myMem->getMemValue(PC, instruction, WORD_SIZE);

        // increment PC & reset zero register
        PC += 4;
        regData.registers[0] = 0;

        // check for halt instruction
        if (instruction == 0xfeedfeed) {
            dump(myMem);
            return 0;
        }

        // TODO: parse instruction by completing function calls to extractBits()
        // and set operands accordingly
        uint32_t opcode = extractBits(instruction, 26 , 31);
        uint32_t rs = extractBits(instruction, 21, 25);
        uint32_t rt = extractBits(instruction, 16 , 20);
        uint32_t rd = extractBits(instruction, 11, 15 );
        uint32_t shamt = extractBits(instruction, 6 , 10);
        uint32_t funct = extractBits(instruction, 0 , 5 );
        uint16_t immediate = extractBits(instruction, 0 , 15 );
        uint32_t address = extractBits(instruction, 0 , 25 );

        // ASK ABOUT THESE 4
        int32_t signExtImm = signExt(immediate);
        uint32_t zeroExtImm = signExt(0);

        uint32_t branchAddr = extractBits(instruction, 0, 15);
        uint32_t jumpAddr = extractBits(instruction, 0, 25);  // assumes PC += 4 just happened

        switch(opcode) {
            case OP_ZERO: // R-type instruction 
                switch(funct) {
                    case FUN_ADD:                         
                    regData.registers[rd] = regData.registers[rs] + regData.registers[rt];
                    break;

                    case FUN_ADDU:
                    regData.registers[rt] = (u_int32_t) regData.registers[rs] + (u_int32_t) regData.registers[rt]; 
                    break;

                    case FUN_AND: 
                    regData.registers[rt] = regData.registers[rs] & regData.registers[rt]; 
                    break;

                    case FUN_JR: 
                    PC = regData.registers[rs];
                    break;

                    case FUN_NOR: 
                    regData.registers[rd] = ~(regData.registers[rs] | regData.registers[rt]);
                    break;

                    case FUN_OR: 
                    regData.registers[rd] = (regData.registers[rs] | regData.registers[rt]);
                    break;

                    case FUN_SLT: 
                    regData.registers[rd] = (regData.registers[rs] < regData.registers[rt]) ? 1 : 0;
                    break;

                    case FUN_SLTU: 
                    regData.registers[rd] = ((u_int32_t)regData.registers[rs] < (u_int32_t) regData.registers[rt]) ? 1 : 0;
                    break;

                    case FUN_SLL: 
                    regData.registers[rd] = (regData.registers[rs] << shamt);
                    break;

                    case FUN_SRL: 
                    regData.registers[rd] = (regData.registers[rs] >> shamt);
                    break;

                    case FUN_SUB:  
                    regData.registers[rd] = regData.registers[rs] - regData.registers[rt];
                    break;
                    
                    case FUN_SUBU: 
                    regData.registers[rd] = (u_int32_t)regData.registers[rs] - (u_int32_t)regData.registers[rt];
                    break;

                    default:
                        fprintf(stderr, "\tIllegal operation...\n");
                        err = true;
                }
                break;

            case OP_ADDI: 
                regData.registers[rt] = regData.registers[rs] + signExtImm;
                break;
            case OP_ADDIU: 
                regData.registers[rt] = (u_int32_t) regData.registers[rs] + (u_int32_t) signExtImm;
                break;
            case OP_ANDI: 
                regData.registers[rd] = regData.registers[rs] & regData.registers[rs];
                break;

            case OP_BEQ: 
                if (regData.registers[rs] == regData.registers[rt]) {
                    PC = PC + 4 + branchAddr;
                }
                break;
                
            case OP_BNE:
                 if (regData.registers[rs] != regData.registers[rt]) {
                    PC = PC + 4 + branchAddr;
                }
                break;
                
            case OP_BLEZ: 
                if (regData.registers[rs] <= 0){
                PC = PC + 4 + branchAddr;
            }
            break;
                
            case OP_BGTZ: 
                if (regData.registers[rs] >= 0){
                PC = PC + 4 + branchAddr;
            }
            break;
                
            case OP_J: 
                PC = jumpAddr; //Shouldn't need the Pc + 4 since you have that assumption for jumpAddr
                break;
                
            case OP_JAL: 
                regData.registers[31] = PC + 8;
                PC = jumpAddr;
                break;

            // Ask About Load and Store   
            case OP_LBU: // ASK ABOUT
                u_int32_t value;
                myMem->getMemValue(regData.registers[rs] + signExtImm, value, BYTE_SIZE);
                regData.registers[rt] = extractBits(value, 0, 7);

            case OP_LHU:  
                u_int32_t value;
                myMem->getMemValue(regData.registers[rs] + signExtImm, value, HALF_SIZE);
                regData.registers[rt] = extractBits(value, 0, 15);

            case OP_LUI: 
                regData.registers[rt] = (int32_t) immediate;

            case OP_LW: 
                u_int32_t value;
                myMem->getMemValue(regData.registers[rs] + signExtImm, value, WORD_SIZE);
                regData.registers[rt] = (int32_t) value;

            case OP_ORI: 
                    regData.registers[rt] = regData.registers[rs] | zeroExtImm;
                
            case OP_SLTI: 
                    regData.registers[rt] = (regData.registers[rs] < signExtImm) ? 1:0;
                
            case OP_SLTIU: 
                    regData.registers[rt] = (regData.registers[rs] < (u_int16_t) signExtImm) ? 1:0;
                    
// Should value be signed or unsigned or does it not matter
            case OP_SB: 
                (int32_t) value;
                value = extractBits(regData.registers[rt], 0, 7);
                myMem->setMemValue(regData.registers[rs] + signExtImm, value, BYTE_SIZE);

            case OP_SH: 
                (int32_t) value;
                value = extractBits(regData.registers[rt], 0, 15);
                myMem->setMemValue(regData.registers[rs] + signExtImm, value, HALF_SIZE);
            case OP_SW: 
                u_int32_t value;
                value = regData.registers[rt];
                myMem->setMemValue(regData.registers[rs] + signExtImm, value, WORD_SIZE);

            default:
                fprintf(stderr, "\tIllegal operation...\n");
                err = true;
        }
    }

    // dump and exit with error
    dump(myMem);
    exit(127);
    return -1;  
}



