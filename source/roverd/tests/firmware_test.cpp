#include <cassert>
#include <iostream>
#include "rover_move.hpp"

int main(int argc, char** argv) {
    const std::string scenario = argc > 1 ? argv[1] : "protocol";
    MachineContext ctx;
    if (scenario == "overflow") {
        fake_uart = std::string(64, 'x') + "A\n";
        assert(!updateContextFromSerial(ctx) && "overlong line must not accept its suffix");
        fake_uart = "B\n";
        assert(updateContextFromSerial(ctx) && ctx.last_letter == 'B');
    } else if (scenario == "zero") {
        fake_now=0; fake_uart="M\n";
        assert(updateContextFromSerial(ctx));
        fake_now=301;
        assert(isLinkTimedOut(ctx) && "command received at boot must time out");
    } else if (scenario == "center") {
        for (char letter : std::string("KLMNOPklmnop")) {
            stopMotors(ctx);
            for(int i=0;i<50;++i) driveFromLetter(ctx,letter);
            assert(ctx.curr_l==ctx.curr_r && ctx.curr_l>0 && "straight deadband must command equal wheel speeds");
        }
    } else {
        assert(!isLinkTimedOut(ctx));
        fake_now=100; fake_uart="A\r\n";
        assert(updateContextFromSerial(ctx) && ctx.last_letter=='A');
        fake_now=400; assert(!isLinkTimedOut(ctx));
        fake_now=401; assert(isLinkTimedOut(ctx));
        fake_uart="AB\n!\n\n";
        assert(!updateContextFromSerial(ctx) && ctx.last_rx_ms==100);
        fake_now=0xfffffff0; fake_uart="z\n";
        assert(updateContextFromSerial(ctx) && ctx.last_letter=='z');
        fake_now=0x120; assert(isLinkTimedOut(ctx));
        fake_uart="?\n"; assert(updateContextFromSerial(ctx));
        for(int i=0;i<50;++i) driveFromLetter(ctx,'A');
        assert(ctx.curr_l>0 && ctx.curr_r<0);
        stopMotors(ctx); assert(ctx.curr_l==0 && ctx.curr_r==0);
        for(int i=0;i<50;++i) driveFromLetter(ctx,'Z');
        assert(ctx.curr_l<0 && ctx.curr_r>0);
        for(int i=0;i<50;++i) driveFromLetter(ctx,'?');
        assert(ctx.curr_l==0 && ctx.curr_r==0);
    }
    std::cout << scenario << " passed\n";
}
