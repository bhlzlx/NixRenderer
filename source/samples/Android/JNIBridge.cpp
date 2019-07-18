// android ndk headers
#include <NixApplication.h>
#include <nix/io/archieve.h>
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <string>

/*
    public native void NixVkInit(Object surface, int width, int height, Object assetManager);
    public native void NixVkDraw();
    public native void NixVkDestroy();
*/

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onInitialize(
                                            JNIEnv*     _env,
                                            jobject     _this,
                                            jobject     _surface,
                                            jint        _width,
                                            jint        _height,
                                            jstring		_documentPath,
                                            jobject     _assetManager
                                            )
{
    static bool initialized = false;
    static jobject currentSurface = NULL;
    if( !initialized ) {
        NixApplication* application = GetApplication();
        ANativeWindow* nativeWindow = ANativeWindow_fromSurface( _env, _surface );
        currentSurface = _surface;
        int length = (_env)->GetStringUTFLength(_documentPath);
        const char* jstr = (_env)->GetStringUTFChars(_documentPath, nullptr);
        std::string documentPath( jstr, jstr+length );
        int size = 0;
        Nix::IArchieve* arch = Nix::CreateStdArchieve(documentPath);
        application->initialize(nativeWindow, arch);
        initialized = true;
    }
    else
    {
        NixApplication* application = GetApplication();
        ANativeWindow* nativeWindow = ANativeWindow_fromSurface( _env, _surface );
        application->resume(nativeWindow, _width, _height);
        currentSurface = _surface;
    }
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onDraw(
                                            JNIEnv*     _env,
                                            jobject     _this
                                            )
{
    NixApplication* application = GetApplication();
    application->tick();
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onSizeChanged(
                                            JNIEnv*     _env,
                                            jobject     _this,
                                            jint        _width,
                                            jint        _height
                                            )
{
    NixApplication* application = GetApplication();
    application->resize( _width, _height );
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onDestroy(
                                            JNIEnv*     _env,
                                            jobject     _this
                                            )
{
    NixApplication* application = GetApplication();
    application->release();
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onResume(
                                            JNIEnv*     _env,
                                            jobject     _this,
                                            jobject     _surface,
                                            jint        _width,
                                            jint        _height
                                            )
{
    NixApplication* application = GetApplication();

    ANativeWindow* nativeWindow = ANativeWindow_fromSurface( _env, _surface );

    application->resume(nativeWindow, _width, _height);
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onPause(
                                            JNIEnv*     _env,
                                            jobject     _this
                                            )
{
    NixApplication* application = GetApplication();
    application->pause();
    return;
}