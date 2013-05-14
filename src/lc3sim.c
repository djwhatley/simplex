#include <stdio.h>
#include <stdlib.h>
#include "../include/lc3sim.h"

static FILE* file;

void setcc(short writeval)
{
	if (writeval < 0)
		cc = -1;
	else if (writeval == 0)
		cc = 0;
	else
		cc = 1;
}

char comparenzp(char nzp)
{
	if (nzp == 7)
		return 1;
	else if (nzp == 6 && cc <= 0)
		return 1;
	else if (nzp == 5 && cc != 0)
		return 1;
	else if (nzp == 4 && cc < 0)
		return 1;
	else if (nzp == 3 && cc >= 0)
		return 1;
	else if (nzp == 2 && cc == 0)
		return 1;
	else if (nzp == 1 && cc > 0)
		return 1;
	return 0;
}

short signext(char value, char bits)
{
	short mask1 = 1 << bits;
	short mask2 = (short) 0xFFFF << bits;
	char ret = 0;
	if ((short)value & mask1)
		ret = (short) mask2 | value;
	else
		ret = (short) ret | value;
}

short fetch_instruction()
{
	ir = mem[pc];
	pc++;
	return ir;
}

void decode_instruction(lc3inst_t* instruction, short raw_inst)
{
	instruction->opcode = (raw_inst & OPCODE_MASK) >> OPCODE_SHFT;
	instruction->nzpbits = (raw_inst & NZP_MASK) >> NZP_SHFT;
	instruction->destreg = (raw_inst & DEST_MASK) >> DEST_SHFT;
	instruction->src1reg = (raw_inst & SRC1_MASK) >> SRC1_SHFT;
	instruction->src2reg = (raw_inst & SRC2_MASK) >> SRC2_SHFT;
	instruction->imm5 = signext((raw_inst & IMM5_MASK) >> IMM5_SHFT, 4);
	instruction->pcoffset9 = signext((raw_inst & PC9_MASK) >> PC9_SHFT, 8);
	instruction->pcoffset11 = signext((raw_inst & PC11_MASK) >> PC11_SHFT, 10);
	instruction->offset6 = signext((raw_inst & OFF6_MASK) >> OFF6_SHFT, 5);
	instruction->trapvect = (raw_inst & TRAP_MASK) >> TRAP_SHFT;
	instruction->jsrr_flag = (raw_inst & JSRR_MASK) >> JSRR_SHFT;
	instruction->imm5_flag = (raw_inst & IMMF_MASK) >> IMMF_SHFT;
}

void execute_instruction(lc3inst_t* instruction)
{
	short old_pc;
	switch (instruction->opcode) {
	case BR:
		printf("BRANCH\n");
		if (comparenzp(instruction->nzpbits))
			pc += instruction->pcoffset9;
		break;
	case ADD:
		if (instruction->imm5_flag)
			regfile[instruction->destreg] = regfile[instruction->src1reg] + instruction->imm5;
		else
			regfile[instruction->destreg] = regfile[instruction->src1reg] + regfile[instruction->src2reg];
		setcc(regfile[instruction->destreg]);
		break;
	case LD:
		regfile[instruction->destreg] = mem[pc+instruction->pcoffset9];
		setcc(regfile[instruction->destreg]);
		break;
	case ST:
		mem[pc+instruction->pcoffset9] = regfile[instruction->destreg];
		break;
	case JSR:
		old_pc = pc;
		if (instruction->jsrr_flag)
			pc = mem[regfile[instruction->src1reg]];
		else
			pc = mem[pc+instruction->pcoffset11];
		regfile[7] = old_pc;
		break;
	case AND:
		if (instruction->imm5_flag)
			regfile[instruction->destreg] = regfile[instruction->src1reg] & instruction->imm5;
		else
			regfile[instruction->destreg] = regfile[instruction->src1reg] & regfile[instruction->src2reg];
		setcc(regfile[instruction->destreg]);
		break;
	case LDR:
		regfile[instruction->destreg] = mem[regfile[instruction->src1reg] + instruction->offset6];
		setcc(regfile[instruction->destreg]);
		break;
	case STR:
		mem[regfile[instruction->src1reg] + instruction->offset6] = regfile[instruction->destreg];
		break;
	case RTI:
		break;
	case NOT:
		regfile[instruction->destreg] = ~regfile[instruction->src1reg];
		setcc(regfile[instruction->destreg]);
		break;
	case LDI:
		regfile[instruction->destreg] = mem[mem[pc+instruction->pcoffset9]];
		setcc(regfile[instruction->destreg]);
		break;
	case STI:
		mem[mem[pc+instruction->pcoffset9]] = regfile[instruction->destreg];
		break;
	case JMP:
		old_pc = pc;
		pc = regfile[instruction->src1reg];
		regfile[7] = old_pc;
		break;
	case LEA:
		regfile[instruction->destreg] = pc + instruction->pcoffset9;
		setcc(regfile[instruction->destreg]);
		break;
	case TRAP:
		switch (instruction->trapvect) {
		case 0x20:
			break;
		case 0x25:
			running = 0;
			break;
		default:
			old_pc = pc;
			pc = instruction->trapvect;
			regfile[7] = old_pc;
			break;
		}
	}
}

void show_register_contents()
{
	printf("Register contents:\n"
				 "R0: x%.4hx\tR1: x%.4hx\n"
				 "R2: x%.4hx\tR3: x%.4hx\n"
				 "R4: x%.4hx\tR5: x%.4hx\n"
				 "R6: x%.4hx\tR7: x%.4hx\n",
				 regfile[0], regfile[1],
				 regfile[2], regfile[3],
				 regfile[4], regfile[5],
				 regfile[6], regfile[7]
			);
	printf("PC: x%.4hx\n", pc);
}

void read_program(FILE* program)
{
	unsigned short a = 1;
	unsigned short address;
	unsigned char num = 0;
	unsigned short b;

	while(a != 0xffff)
	{
		a = ((fgetc(program) << 8) | fgetc(program));
		if (!num)
		{
			address = a;
			a = ((fgetc(program) << 8) | fgetc(program));
			num = a;
			b = address + num;
		} else {
			mem[b-num] = a;
			num--;
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Bad arguments! Just give me a filename of an assembled LC-3 program!\n");
		exit(-1);
	}

	FILE* program = fopen(argv[1], "r");
	if (!program)
	{
		printf("File not found!\n");
		exit(-1);
	}

	read_program(program);

	running = 1;
	pc = 0x3000;

	short next;
	lc3inst_t instruction;
	while (running)
	{
		next = fetch_instruction();
		printf("x%.4hx\n", next);
		decode_instruction(&instruction, next);
		execute_instruction(&instruction);
	}

	show_register_contents();

	return 0;
}