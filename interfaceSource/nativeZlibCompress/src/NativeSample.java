

public class NativeSample {
	static{
		try{
			System.loadLibrary("test");
		}catch(UnsatisfiedLinkError e){
			System.err.println( "Cannot load hello library:\n " + e.toString() );
		}
		/*try {
	        System.loadLibrary("libtest");
	    } catch (UnsatisfiedLinkError e) {
	        try {
	            NativeUtils.loadLibraryFromJar("/lib/library/libtest.so");
	        } catch (IOException e1) {
	            throw new RuntimeException(e1);
	        }
	    }*/
	}
	public static native int add_func(int a, int b);
	
	/*public static void main(String[] args){
		
		BufferedReader input = new BufferedReader(new InputStreamReader(System.in));
		int para1=0;
		int para2=0;
		try{
			//System.setProperty("java.library.path", "/root/zn/JINJAVA");
			System.out.println("The libraryPath:"+System.getProperty("java.library.path"));
						
			System.out.println("please input paramter1");		
			para1= Integer.parseInt(input.readLine());
			
			System.out.println("please input paramter2");		
			para2= Integer.parseInt(input.readLine());
			
			int result = add_func(para1,para2);
			System.out.println(result);
			
		}catch(Exception e){
			e.printStackTrace();
		}		
		
	 }*/
}
