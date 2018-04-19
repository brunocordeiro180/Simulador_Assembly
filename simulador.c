// 	BRUNO CORDEIRO MENDES 150007094 SIMULADOR MIPS TRABALHO 2
// 	Compilador: gcc
// 	Ambiente: Linux 64x 8GB de RAM
// 	Editor de texto utilizado: Sublime Text 3.1.1
// 	- Para executar o arquivo digite no terminal:
// 		gcc simulador.c -o simulador
// 		./simulador  

#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#define MEM_SIZE 4096 //cada arquivo tem 2048 valores, juntando os dois da 4096 palavras
int32_t mem[MEM_SIZE];  //alocando espaço para memória do mips
uint32_t PC;            // program counter
uint32_t R[32];         //banco de registradores
int opcode, rs, rt, rd, shamt,funct;   //campos de instruçao do mips
int16_t imm;                           //imediato usado para quando op != 0x00
uint32_t hi, lo, reg, ri;     //hi -> 32 maiores bits da mult, lo-> 32 menore, reg-> endereço para jump
                              //ri->registrador de instrução
void fetch();         //	fetch() lê uma instrução da memória e coloca-a em ri, atualizando o
                      //pc para apontar para a próxima instrução
void decode();        //decodifica instrução
void execute();       //executa instrução
void step();          //realiza fetch->decode->execute
void run();           //roda programa ate encontrar chamada para encerramento ou até o pc ultrapassar o limite do segmento de código
int32_t lw(uint32_t address, int16_t kte);  //lê uma palavra
int32_t lh(uint32_t address, int16_t kte);  //lê meia palavra
int32_t lhu(uint32_t address, int16_t kte); //lê meia palavra sem overflow
int32_t lb(uint32_t address, int16_t kte);  //lê byte
int32_t lbu(uint32_t address, int16_t kte); //lê byte sem overflow
void sw(uint32_t address, int16_t kte, int32_t dado); //guarda palavra na memoria
void sh(uint32_t address, int16_t kte, int16_t dado); //guarda meia palavra
void sb(uint32_t address, int16_t kte, int8_t dado);   //guarda byte na memoria
void menu();

enum OPCODES	{ 	 //	so	sao	considerados	os	6	primeiros	bits	dessas	constantes

	EXT=0x00,	LH=0x21, 		SB=0x28, 			BLEZ=0x06,			SLTIU=0x0B,
	LHU=0x25,	SH=0x29,		BGTZ=0x07, 			ANDI=0x0C,			JAL=0x03,
	BEQ=0x04,	ADDI=0x08,		ORI=0x0D,			J=0x02,				LW=0x23,
	LBU=0x24,	SW=0x2B,		LB=0x20, 			LUI=0x0F,			ADDIU=0x09,
	BNE=0x05,	SLTI=0x0A,		XORI=0x0E
};

enum FUNCT{
	ADD=0x20,		OR=0x25, 		SLL=0x00,	SUB=0x22,	XOR=0x26,	SRL=0x02,
	MULT=0x18,		NOR=0x27,		SRA=0x03,	DIV=0x1A,	AND=0x24,	SLT=0x2A,
	JR=0x08,		SYSCALL=0x0c,	MFHI=0x10,	MFLO=0x12,	ADDU=0x21
};

void fetch(){       //guarda instrução no ri e incrementa PC

	ri = lw(PC, 0);
    PC += 4; //update program counter
}

void decode(){
	opcode = (ri & 0xFC000000) >> 26; //pega 6 primeiros bit's
	rs = (ri & 0x03E00000) >> 21;     //pega 5 bits
	rt = (ri & 0x001F0000) >> 16;     //pega 5 bits
	rd = (ri & 0x0000F800) >> 11;      //pega 5 bits
	shamt = (ri & 0x000007C0) >> 6;    //pega 5 bits
	funct = (ri & 0x0000003F);         //pega 6 ultimos bits
	imm = (int16_t)(ri & 0x0000FFFF);   //pega 16 ultimos bits
	reg = (ri & 0x03FFFFFF) << 2;       //pega 26 ultimos bits e utiliza shift
}

