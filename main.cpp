#include "ARM64_CPU.h"
#include <iostream>
using namespace std;

void sub_98604(ARM64_CPU& cpu) {
    cpu.adds(W4, W4, W4);
    if (cpu.cbz(W4)) goto lable1;
    return;

lable1:
    cpu.ldr(W4, X0, 4);
    cpu.adcs(W4, W4, W4);
    return;
}
int main() {
    ARM64_CPU cpu = ARM64_CPU();
    Memory& memory = cpu.mem;
    cpu.mem.loadFile("D:\\apk\\com.ct.client\\compress.data");
    cpu.writeReg(X0, 0);
    cpu.writeReg(X1, 0x40e09);
    cpu.writeReg(X2, 0x40e09);
    cpu.add(X7, X0, W1, UXTW);
    //MOV  W5, #0xFFFFFFFF
    //MOV  W4, #0x80000000
    cpu.writeReg(W5, 0xFFFFFFFF);
    cpu.writeReg(W4, 0x80000000);

    goto loc_98624;

loc_9861C:
    cpu.ldrb(W3, X0, 1);
    cpu.strb(W3, X2, 1);
    //std::cout
    //	<< "write£º 0x"
    //	<< std::hex
    //	<< cpu.readReg(W3)
    //	<< std::endl;

loc_98624:
    sub_98604(cpu);
    if (cpu.b_cs()) {
        goto loc_9861C;
    }
    cpu.writeReg(W1, 1);
    goto loc_98640;
loc_98634:
    cpu.sub(W1, W1, 1);
    sub_98604(cpu);
    cpu.adc(W1, W1, W1);

loc_98640:
    sub_98604(cpu);
    cpu.adc(W1, W1, W1);
    sub_98604(cpu);
    if (cpu.b_cc()) {
        goto loc_98634;
    }
    cpu.subs(W3, W1, 3);
    cpu.writeReg(W1, 0);
    if (cpu.b_cc()) {
        goto loc_9867C;
    }
    cpu.ldrb(W5, X0, 1);

loc_98660:
    cpu.orr(W5, W5, W3, LSL, 8);
    cpu.mvn(W5, W5);
    if (cpu.cbz(W5)) {
        std::ofstream file("D:\\apk\\com.ct.client\\uncompress.data", std::ios::binary);
        size_t start = 0x40e09;
        for (size_t addr = start; addr < memory.bytes.size(); addr++)
        {
            uint8_t data = memory.read8(addr);
            file.write(reinterpret_cast<char*>(&data), 1);
        }

        file.close();
        return 0;
    }
    cpu.tst(W5, 1);
    cpu.asr(W5, W5, 1);
    if (cpu.b_ne()) {
        goto loc_986A8;
    }
loc_98678:
    goto loc_98684;

loc_9867C:
    sub_98604(cpu);
    if (cpu.b_cs()) {
        goto loc_986A8;
    }

loc_98684:
    cpu.writeReg(W1, 1);
    sub_98604(cpu);
    if (cpu.b_cs()) {
        goto loc_986A8;
    }
loc_98690:
    sub_98604(cpu);
    cpu.adc(W1, W1, W1);
    sub_98604(cpu);
    if (cpu.b_cc()) {
        goto loc_98690;
    }
    cpu.add(W1, W1, 4);
    goto loc_986B4;

loc_986A8:
    sub_98604(cpu);
    cpu.adc(W1, W1, W1);
    cpu.add(W1, W1, 2);
loc_986B4:
    cpu.adds(W31, W5, 0x500);
    cpu.cinc(W1, W1, CS);
    cpu.add(X3, X2, W1, UXTW);
    cpu.ldurb(W3, X3, -1);
loc_986C4:
    cpu.ldrb(W3, X2, W5, SXTW);
    cpu.strb(W3, X2, 1);
    cpu.subs(W1, W1, 1);
    if (cpu.b_ne()) {
        goto loc_986C4;
    }
    goto loc_98624;

}