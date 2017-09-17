/***********************************************************************
 *
 *		ERROR.C
 *		Error message printer for 68000 Assembler
 *
 *    Function: printError()
 *		Prints an appropriate message to the specified output
 *		file according to the error code supplied. If the
 *		errorCode is OK, no message is printed; otherwise an
 *		WARNING or ERROR message is produced. The line number
 *		will be included in the message unless lineNum = -1.
 *
 *	 Usage:	printError(outFile, errorCode, lineNum)
 *		FILE *outFile;
 *		int errorCode, lineNum;
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/12/86
 *
 ************************************************************************/


#include <stdio.h>
#include "asm.h"

int     printError(FILE *outFile, int errorCode, int lineNum)
{
	char    numBuf[20];

	if (errorCode && lineNum >= 0) {
		fprintf(outFile, "%s %d : ", fileName, lineNum);
	}

	switch (errorCode) {
	case SYNTAX:
		fprintf(outFile, "ERROR : Invalid syntax\n");
		break;
	case UNDEFINED:
		fprintf(outFile, "ERROR : Undefined symbol\n");
		break;
	case DIV_BY_ZERO:
		fprintf(outFile, "ERROR : Division by zero attempted\n");
		break;
	case NUMBER_TOO_BIG:
		fprintf(outFile, "WARNING : Numeric constant exceeds 32 bits\n");
		break;
	case ASCII_TOO_BIG:
		fprintf(outFile, "WARNING : ASCII constant exceeds 4 characters\n");
		break;
	case INV_OPCODE:
		fprintf(outFile, "ERROR : Invalid opcode\n");
		break;
	case INV_SIZE_CODE:
		fprintf(outFile, "WARNING : Invalid size code\n");
		break;
	case INV_ADDR_MODE:
		fprintf(outFile, "ERROR : Invalid addressing mode\n");
		break;
	case MULTIPLE_DEFS:
		fprintf(outFile, "ERROR : Symbol multiply defined\n");
		break;
	case PHASE_ERROR:
		fprintf(outFile, "ERROR : Symbol value differs between first and second pass\n");
		break;
	case INV_QUICK_CONST:
		fprintf(outFile, "ERROR : MOVEQ instruction constant out of range\n");
		break;
	case INV_VECTOR_NUM:
		fprintf(outFile, "ERROR : Invalid vector number\n");
		break;
	case INV_BRANCH_DISP:
		fprintf(outFile, "ERROR : Branch instruction displacement is out of range or invalid\n");
		break;
	case LABEL_REQUIRED:
		fprintf(outFile, "ERROR : Label required with this directive\n");
		break;
	case INV_DISP:
		fprintf(outFile, "ERROR : Displacement out of range\n");
		break;
	case INV_ABS_ADDRESS:
		fprintf(outFile, "ERROR : Absolute address exceeds 16 bits\n");
		break;
	case INV_8_BIT_DATA:
		fprintf(outFile, "ERROR : Immediate data exceeds 8 bits\n");
		break;
	case INV_16_BIT_DATA:
		fprintf(outFile, "ERROR : Immediate data exceeds 16 bits\n");
		break;
	case INV_8_BIT_DATA_WARN:
		fprintf(outFile, "WARNING : Immediate data exceeds 8 bits\n");
		break;
	case INV_16_BIT_DATA_WARN:
		fprintf(outFile, "WARNING : Immediate data exceeds 16 bits\n");
		break;
	case INCOMPLETE:
		fprintf(outFile, "WARNING : Evaluation of expression could not be completed\n");
		break;
	case NOT_REG_LIST:
		fprintf(outFile, "ERROR : The symbol specified is not a register list symbol\n");
		break;
	case REG_LIST_SPEC:
		fprintf(outFile, "ERROR : Register list symbol used in an expression\n");
		break;
	case REG_LIST_UNDEF:
		fprintf(outFile, "ERROR : Register list symbol not previously defined\n");
		break;
	case INV_SHIFT_COUNT:
		fprintf(outFile, "ERROR : Invalid constant shift count\n");
		break;
	case INV_FORWARD_REF:
		fprintf(outFile, "ERROR : Forward references not allowed with this directive\n");
		break;
	case INV_LENGTH:
		fprintf(outFile, "ERROR : Block length is less that zero\n");
		break;
	case ODD_ADDRESS:
		fprintf(outFile, "ERROR : Origin value is odd (Location counter set to next highest address)\n");
		break;
	case WARN_NO_INPL:
		fprintf(outFile, "WARNING : no inplimention\n");
		break;
	default:
		if (errorCode > ERROR)
			fprintf(outFile, "ERROR : No message defined\n");
		else if (errorCode > WARNING)
			fprintf(outFile, "WARNING : No message defined\n");
	}

	return NORMAL;

}