void execute(){
	switch(opcode){
		case ((EXT  & 0x3F)): //pega somente 6 bits mais significativos do opcode
		switch (funct){
			case (ADD  & 0x3F):  //soma com overflow
			R[rd] = R[rs] + R[rt];
			break;

			case (ADDU  & 0x3F): //soma sem overflow
			R[rd] = (uint32_t)((uint32_t)R[rs] + (uint32_t)R[rt]);
			break;

			case(OR & 0x3F):     //bitwise OR entre $rs e $rt
			R[rd] = R[rs] | R[rt];
			break;

			case(SUB & 0x3F):    //subtração entre $rs e $rt
			R[rd] = R[rs] - R[rt];
			break;

			case (MULT & 0x3F): ;   //multiplicação
			uint64_t produto;
			produto = R[rs] * R[rt];
			lo = (produto & 0x00000000FFFFFFFF);
			hi = produto >> 32;
			break;

			case (DIV & 0x3F):   //divisão
			hi = R[rs]/R[rt];
			lo = R[rs] % R[rt];
			break;

			case (SLL & 0x3F):   //shift left utilizando shamt para o deslocamento
			R[rd] = R[rt] << shamt;
			break;

			case (XOR & 0x3F):   //bitwise XOR entre $rs e $rt
			R[rd] = R[rt] ^ R[rs];
			break;

			case (SRL & 0x3F):   //shift right utilizando shamt para o deslocamento
			R[rd] = R[rt] >> shamt;
			break;

			case(NOR & 0x3F):  //bitwise NOR entre $rs e $rt
			R[rd] = ~(R[rt] | R[rs]);
			break;

			case(SRA & 0x3F):	//shift right arithmetic
			R[rd] = (int16_t)(R[rt] >> shamt);
			break;

			case (AND & 0x3F):	//bitwise AND entre $rt e $rs
			R[rd] = R[rt] & R[rs];
			break;

			case (SLT & 0x3F):	//set $rd to 1 if $rs less than $rt
			if (R[rs] < R[rt])
			{
				R[rd] = 1;
			}
			else{
				R[rd] = 0;
			}
			break;

			case (JR & 0x3F):	//jump registrer
			PC = R[rs];
			break;

			case (MFHI & 0x3F):  //move from hi register
			R[rd] = hi;
			break;

			case (MFLO & 0x3F):	 //move from lo register
			R[rd] = lo;
			break;

			case (SYSCALL & 0x3F): 	//realiza print
			if(R[2] == 10){		//se for chamada para encerramento de funcão
				printf("\n-- program is finished running --\n");
				menu();
			}
			if (R[2] == 1)		//chamada para impressão de inteiro
			{
				printf("%i", R[4]);	
			}
			if (R[2] == 4){		//chamada para impressão de string
				uint32_t endereco;
				int i = 0;
				while(mem[(R[4])/4] != 0x00){ //posicao + 4 = posição do primeira letra
					endereco = R[4]/4;		
					R[4] += 4;
					printf("%c", mem[endereco] & 0x000000FF);
					printf("%c", (mem[endereco] & 0x0000FF00) >> 8);
					printf("%c", (mem[endereco] & 0x00FF0000) >> 16);
					printf("%c", (mem[endereco] & 0xFF000000) >> 24);
					if ((mem[endereco] & 0x000000FF) == 0x00 && i > 0)
					{
						break;
					}
					if (((mem[endereco] & 0x0000FF00) >> 8 )== 0x00 && i > 0)
					{
						break;
					}
					if (((mem[endereco] & 0x00FF0000) >> 16) == 0x00 && i > 0)
					{
						break;
					}
					if (((mem[endereco] & 0xFF000000) >> 24) == 0x00 && i > 0)
					{
						break;
					}
					i++;			
				
				}
			}
			if(R[2] == 5)
			{	
				scanf("%i", &R[2]);

			}
			break;
		}
		break;
		case (LW  & 0x3F): //Load Word $rt, $rs, imm
		R[rt] = lw(R[rs], imm);
		break;

		case (LH  & 0x3F):	//Load Half Word $rt, $rs, imm
		R[rt] = lh(R[rs], imm);
		break;

		case (LB & 0x3F):	//Load Byte $rt, $rs, imm
		R[rt] = lb(R[rs], imm);
		break;

		case(SW & 0x3F):	//Store Word $rt, $rs, imm
		sw(R[rs], imm, R[rt]);
		break;

		case(SH & 0x3F):	//Store Half Word $rt, $rs, imm
		sh(R[rs], imm, R[rt]);
		break;

		case(SB & 0x3F):	//Store Byte $rt, $rs, imm
		sb(R[rs], imm, R[rt]);
		break;

		case (BLEZ & 0x3F):	//Brach less then or equal zero
		if(R[rs] <= R[0]){
			PC = imm;
		}
		break;

		case (SLTIU & 0x3F):	//set if less than immediate unsigned
		if ((uint16_t)(R[rs] < imm)){
			R[rt] = 1;
		}
		else{
			R[rt] = 0;
		}
		break;

		case (LHU & 0x3F):		//load unsigned int
		R[rt] = lhu(R[rs], imm);
		break;

		case (BGTZ & 0x3F):		//branch if greater then zero
		if(R[rs] > R[0]){
			PC = imm;
		}
		break;

		case(ANDI & 0x3F):		//bitwise AND $rs and immediate
		R[rt] = R[rs] & imm;
		break;

		case (JAL & 0x3F):		//jump and link
		R[31] = PC;
		PC = reg;
		break;

		case (J & 0x3F):		//jump to adress
		PC = reg;
		break;

		case (BEQ & 0x3F):		//branch if equal
		if (R[rs] == R[rt])
		{
			PC = PC  + (imm << 2);
		}
		break;

		case (BNE & 0x3F):		//branch if not equal
		if(R[rs] != R[rt]){
			PC = PC  + (imm << 2);
		}
		break;

		case (ADDI & 0x3F):		//addi $rt, $rs, imm
		R[rt] = R[rs] + imm;

		break;

		case (ADDIU & 0x3F):	//add with unsigned immediate
		R[rt] = (uint32_t)(R[rs] + imm);
		break;

		case (ORI & 0x3F):		//bitwise OR with immediate
		R[rt] = R[rs] | imm;

		break;

		case (LUI & 0x3F): //16 bits mais significativos de $rt serao iguais imediato
		R[rt] =	R[rt] | (((uint32_t)(imm) << 16) & 0xFFFF0000);

		break;

		case (SLTI & 0x3F):	//set less than immediate
		if((uint32_t)(R[rt] < imm)){
			R[rs] = 1;
		}
		else{
			R[rs] = 0;
		}
		break;

		case (XORI & 0x3F):	//bitwise XORI $rs and immediate
		R[rt] = R[rs] ^ imm;
		break;

	}
}

