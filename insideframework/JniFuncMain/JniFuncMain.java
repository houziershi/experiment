

public class JniFuncMain
{
	private static int staticIntField = 300;
	
	static { System.loadLibrary("jnifunc"); }
	public static native JniTest createJniObject();
	
	public static void main(String[] arg)
	{
		System.out.println("[java] createJniObject()");
		JniTest jniObj = createJniObject();
		
		jniObj.callTest();
	}
}