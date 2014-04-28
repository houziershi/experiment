


#include <stdio.h>

#if 0
#include "HelloJNI.h"


JNIEXPORT void JNICALL Java_HelloJNI_printHello (JNIEnv *env, jobject obj)
{
	printf("Hello World");
	return;
}

JNIEXPORT void JNICALL Java_HelloJNI_printString (JNIEnv *env, jobject obj, jstring string)
{
	const char *str = (*env)->GetStringUTFChars(env, string, 0);
	printf("%s\n", str);
	return;
}
#else
#include "JNIHelp.h"
#include "jni.h"

void printHelloNative (JNIEnv *env, jobject obj);
void printStringNative (JNIEnv *env, jobject obj, jstring string);

extern "C" jint JNI_Onload(JavaVM *vm, void *reserved)
{
	JNIEnv *env = NULL;
	JNINativeMethod nm[2];
	jclass cls;
	jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        printf("GetEnv failed!");
        return result;
    }

	cls = env->FindClass("HelloJNI");

	nm[0].name = "printHello";
	nm[0].signature = "()V";
	nm[0].fnPtr = (void *)printHelloNative;

	nm[1].name = "printString";
	nm[1].signature = "(Ljava/lang/String)V";
	nm[1].fnPtr = (void *)printStringNative;

	env->RegisterNatives(cls, nm, 2);

	return JNI_VERSION_1_4;
}

void printHelloNative (JNIEnv *env, jobject obj)
{
	printf("Hello World");
	return;
}

void printStringNative (JNIEnv *env, jobject obj, jstring string)
{
	const char *str = env->GetStringUTFChars(string, 0);
	printf("%s\n", str);
	return;
}

#endif