void step(){
	fetch();
	decode();
	execute();
}

void run(){
	step();
	if(PC <= 4096){
		run();
	}
	else{
		printf("\n -- program is finished running (dropped off bottom) --\n");
	}
}

int32_t lw(uint32_t address, int16_t kte){

	if ((address + kte) % 4 != 0 && (address + kte) != 0) //verifica se endereço é um multiplo de 4
	{
		printf("error lw\n");
		exit(-1);
	}
	else{
		kte = (int32_t)kte;
		address = (address + kte)/4;
	}

	return mem[address];
}

int32_t lh(uint32_t address, int16_t kte){
	if ((address + kte) % 2 != 0) //verifica se endereço é um multiplo de 2
	{
		printf("error lh\n");
		exit(-1);
	}
	else{
		int resto =	(address + kte) % 4;  
		address = (address + kte)/4;
		int32_t value;
		switch (resto){
			case 0:
			value = mem[address] & 0x0000FFFF;
			break;
			case 1:
			value = (mem[address] & 0xFFFF0000) >> 16;
		}

		return value;
	}
}

int32_t lhu(uint32_t address, int16_t kte){
	if ((address + kte) % 2 != 0) //verifica se endereço é um multiplo de 2
	{
		printf("error lh\n");
		exit(-1);
	}
	else{
		int resto =	(address + kte) % 4;  
		address = (address + kte)/4;
		uint32_t value;
		switch (resto){
			case 0:
			value = (uint32_t)(mem[address] & 0x0000FFFF);
			break;
			case 1:
			value = (uint32_t)((mem[address] & 0xFFFF0000) >> 16);
		}

		return value;
	}
}

int32_t lb(uint32_t address, int16_t kte){
	int posicao;
	int8_t byte;
	uint32_t value;
	byte = (address + kte) % 4;
	address = (address + kte)/4;
	value = mem[address + kte];
	switch(byte){
		case 0:
		value = (int16_t)(value & 0x000000FF);
		case 1:
		value = (int16_t)((value & 0x0000FF00) >> 8);
		case 2:
		value = (int16_t)((value & 0x00FF0000) >> 16);
		case 3:
		value = (int16_t)((value & 0xFF000000) >> 24);
	}

	return value;
}

int32_t lbu(uint32_t address, int16_t kte){
	int posicao;
	int8_t byte; //byte da palavra que sera lido
	uint32_t value;
	byte = (address + kte) % 4;
	address = (address + kte)/4;
	value = mem[address + kte];
	switch(byte){
		case 0:
		value = (uint8_t)(value & 0x000000FF);
		case 1:
		value = (uint8_t)((value & 0x0000FF00) >> 8);
		case 2:
		value = (uint8_t)((value & 0x00FF0000) >> 16);
		case 3:
		value = (uint8_t)((value & 0xFF000000) >> 24);
	}

	return value;
}

