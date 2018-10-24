#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "NativeCompressCall.h"
#include "zlib.h"

typedef unsigned char byte;

static jfieldID consumedID;
static jfieldID finishedID;

jint throwException(JNIEnv *env, char *message)
{
    jclass exClass = (*env)->FindClass(env,"java/lang/RuntimeException");
    // Can never actually happen on a sane JVM, but better be safe anyway
    if (exClass == NULL) {
        return -1;
    }

    return (*env)->ThrowNew(env,exClass, message);
}

JNIEXPORT void JNICALL Java_NativeCompressCall_initFields(JNIEnv* env, jclass clazz) 
{
    // We trust that these fields will be there
    //consumedID = env->GetFieldID(clazz, "consumed", "I");
    consumedID = (*env)->GetFieldID(env,clazz, "consumed", "I");
    //finishedID = env->GetFieldID(clazz, "finished", "Z");
    finishedID = (*env)->GetFieldID(env,clazz, "finished", "Z");
}

JNIEXPORT void JNICALL Java_NativeCompressCall_reset(JNIEnv* env, jobject obj, jlong ctx, jboolean compress)
{

    z_stream* stream = (z_stream*) ctx;
    int ret = (compress) ? deflateReset(stream) : inflateReset(stream);

    if (ret != Z_OK) {
    	char err[64] = {0};
	snprintf(err,64,"Could not reset z_stream: %d",ret);
        throwException(env, err);
        //throwException(env, "Could not reset z_stream: " + to_string(ret));
    }
	
}


JNIEXPORT void JNICALL Java_NativeCompressCall_end(JNIEnv* env, jobject obj, jlong ctx, jboolean compress) 
{
    z_stream* stream = (z_stream*) ctx;
    int ret = (compress) ? deflateEnd(stream) : inflateEnd(stream);

    free(stream);

    if (ret != Z_OK) {
        char err[64] = {0};
	snprintf(err,64,"Could not free z_stream: %d",ret);
        throwException(env, err);
        //throwException(env, "Could not free z_stream: ", ret);
    }
	
}

JNIEXPORT jlong JNICALL Java_NativeCompressCall_init(JNIEnv* env, jobject obj, jboolean compress, jint level) 
{
    int ret = -1;
    z_stream* stream = (z_stream*) calloc(1, sizeof (z_stream));
    ret = (compress) ? deflateInit2(stream, 1, 8, 31, level, 0) : inflateInit2(stream, 31);


    if (ret != Z_OK) {
        char err[64] = {0};
	snprintf(err,64,"Could not init z_stream: %d",ret);
        throwException(env, err);
        //throwException(env, "Could not init z_stream", ret);
	return 0;
    }

    return (jlong) stream;
}

JNIEXPORT jint JNICALL Java_NativeCompressCall_process(JNIEnv* env, jobject obj, jlong ctx, jlong in, jint inLength, jlong out, jint outLength, jboolean compress) 
{
    int ret = -1;
    z_stream* stream = (z_stream*) ctx;

    stream->avail_in = inLength;
    stream->next_in = (byte*) in;

    stream->avail_out = outLength;
    stream->next_out = (byte*) out;

	//printf("in = %s, inlength = %d; outlength = %d\n",stream->next_in,stream->avail_in, stream->avail_out);
	//printf("inlength = %d; outlength = %d\n",stream->avail_in, stream->avail_out);

    ret = (compress) ? deflate(stream, Z_FINISH) : inflate(stream, Z_PARTIAL_FLUSH);
	//printf("ret = %d, out = %s\n, outlength = %d\n", ret,(byte*)out,stream->avail_out);
	//printf("ret = %d, outlength = %d, avail_out = %d\n", ret,stream->avail_out,outLength - stream->avail_out);

    switch (ret) {
        case Z_STREAM_END:
            //env->SetBooleanField(obj, finishedID, true);
            (*env)->SetBooleanField(env,obj, finishedID, 1);
            break;
        case Z_OK:
            break;
        default:
    	   (*env)->SetIntField(env,obj, consumedID, inLength - stream->avail_in);
	    return ret;
            //throwException(env, "Unknown z_stream return code", ret);
            break;
    }

    //env->SetIntField(obj, consumedID, inLength - stream->avail_in);
    (*env)->SetIntField(env,obj, consumedID, inLength - stream->avail_in);

#if 0
	if(ret == Z_OK || ret == Z_STREAM_END)
	{
		FILE *fp = NULL;
		fp = fopen("out.tgz","wb");

		fwrite((byte*)out,outLength - stream->avail_out,1,fp);

		fclose(fp);

	}
#endif


	//printf("out = %s\n, outlength = %d\n",stream->next_out,stream->avail_out);

    return outLength - stream->avail_out;
}


