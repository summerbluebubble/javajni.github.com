package com.nativeTest;

/*
 *TODO java调用C的库函数
 *2017-5-23
 *zhuna
 */
public class NativeCallFromC {
	
	
	static{
		try{
			System.loadLibrary("HelloNative");
		}catch(UnsatisfiedLinkError e){
			System.err.println( "Cannot load hello library:\n " + e.toString() );
		}
		
	}
	 
	 public static native int deflateInit2(int level, int method, int windowbits, int  memlevel, int strategy);
	 
	 public static native int deflate();
	 
	 public static native int deflateEnd();
	 
	 public static native int inflateInit2();
	 
	 public static native int inflate();
	 
	 public static native int inflateEnd();
	 
	 public static void main(String[] args){
		 
		 deflateInit2(1,1,1,1,1);
	 }
}
