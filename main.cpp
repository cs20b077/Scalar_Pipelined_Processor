#include <bits/stdc++.h>
using namespace std;

/*
LAB 8
Biswadip Mandal (CS20B017)
Syed Hurmath Arshed (CS20B077)
*/

int PC = 0;
int IR = 0;
int cycles = 0;

int stalls = 0;
int controlStalls = 0;
int dataStalls = 0;

int dataHazard = 0;
int controlHazard = 0;

int CurrentInstructions = 0;
int InstructionCount = 0;
int ArithmeticInstCount = 0;
int LogicalInstCount = 0;
int ControlInstCount = 0;
int DataInstCount = 0;
int HaltInstCount = 0;

vector<int> registerFile(16, 0);
vector<string> instructionCache(256, "00");
vector<int> dataCache(256, 0);
vector<bool> valid(16, 1);


string DecTohex(int n)
{
    char hexaDeciNum[5];
    string s = "";
    int i = 0;
    while (n != 0)
    {
        int temp = 0;
        temp = n % 16;

        if (temp < 10)
            hexaDeciNum[i] = temp + 48;
        else
            hexaDeciNum[i] = temp + 87;

        i++;
        n = n / 16;
    }

    for (int j = i - 1; j >= 0; j--)
        s = s + hexaDeciNum[j];

    return s;
}

int hexToDec(string hexVal)
{
    int len = hexVal.size();
    int base = 1;
    int dec_val = 0;

    for (int i = len - 1; i >= 0; i--)
    {
        if (hexVal[i] >= '0' && hexVal[i] <= '9')
        {
            dec_val += (int(hexVal[i]) - 48) * base;
            base = base * 16;
        }
        else if (hexVal[i] >= 'a' && hexVal[i] <= 'f')
        {
            dec_val += (int(hexVal[i]) - 87) * base;
            base = base * 16;
        }
    }
    return dec_val;
}

class Instruction
{
public:
    int currentState = -1; // Stores 'k' if instruction has completed 'k' stages(if not started then stores -1)
    bool stalled = true;

    void start(int position)
    {
        this->position = position;
        currentState = 0;
        stalled = true;

        ALUOutput = 0;
        LMD = 0;
        A = 0;
        B = 0;

        instruction = "";
        hasStarted = true;
        ArithmI = false;
        LogicI = false;
        LoadStoreI = false;
        controlI = false;
        haltI = false;

        dest = 0;
        src1 = 0;
        src2 = 0;
        opcode = '0';

        next();
    }

    void flush()
    {
        currentState = -1;
        stalled = true;

        ALUOutput = 0;
        LMD = 0;
        A = 0;
        B = 0;

        instruction = "";
        hasStarted = true;
        ArithmI = false;
        LogicI = false;
        LoadStoreI = false;
        controlI = false;
        haltI = false;

        dest = 0;
        src1 = 0;
        src2 = 0;
        opcode = '0';
    }

    void next()
    {
        if (currentState == -1 || !hasStarted )
            return;

        currentState++;

        if (currentState == 3 && stalled)
        {     
            currentState = 2;
            Instruction_Decode_Cycle();       
            return;
        }

        switch (currentState)
        {
        case 1:
            Instruction_Fetch_Cycle();
            break;
        case 2:
            Instruction_Decode_Cycle();
            break;
        case 3:
            Execution_Cycle();
            break;
        case 4:
            Memory_Access_Cycle();
            break;
        case 5:
            Write_Back_Cycle();
            break;
        }
    }

private:
    int position;
    int ALUOutput;
    int LMD;
    int A;
    int B;
    int C;
    string instruction;
    bool hasStarted = false;

    bool ArithmI = false;
    bool LogicI = false;
    bool LoadStoreI = false;
    bool controlI = false;
    bool haltI = false;

    int dest, src1, src2;
    char opcode;

    void Instruction_Fetch_Cycle()
    {
        
        string byte1, byte2;
        byte1 = instructionCache[position];
        byte2 = instructionCache[position + 1];

        instruction = byte1 + byte2;
        IR = hexToDec(instruction);

        if (instruction != "0000")
        {
            CurrentInstructions++;
            InstructionCount++;
            PC += 2;
        }
        else {
            hasStarted = false;
        }
    }

