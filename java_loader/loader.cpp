#include "pch.h"
#include <assert.h>
auto main(int argc, char** argv) -> int {
    jni::Vm vm("C:\\Program Files\\Java\\jdk-16.0.2\\bin\\server\\jvm.dll");
    jni::Class virtualMachineClass = jni::Class("com/sun/tools/attach/VirtualMachine");
    jni::Object virtualMachineObject = virtualMachineClass.cs_call<jni::Object>("attach","(Ljava/lang/String;)Lcom/sun/tools/attach/VirtualMachine;", argv[0]); //目标pid
    virtualMachineClass.cs_dynamic_call<void>(virtualMachineObject, "loadAgentPath", "(Ljava/lang/String;)V", "C:\\SentinelJava64.dll");
    virtualMachineClass.cs_call<void>("detach", "()V");
    return 0;
}