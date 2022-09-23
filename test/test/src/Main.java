import java.lang.management.ManagementFactory;
import java.util.Scanner;
import com.sun.tools.attach.VirtualMachine;

public class Main {
    public static void main(String[] args) {
        //System.loadLibrary("E:\\项目\\jni_log4j_detect\\duck_log4j\\log4j_detect\\x64\\Release\\log4j_detect.dll");

        String name = ManagementFactory.getRuntimeMXBean().getName();
        // get pid
        String pid = name.split("@")[0];
        System.out.println("Pid is:" + String.format("%02X", Integer.parseInt(pid)));

        System.out.println("Hello world!");
        Scanner userInput = new Scanner(System.in);
        userInput.nextLine();
    }
}