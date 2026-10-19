#include "key_policy.h"
#include "lingua_ffi.h"
#include <cassert>
#include <iostream>
int main() {
    using lingua::handles_key;
    assert(!handles_key(0xBD,false,false)); assert(!handles_key(0xBB,false,false));
    assert(!handles_key('1',false,false)); assert(!handles_key('A',true,true));
    assert(handles_key('A',false,false)); assert(handles_key(0xBD,true,false));
    assert(handles_key(0xBB,true,false)); assert(!handles_key(0xBD,true,false,0x21,0x22));
    assert(handles_key(0x21,true,false,0x21,0x22)); assert(lingua_abi_version()==1);
    auto* context=lingua_create(); assert(context);
    LinguaConfig config{}; assert(lingua_refresh(context,&config)==0);
    LinguaPage page{}; assert(lingua_query(context,"xuexi",0,&page)==0);
    assert(page.schema_version==1 && page.item_count>0);
    assert(lingua_query(context,"=",0,&page)==-2);
    lingua_destroy(context);
    std::cout << "Native key policy, ABI layout and Rust dictionary bridge passed.\n";
}
