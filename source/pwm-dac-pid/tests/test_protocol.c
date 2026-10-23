#include "protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    command c;
    const char* bad[]={"SET","AUTO","KP","SET abc","SET 2garbage","SET 1 2","READ 0","MONITOR 1","NOPE","","SET 999999999999999999999999999","AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA 7"};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];++i) assert(!parse_command(bad[i],&c));
    assert(parse_command("SET -4",&c) && c.kind==CMD_SET && c.value==0);
    assert(parse_command("SET 999",&c) && c.value==255);
    assert(parse_command("AUTO 2048",&c) && c.value==1023);
    assert(parse_command("AUTO -1",&c) && c.value==0);
    assert(parse_command(" KP 500 ",&c) && c.kind==CMD_KP && c.value==500);
    assert(!parse_command("KI -1",&c));
    assert(parse_command("KD 1",&c) && c.kind==CMD_KD);
    assert(parse_command("READ",&c) && c.kind==CMD_READ);
    assert(parse_command("MONITOR",&c) && c.kind==CMD_MONITOR);
    assert(parse_command("STOP",&c) && c.kind==CMD_STOP);
    command_buffer b={{0},0,0};
    for(int i=0;i<100;++i) assert(command_feed(&b,'A')==0);
    assert(command_feed(&b,'\r')==-1);
    assert(command_feed(&b,'S')==0); assert(command_feed(&b,'E')==0);
    assert(command_feed(&b,'T')==0); assert(command_feed(&b,' ')==0);
    assert(command_feed(&b,'5')==0); assert(command_feed(&b,'\n')==1);
    assert(parse_command(b.text,&c) && c.value==5);
    puts("Malformed command rejection, bounded framing, ranges and gain commands PASS");
}
