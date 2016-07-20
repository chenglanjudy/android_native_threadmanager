#include "logging.h"
#include "Thread.h"
#include "MessageHandler.h"
#include "Looper.h"
#include "Message.h"
#include "MessageQueue.h"
#include "ThreadDefs.h"
#include <jni.h>

namespace ThreadManager{

static jobject g_obj = NULL;
static JavaVM *g_jvm = NULL;

int flag = 1;

//----------- NativeTManager --------------
class NativeTManager : public MessageHandlerPolicyInterface 
					, public LooperPolicyInterface {
protected:
	virtual ~NativeTManager();
public:
	NativeTManager(jobject serviceObj);
	virtual void start();
	virtual int sendMsg();
	virtual void attachJavaThread();
	virtual void detachJavaThread();
	virtual void notifyMessage();
private:
	jobject mServiceObj;
	HandleThread* ht;
	JNIEnv* mEnv;
	MessageHandlerInterface* mh;
};//end of class NativeTManager

NativeTManager::NativeTManager(jobject serviceObj){
	mServiceObj = serviceObj;
	ht = new HandleThread(this);
	LOG_IF_ERRNO(NULL == ht,"Can't allocate a new MessageHandler.");
}

NativeTManager::~NativeTManager(){
	delete ht;
	mEnv->DeleteGlobalRef(mServiceObj);
}

void NativeTManager::start(){
	int result = ht->run("Anthor Thread", PRIORITY_URGENT_DISPLAY);
	LOG_IF_ERRNO(result!=0,"Could not start Anthor thread due to error %d.", result);

	mh = new MessageHandler(ht->getLooper(),ht->getLooper()->getQueue(),this);
	LOG_IF_ERRNO(NULL == mh,"Can't allocate a new MessageHandler.");
}

int NativeTManager::sendMsg(){
	mh->sendMessage(*(mh->obtainMessage()),systemTime(SYSTEM_TIME_MONOTONIC));
}

void NativeTManager::attachJavaThread(){
	LOG_IF_ERRNO(g_jvm->AttachCurrentThread(&mEnv, NULL) != JNI_OK
		, "%s: AttachCurrentThread() failed", __FUNCTION__);    
}

void NativeTManager::detachJavaThread(){
	LOG_IF_ERRNO(g_jvm->DetachCurrentThread() != JNI_OK 
		, "%s: DetachCurrentThread() failed", __FUNCTION__);
}

jobject getInstance(JNIEnv* env, jclass obj_class){    
	jmethodID construction_id = env->GetMethodID(obj_class, "<init>", "()V"); 
	LOG_IF_ERRNO(construction_id == NULL,"Get Method error.");
	jobject obj = env->NewObject(obj_class, construction_id);  
	LOG_IF_ERRNO(obj == NULL,"Get New Object error.");
	return obj;
}


jstring get(JNIEnv* env){
	jstring str;  
	jclass java_class = env->FindClass("com/nan/thread/CForCall");    
	if (java_class == 0){       
		return env->NewStringUTF("not find class!");    
	}   

	jobject java_obj = getInstance(env, java_class);   
	if (java_obj == 0){       
		return env->NewStringUTF("not find java OBJ!");   
	}   

	jmethodID java_method = env->GetMethodID(java_class, "GetJavaString", "()Ljava/lang/String;");  

	if(java_method == 0){       
		return env->NewStringUTF("not find java method!");   

	}   

	str = (jstring)env->CallObjectMethod(java_obj, java_method);   
	return str;
}


void NativeTManager::notifyMessage(){

	if(mEnv == NULL){
		ALOGE("Get the JNI ENV ERROR.");
		return;
	}
	
	jclass cls = mEnv->GetObjectClass(g_obj);

	LOG_IF_ERRNO(cls == NULL,"Get class error.");

	jmethodID mid = mEnv->GetMethodID(cls,"printString","()V");

	LOG_IF_ERRNO(mid == NULL,"Get method error.");

	mEnv->CallVoidMethod(g_obj,mid);

	/*
	//Get the method from the class.
	mid = mEnv->GetStaticMethodID(cls, "fromJNI", "(I)V");
	if (mid == NULL) 
	{
		ALOGE("GetMethodID() Error.....");
		return;  
	}

	//Invoke static method.
	mEnv->CallStaticVoidMethod(cls, mid ,flag++);
	*/
}

//-------------- Native method. --------------------

void initClassHelper(JNIEnv *env, const char *path, jobject *objptr) {
    jclass cls = env->FindClass(path);
    if(!cls) {
        ALOGE("initClassHelper: failed to get %s class reference", path);
        return;
    }
    jmethodID constr = env->GetMethodID(cls, "<init>", "()V");
    if(!constr) {
        ALOGE("initClassHelper: failed to get %s constructor", path);
        return;
    }
    jobject obj = env->NewObject(cls, constr);
    if(!obj) {
        ALOGE("initClassHelper: failed to create a %s object", path);
        return;
    }
    (*objptr) = env->NewGlobalRef(obj);
}



extern "C"{
	JNIEXPORT int Java_com_nan_thread_MyThreadActivity_nativeStart( JNIEnv* env, jobject obj, jint ptr);
	JNIEXPORT int Java_com_nan_thread_MyThreadActivity_nativeSendMsg( JNIEnv* env, jobject obj, jint ptr);
	JNIEXPORT int Java_com_nan_thread_MyThreadActivity_nativeInit( JNIEnv* env, jobject obj);
	JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved);
	JNIEXPORT jstring JNICALL Java_com_nan_thread_MyThreadActivity_stringFromJNI(JNIEnv* env, jobject thiz);
}


extern "C" JNIEXPORT jstring JNICALL Java_com_nan_thread_MyThreadActivity_stringFromJNI(JNIEnv* env, jobject thiz){ 
	return get(env);
}


//
extern "C" JNIEXPORT int Java_com_nan_thread_MyThreadActivity_nativeStart( JNIEnv* env, jobject obj, jint ptr)
{
    NativeTManager* im = reinterpret_cast<NativeTManager*>(ptr);
	im->start();
}

//
extern "C" JNIEXPORT int Java_com_nan_thread_MyThreadActivity_nativeSendMsg( JNIEnv* env, jobject obj, jint ptr)
{
    NativeTManager* im = reinterpret_cast<NativeTManager*>(ptr);
	return im->sendMsg();
}


extern "C" JNIEXPORT int Java_com_nan_thread_MyThreadActivity_nativeInit( JNIEnv* env, jobject obj)
{
	initClassHelper(env, "com/nan/thread/Inform", &g_obj);
    NativeTManager* tm = new NativeTManager(g_obj);
	return reinterpret_cast<jint>(tm);
}//end function

//
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved){  
	g_jvm = vm;
	JNIEnv* env = NULL;    
	jint result = -1;
	
	if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK)     { 
		ALOGE("GetEnv failed!");            
		return result;    
	}    

	//initClassHelper(env, "com/nan/thread/Inform", &g_obj);

	return JNI_VERSION_1_6;
}//end function

}//end of namespace
