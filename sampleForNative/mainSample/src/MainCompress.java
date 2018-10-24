import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;

import java.io.BufferedReader;
import java.io.InputStreamReader;

public class MainCompress {

	/** TODO
	 * @param args
	 * 2017-6-8
	 * zhuna
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		NativeCompressEncap nativeCompress = new NativeCompressEncap();	
		BufferedReader input = new BufferedReader(new InputStreamReader(System.in));
		boolean compress=false;
		int compressionLevel = 0;
								
		try{
						
			//System.out.println("The libraryPath:"+System.getProperty("java.library.path"));
			
			System.out.println("Please input the compress:");
			//输入类型  true:压缩    false:解压缩
			String compressFlag = input.readLine(); 
			if("true".equals(compressFlag)){
				compress=true;
			}									
			System.out.println("Please input compressionLevel:");
			//压缩/解压缩等级
			compressionLevel= Integer.parseInt(input.readLine());
			
			//压缩初始化接口调用
			long initResult = nativeCompress.init(compress, compressionLevel);
			
			//压缩初始化失败
			if(0 == initResult){
				System.out.println("Initialization operation failed,please check!");
				
			}else{//成功
				System.out.println("Initialization operation successful!");
				
				byte[] dataBuf = new byte[1 << 25]; // 32 megabytes				
				System.out.println("Please input deflate data:");
				String in = input.readLine();
				dataBuf = in.getBytes();
				ByteBuf originalBuf = Unpooled.directBuffer();
				originalBuf.writeBytes( dataBuf );
				
				ByteBuf compressed = Unpooled.directBuffer();				
				ByteBuf uncompressed = Unpooled.directBuffer();	
				
				//自定义输出缓存区的大小
				compressed.ensureWritable( 64*1024 );
				uncompressed.ensureWritable( 64*1024 );
								
				int processed = nativeCompress.process(originalBuf, compressed);
				compressed.writerIndex( compressed.writerIndex() + processed );
				
				//*********************解压缩************************
				//解压缩初始化
				nativeCompress.init(false, 0);
				//解压缩接口调用
				processed = nativeCompress.process(compressed, uncompressed);
				//解压缩内容写入缓冲区
				uncompressed.writerIndex( uncompressed.writerIndex() + processed );
				
				byte[] inflateCheck = new byte[ uncompressed.readableBytes() ];
				uncompressed.readBytes( inflateCheck );
		        String inflateResult = new String(inflateCheck);
		        //处理结果输出
		        System.out.println("The inflate result:"+inflateResult);
		       
			}
			
		}catch(Exception e){
			e.printStackTrace();
		}		
	
	}
}
