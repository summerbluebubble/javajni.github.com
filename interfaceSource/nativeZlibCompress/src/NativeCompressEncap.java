import com.google.common.base.Preconditions;

import io.netty.buffer.ByteBuf;

/**
 * 数据压缩解压缩中间类调用，包括初始化和过程调用方法，该类用来调用本地函数对象NativeCompressCall
 *
 * @author zhuna
 *
 */
public class NativeCompressEncap {

	//本地方法调用
	private final NativeCompressCall nativeCompress = new NativeCompressCall();

	private boolean compress;
	private long ctx;

	/**
	 * TODO 初始化
	 * @param compress 压缩解压缩标记 true:压缩  false:解压缩
	 * @param compressionLevel 压缩解压缩等级
	 * @return
	 * 2017-6-12
	 * zhuna
	 */
	public long init(boolean compress, int compressionLevel){
		/*new NativeCompressCall().consumed=1000;
		new NativeCompressCall().finished=false; */
		free();
		this.compress = compress;
		//System.out.println(compress);
		this.ctx = nativeCompress.init(compress, compressionLevel);
		return ctx;
	}

	/**
	 * TODO 空间释放
	 * 2017-6-12
	 * zhuna
	 */
	public void free(){
		if ( ctx != 0 ){
			new NativeCompressCall().end( ctx, compress );
			ctx = 0;
		}

		new NativeCompressCall().consumed = 0;
		new NativeCompressCall().finished = false;
	}

	/**
	 * TODO 压缩解压缩调用
	 * @param in
	 * @param out
	 * 2017-6-12
	 * zhuna
	 */
	public int process(ByteBuf in, ByteBuf out){
		in.memoryAddress();
		out.memoryAddress();
		Preconditions.checkState( ctx != 0, "Invalid pointer to compress!" );
		//System.out.println(!nativeCompress.finished);
		//while ( !nativeCompress.finished ){

			int processed = nativeCompress.process(ctx, in.memoryAddress() + in.readerIndex(), in.readableBytes(), out.memoryAddress() + out.writerIndex(), out.writableBytes(), compress);
			//in.readerIndex( in.readerIndex() + nativeCompress.consumed );
			//out.writerIndex( out.writerIndex() + processed );

		//}

		//nativeCompress.reset(ctx, compress);
		nativeCompress.consumed = 0;
		nativeCompress.finished = false;

		return processed;
	}
}
