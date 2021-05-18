public class test_libbraille {
    static {
	try {
	    System.loadLibrary("jbraille");
	} catch (UnsatisfiedLinkError e) {
	    System.err.println("Native code library failed to load. " + e);
	    System.exit(1);
	}
    }

   public static void main(String argv[]) {
       System.out.println("debut");
       System.out.println(jbraille.braille_init());
       jbraille.braille_display("test_libbraille started");
       System.out.println("fin");
   }
}