    void Instruction_Decode_Cycle()
    {        
        opcode = instruction[0];
        dest = hexCharToDec(instruction[1]);
        src1 = hexCharToDec(instruction[2]);
        src2 = hexCharToDec(instruction[3]);

        string registerValue;

        if(opcode == 'a'){
            A = dest;
            B = src1;
        }
        else  A = registerFile[dest];

        if(opcode == 'b') {            
            B = src1;
            C = src2;
        }
        else if(opcode != 'a') B = registerFile[src1];

        if(opcode == '8' || opcode == '9') C = src2;
        else if(opcode != 'b') C = registerFile[src2];

        // 2 register operand in source
        if(opcode == '0' || opcode == '1' || opcode == '2' || opcode == '4' || opcode == '5' || opcode == '7') {
            if(!valid[src1] || !valid[src2]){
                stalls++;
                dataStalls++;
                dataHazard = 1;
                return;
            }
        }
         
        // 1 register operand in source
        else if(opcode == '6' || opcode == '8') {
            if(!valid[src1]){
                stalls++;
                dataStalls++;
                dataHazard = 1;
                return;
            }
        }


        // 2 register operand (1 source + 1 dest)
        else if(opcode == '9')                  // STORE 
        {
            C = src2;
            if(!valid[src1]){
                stalls++;
                dataStalls++;
                dataHazard = 1;
                return;
            }
        }

        //  1 register operand
        else if(opcode == '3' || opcode == 'b')  // INC or BEQZ
        {
            if(!valid[dest]){
                stalls++;
                dataStalls++;
                dataHazard = 1;
                return;
            }
            else{
                if(opcode == 'b'){
                    stalls += 2;
                    controlStalls += 2;
                    controlHazard = 2;
                    ControlInstCount++;
                    stalled = false;
                    return;
                }
            }
        }

        // 0 register operand
        else if(opcode == 'a'){ // JMP
            stalls += 2;
            controlStalls += 2;
            controlHazard = 2;
            stalled = false;
            ControlInstCount++;
            return;
        }
        else if(opcode == 'f'){   // HALT
            haltI = true;
            stalled = false;
            HaltInstCount++;
            return;
        }

        valid[dest] = 0;
        stalled = false;

        if (opcode == '8' || opcode == '9')
            LoadStoreI = true;
        if (opcode == '0' || opcode == '1' || opcode == '2' || opcode == '3')
            ArithmI = true;
        if (opcode == '4' || opcode == '5' || opcode == '6' || opcode == '7')
            LogicI = true;

        
        ArithmeticInstCount += ArithmI;
        LogicalInstCount += LogicI;
        DataInstCount += LoadStoreI;
    }

    void Execution_Cycle()
    {
        if (haltI) return;

        if (opcode == '8' || opcode == '9' || opcode == '0')
            ALUOutput = (B + C) % 256;
        else if (opcode == '1')
            ALUOutput = (B - C) % 256;
        else if (opcode == '2')
            ALUOutput = (B * C) % 256;
        else if (opcode == '3')
            ALUOutput = (A + 1) % 256;
        else if (opcode == '4')
            ALUOutput = B & C;
        else if (opcode == '5')
            ALUOutput = B | C;
        else if (opcode == '6')
            ALUOutput = ~B;
        else if (opcode == '7')
            ALUOutput = B ^ C;
        else if (opcode == 'a'){
            int val = (A << 4) | B;
            int signedVal = (val & 1 << 7) == 0 ? val : (val - 256);
            PC = PC + 2 * (signedVal);
        }
        else if (opcode == 'b'){
            if (A == 0)
            {
                int val = (B << 4) | C;
                int signedVal = (val & 1 << 7) == 0 ? val : (val - 256);
                PC = PC + 2 * (signedVal);
            }
        }
            
    }

    void Memory_Access_Cycle()
    {
        if (opcode == '8')
        { // LOAD
            LMD = dataCache[ALUOutput];
        }
        else if (opcode == '9') // STORE
        {
            dataCache[ALUOutput] = registerFile[dest];
        }
    }

