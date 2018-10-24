/**
 * 本地方法类，加载本地动态库，需要业务调用的本地方法
 * 编码完成后使用JNI中javah命令生成.h的头文件（lib里面的NativeCompressCall.h）
 * @author zhuna
 *
 */
public class NativeCompressCall {
	int consumed;
	boolean finished;
	//调用System.loadLibrary去加载动态库，而它其实就是通过ClassLoader的loadLibrary方法来加载
	static{
		try{
			//System.out.println("The libraryPath:"+System.getProperty("java.library.path"));
			//System.setProperty("java.library.path", "/root/zn/JINJAVA");
			//System.out.println("The libraryPath:"+System.getProperty("java.library.path"));
			System.loadLibrary("gaiqiz");
			initFields();

		}catch(UnsatisfiedLinkError e){
			System.err.println( "Cannot load library:\n " + e.toString() );
		}
	}

	static native void initFields();

	public native void end(long ctx, boolean compress);

	public native void reset(long ctx, boolean compress);

	public native long init(boolean compress, int compressionLevel);

	public native int process(long ctx, long in, int inLength, long out, int outLength, boolean compress);

}
