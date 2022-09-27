// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
namespace Agent {
namespace Callback {
auto findJavaClass(JNIEnv* jni_env, const char* name) -> jclass {
    jclass ref = jni_env->FindClass(name);
    if (ref == nullptr) {
        jni_env->ExceptionClear();
    }
    return ref;
}
auto getClassFullPath(JNIEnv* jni_env, jobject javaObj) -> std::string {
    std::string result{};
    // getSimpleName
    jclass javaClass = jni_env->GetObjectClass(javaObj);
    jmethodID method_GetClass =
        jni_env->GetMethodID(javaClass, "getClass", "()Ljava/lang/Class;");
    jobject getClass_CallObj =
        jni_env->CallObjectMethod(javaObj, method_GetClass);
    jclass getClass_CallClass = jni_env->GetObjectClass(getClass_CallObj);
    jmethodID method_GetSimpleName = jni_env->GetMethodID(
        getClass_CallClass, "getSimpleName", "()Ljava/lang/String;");
    jstring jstring_GetSimpleName = (jstring)jni_env->CallObjectMethod(
        getClass_CallObj, method_GetSimpleName);
    const char* simpleName_StringBuffer =
        jni_env->GetStringUTFChars(jstring_GetSimpleName, nullptr);
    std::string simpleName = simpleName_StringBuffer;
    simpleName += ".class";
    jni_env->ReleaseStringUTFChars(jstring_GetSimpleName,
                                   simpleName_StringBuffer);
    // getResource
    jmethodID method_GetResource =
        jni_env->GetMethodID(getClass_CallClass, "getResource",
                             "(Ljava/lang/String;)Ljava/net/URL;");
    jstring jstring_SimpleNameBuffer =
        jni_env->NewStringUTF(simpleName.c_str());
    jobject urlObj = (jstring)jni_env->CallObjectMethod(
        getClass_CallObj, method_GetResource, jstring_SimpleNameBuffer);
    if (urlObj == nullptr) {
        // fix me
        return result;
    }
    // url_obj to string
    jclass javaUrlClass = jni_env->GetObjectClass(urlObj);
    jmethodID method_ToString =
        jni_env->GetMethodID(javaUrlClass, "toString", "()Ljava/lang/String;");
    jstring jstring_urlStringBuffer =
        (jstring)jni_env->CallObjectMethod(urlObj, method_ToString);
    const char* url =
        jni_env->GetStringUTFChars(jstring_urlStringBuffer, nullptr);
    result = url;
    jni_env->ReleaseStringUTFChars(jstring_urlStringBuffer, url);
    return result;
}
auto getStackPackageList(JNIEnv* jni_env) -> std::vector<std::string> {
    std::vector<std::string> packageList{};
    // get StackTraceElement by Thread.currentThread().getStackTrace()
    jclass javaThreadClass = findJavaClass(jni_env, "java/lang/Thread");
    jmethodID method_CurrentThread = jni_env->GetStaticMethodID(
        javaThreadClass, "currentThread", "()Ljava/lang/Thread;");
    jobject javaThreadObj =
        jni_env->CallStaticObjectMethod(javaThreadClass, method_CurrentThread);
    jclass javaThreadObjClass = jni_env->GetObjectClass(javaThreadObj);
    jmethodID method_GetStackTrace =
        jni_env->GetMethodID(javaThreadObjClass, "getStackTrace",
                             "()[Ljava/lang/StackTraceElement;");
    jobjectArray javaStackTraceElementArray =
        (jobjectArray)jni_env->CallObjectMethod(javaThreadObj,
                                                method_GetStackTrace);
    // get StackTraceElement
    jclass javaStackTraceElementClass =
        findJavaClass(jni_env, "java/lang/StackTraceElement");
    jmethodID method_GetClassName = jni_env->GetMethodID(
        javaStackTraceElementClass, "getClassName", "()Ljava/lang/String;");
    jmethodID method_GetMethodName = jni_env->GetMethodID(
        javaStackTraceElementClass, "getMethodName", "()Ljava/lang/String;");
    /*
    jmethodID method_GetLineNumber = jni_env->GetMethodID(
    javaStackTraceElementClass, "getLineNumber", "()I");
    */
    // get StackTraceElement
    const auto javaStackTraceElementArrayLength =
        jni_env->GetArrayLength(javaStackTraceElementArray);
    for (auto i = 0; i < javaStackTraceElementArrayLength; i++) {
        jobject javaStackTraceElementObj =
            jni_env->GetObjectArrayElement(javaStackTraceElementArray, i);
        jstring jstring_ClassNameBuffer = (jstring)jni_env->CallObjectMethod(
            javaStackTraceElementObj, method_GetClassName);
        const char* className =
            jni_env->GetStringUTFChars(jstring_ClassNameBuffer, nullptr);
        jstring jstring_MethodNameBuffer = (jstring)jni_env->CallObjectMethod(
            javaStackTraceElementObj, method_GetMethodName);
        const char* methodName =
            jni_env->GetStringUTFChars(jstring_MethodNameBuffer, nullptr);
        std::string fullPackageName{};
        fullPackageName += className;
        fullPackageName += ".";
        fullPackageName += methodName;
        packageList.push_back(fullPackageName);
        // Tools::DbgPrint("fullPackageName: %s \n", fullPackageName.c_str());
        /*
        jint lineNumber =
        jni_env->CallIntMethod(javaStackTraceElementObj,
                                                method_GetLineNumber);
        Tools::DbgPrint("StackTraceElement: %s.%s:%d \n", className,
        methodName, lineNumber);
        */

        jni_env->ReleaseStringUTFChars(jstring_ClassNameBuffer, className);
        jni_env->ReleaseStringUTFChars(jstring_MethodNameBuffer, methodName);
    }
    return packageList;
}
auto __stdcall ClassFileLoadHook(jvmtiEnv* jvmti_env, JNIEnv* jni_env,
                                 jclass class_being_redefined, jobject loader,
                                 const char* name, jobject protection_domain,
                                 jint class_data_len,
                                 const unsigned char* class_data,
                                 jint* new_class_data_len,
                                 unsigned char** new_class_data) -> void {
    if (loader == nullptr || jni_env == nullptr) {
        return;
    }
    clock_t startTime, endTime;
    startTime = clock();
    // 是否在原有文件中
    const auto javaClass = findJavaClass(jni_env, name);

    // 检查loader的类 todo:黑名单类
    const auto loaderPath = getClassFullPath(jni_env, loader);
    if (loaderPath.size() == 0) {
        return;
    }
    // 检查栈
    const auto packageList = getStackPackageList(jni_env);
    static std::string log4jJniPackageName =
        "org.apache.logging.log4j.core.lookup.JndiLookup.lookup";
    bool lookUpJniManager = false;
    for (const auto& package : packageList) {
        if (package == log4jJniPackageName) {
            lookUpJniManager = true;
            break;
        }
    }
    endTime = clock();
    if (lookUpJniManager && javaClass == nullptr) {
        Tools::DbgPrint("suspicious package load : %s from %s by log4j \n",
                        name, loaderPath.c_str());
        Tools::DbgPrint("time: %lf ms \n",
                        (double)(endTime - startTime) / CLOCKS_PER_SEC);
    }
}
}  // namespace Callback
auto Init(JavaVM* vm) -> jint {
    jvmtiEnv* jvmti = nullptr;
    jint res = vm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_0);
    if (res != JNI_OK || jvmti == nullptr) {
        Tools::DbgPrint(
            "ERROR: Unable to access JVMTI Version 1, 2 or higher\n");
        return JNI_ERR;
    }
    // https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html#ClassFileLoadHook
    jvmtiEventCallbacks callbacks = {0};
    callbacks.ClassFileLoadHook = Callback::ClassFileLoadHook;

    jvmtiCapabilities capabilities = {0};
    capabilities.can_generate_all_class_hook_events = 1;

    jvmti->AddCapabilities(&capabilities);
    jvmti->SetEventCallbacks(&callbacks, sizeof(jvmtiEventCallbacks));
    jvmti->SetEventNotificationMode(JVMTI_ENABLE,
                                    JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
    Tools::DbgPrint("Agent::Init\n");
    return 0;
}
}  // namespace Agent
JNIEXPORT auto __stdcall Agent_OnAttach(JavaVM* vm, char* options,
                                        void* reserved) -> jint {
    return Agent::Init(vm);
}
JNIEXPORT auto __stdcall Agent_OnUnload(JavaVM* vm) -> void {
    Tools::DbgPrint("Agent_OnUnload");
}
auto __stdcall DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                       LPVOID lpReserved) -> bool {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return true;
}