    void Write_Back_Cycle()
    {
        CurrentInstructions--;
        if(haltI) return;

        if (ArithmI || LogicI)
            registerFile[dest] = ALUOutput;

        else if (opcode == '8')
            registerFile[dest] = LMD;

        valid[dest] = 1;
        
    }

    int hexCharToDec(char ch)
    {
        int dec_val = 0;
        if (ch >= '0' && ch <= '9')
            dec_val = (int(ch) - 48);
        else if (ch >= 'a' && ch <= 'f')
            dec_val = (int(ch) - 87);

        return dec_val;
    }
};

void WriteToFile()
{
    ofstream ODCache;
    ODCache.open("output/ODCache.txt");
    for (int i = 0; i < 256; i++)
    {
        int d = dataCache[i];
        if (d < 0)
            d = 256 + d;

        string data = DecTohex(d);
        if(data.size() == 0) data = "00";
        else if(data.size() == 1) data = "0" + data;
        ODCache << data << "\n";
    }
    ODCache.close();
}

int main()
{

    ifstream ICache;
    ICache.open("input/ICache.txt");
    for (int i = 0; i < 256; i++)
    {
        string instruction;
        ICache >> instruction;

        instructionCache[i] = instruction;
    }
    ICache.close();

    ifstream DCache;
    DCache.open("input/DCache.txt");
    for (int i = 0; i < 256; i++)
    {
        string data;
        DCache >> data;

        int d = hexToDec(data);
        int signedD = (d & 1 << 7) == 0 ? d : (d - 256);
        dataCache[i] = signedD;
    }
    DCache.close();

    ifstream RF;
    RF.open("input/RF.txt");
    for (int i = 0; i < 16; i++)
    {
        string registerVal;
        RF >> registerVal;

        int rv = hexToDec(registerVal);
        int signedRV = (rv & 1 << 7) == 0 ? rv : (rv - 256);
        registerFile[i] = signedRV;
    }
    RF.close();

    Instruction array[5];

    array[0].start(PC);
    cycles++;

    /* Idea :
    At the start of each iteration,
    array[0] will undergo ID Cycle
    array[1] will undergo EX Cycle
    array[2] will undergo MEM Cycle
    array[3] will undergo WB Cycle
    */

    while ( CurrentInstructions > 0)
    {
        array[0].next();
        array[3].next();
        array[2].next();
        array[1].next();

        cycles++;

        for(int j = 4; j > 1; j--) {
            array[j] = array[j - 1];
        }
        array[1].flush();

        if(dataHazard > 0)
        {
            dataHazard--;
            continue;
        }
        if(controlHazard > 0){
            array[0].next();
            array[3].next();
            array[2].next();
            array[1].next();

            for(int j = 4; j > 1; j--)
                array[j] = array[j - 1];
        
            array[1].flush();

            cycles++;
            controlHazard = 0;
            continue;
        }

        array[1] = array[0];
        array[0].start(PC);
    }

    ofstream file;
    file.open("output/Output.txt");

    file << "Total number of instructions executed\t: "<< InstructionCount << "\n";
    file << "Number of instructions in each class\n";
    file << "Arithmetic instructions\t\t\t\t\t: "<< ArithmeticInstCount << "\n";
    file << "Logical instructions\t\t\t\t\t: "<<LogicalInstCount << "\n";
    file << "Data instructions\t\t\t\t\t\t: "<<DataInstCount << "\n";
    file << "Control instructions\t\t\t\t\t: "<< ControlInstCount << "\n";
    file << "Halt instructions\t\t\t\t\t\t: "<< HaltInstCount << "\n";
	file << "Cycles Per Instruction\t\t\t\t\t: " << (double) cycles / InstructionCount<<"\n";
    file << "Total number of stalls\t\t\t\t\t: "<< stalls << "\n";
    file << "Data stalls (RAW)\t\t\t\t\t\t: "<< dataStalls << "\n";
    file << "Control stalls\t\t\t\t\t\t\t: " << controlStalls << "\n";

    file.close();

    WriteToFile();
    cout << "Execution Completed" << endl;

    return 0;
}