void sw(uint32_t address, int16_t kte, int32_t dado){
	if ((address + kte) % 4 != 0 && (address + kte) != 0) //verifica se endereço é um multiplo de 4
	{
		printf("error sw\n");
		exit(-1);
	}
	else{
		kte = (int32_t)kte;
		address = (address + kte)/4;
		mem[address] = dado;
	}
}

void sh(uint32_t address, int16_t kte, int16_t dado){
	if ((address + kte) % 2 != 0) //verifica se endereço é um multiplo de 2
	{
		printf("error sh\n");
		exit(-1);
	}
	else{
		int resto =	(address + kte) % 4;  
		address = (address + kte)/4;
		int32_t value;
		switch (resto){
			case 0:
			value = (mem[address] & 0xFFFF0000) | (uint32_t)dado;
			break;
			case 1:
			value = (mem[address] & 0x0000FFFF) | ((uint32_t)dado << 16);
		}
		mem[address] = value;
	}
}

void sb(uint32_t address, int16_t kte, int8_t dado){
	int posicao;
	int32_t value;
	int8_t byte;
	address = (address + kte)/4;
	byte = (address + kte) % 4; //resto da divisao indica posicao do byte
	value = mem[address + kte];
	switch(byte){
		case 0:
		value = (value & 0x000000FF) | dado;
		break;

		case 1:
		value = (value & 0x0000FF00) | dado;
		break;

		case 2:
		value = (value & 0x00FF0000) | dado;
		break;

		case 3:
		value = (value & 0xFF000000) | dado;
		break;

	}
	mem[address + kte] = value;

}

void dump_mem(int start, int end, char format){ //imprime conteúdo da memoria
	for (int i = start; i < end; i++)
	{
		if (format == 'h')
		{
			printf("%x    ", mem[i]);
		}
		if (format == 'i')
		{
			printf("%4i ", mem[i]);
		}
	}
	printf("\n");
}

void dump_reg(char format){		//imprime banco de registradores
	for (int i = 0; i < 32; ++i)
	{
		if (format == 'h')
		{
			printf("%8x", R[i]);
		}
		if (format == 'i')
		{
			printf("%4i", R[i]);
		}
	}
	printf("\n");
	if (format == 'h')
	{
		printf("PC %8x\n", PC);
		printf("HI %8x\n", hi);
		printf("LO %8x\n", lo);
	}
	if (format == 'i')
	{
		printf("PC %4i\n", PC);
		printf("HI %4i\n", hi);
		printf("LO %4i\n", lo);
	}
}

void menu(){
	int i = 0, num = 0, inicio, fim;
	char formato;
	do  	//menu iterativo
	{
		printf("\nDigite forma de impressão\n");
		printf("1: dump_mem\n");
		printf("2: dump_reg\n");

		printf("0: Sair do programa\n");

		scanf("%d", &num);
		system("cls || clear");

		if(num == 0){
			break;
		}
		else{
			switch(num){
				case 1:
				printf("Digite inicio e fim e formato\n");
				scanf("%i %i %c" , &inicio, &fim, &formato);
				dump_mem(inicio, fim, formato);
		

				break;
				case 2:
				printf("Digite formato(h para hexadecimal, i para inteiro)\n");
				scanf("%c" , &formato);
				dump_reg(formato);
				

				break;

				case 0:
				exit(-1);
				break;

				default:
				exit(-1);
				break;

			}
		}

	} while (num != 0);
	exit(-1);
}

int main()
{
	FILE *arq1, *arq2; //arquivo1 sendo text e arquivo2 o data

	int i = 0;
	PC = 0;
	char formato;
	R[29] = 0x00003ffc; //Set $sp com o endereço da pilha

	arq2 = fopen("text", "rb"); //abertura de arquivo binario

	if (arq2 == NULL){
		printf("Nao foi possivel abrir arquivo\n");
		return 0;
	}
	else{
		for (; i < 2048; i++)
		{
			fread(&mem[i], 1, 4, arq2);
		}
	}

	// Abre um arquivo BINÁRIO para LEITURA
	arq1 = fopen("data", "rb");
	if (arq1 == NULL)  // Se houve erro na abertura
	{
		printf("Problemas na abertura do arquivo\n");
		return 0;
	}
	else{
		for(i = 2048; feof(arq1) == 0; i++)
		{
			fread(&mem[i], 1, 4, arq1);
		}
	}

	run();

	return 0;

}
