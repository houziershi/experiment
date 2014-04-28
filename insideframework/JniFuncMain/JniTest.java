
class JniTest
{
	private int intField;
	
	public JniTest(int num)
	{
		intField = num;
		
		System.out.println("JniTest JniTest(): intField = " + intField);
	}

	public int callByNative(int num)
	{
		System.out.println("JniTest callByNative(): num = " + num);
		return num;
	}
	
	public void callTest()
	{
		System.out.println("JniTest callTest(): intField = " + intField);
	}
}



