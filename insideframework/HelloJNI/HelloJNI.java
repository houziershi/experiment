class HelloJNI
{
	native void printHello();
	native void printString(String str);
	
	static {System.loadLibrary("hellojni");}
	
	public static void main(String args[])
	{
		HelloJNI myJNI = new HelloJNI();
		
		myJNI.printHello();
		myJNI.printString("hello world from printString fun");
	}
}