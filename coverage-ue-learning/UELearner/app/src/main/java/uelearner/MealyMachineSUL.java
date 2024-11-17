package uelearner;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.Arrays;

import org.slf4j.LoggerFactory;

import de.learnlib.sul.SUL;

import org.slf4j.Logger;

import static java.lang.Thread.sleep;

class MealyMachineSUL implements SUL<String, String> {
    private static final Logger LOGGER = LoggerFactory.getLogger(MealyMachineSUL.class);
    private final String path;
    private Process process;
    private BufferedReader reader;
    private BufferedWriter writer;


    public MealyMachineSUL(String programPath) throws IOException {
      this.path = programPath;
    }

    public void reset() {
        // Send SIGUSR1 to reset the process (if the Java process is set up to handle this)
        ProcessBuilder processBuilder = new ProcessBuilder("kill", "-USR1", String.valueOf(process.pid()));
        try {
            processBuilder.start();
        } catch (IOException e) {
            LOGGER.error("Error while resetting Mealy machine: {}", e.getMessage());
        }
    }

    public void close() throws IOException {
    }

	@Override
	public void pre() {
//        LOGGER.info("=========== PRE =============");
        if(this.process == null){
          try{
            LOGGER.debug("Looking for target in directory:{}", System.getProperty("user.dir"));
            process = new ProcessBuilder(this.path).start();
          } catch(IOException e){
              LOGGER.error("Could not start target: {}", e.toString());
            System.exit(-1);
          }

          LOGGER.warn("Process is up");
          writer = new BufferedWriter(new OutputStreamWriter(process.getOutputStream()));
          reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
          LOGGER.info("mealy machine started on PID: {}", this.process.pid());
        }
	}

	@Override
	public void post() {
//        LOGGER.info("=========== POST =============");
        reset();
        String res;
        try {
            // Wait for the "OK" confirmation after reset
            do {
                res = reader.readLine();
                //LOGGER.info("Got: " + res);
            } while (res != null && !res.contains("OK"));
        } catch (IOException e) {
            throw new RuntimeException("Error reading reset confirmation", e);
        }
	}

	@Override
	public String step(String in) {

    // Write the input character
    try {
      writer.write(in);
      writer.newLine();
      writer.flush();

//      LOGGER.debug("Wrote input: " + in);
    } catch (IOException io){
//        LOGGER.error("Could not write to target {}", io.toString());
        System.exit(-1);
    }

    // Read output character
    String output = "";
    try {
        output = reader.readLine();
        //LOGGER.debug("Got output: " + output);
    } catch (IOException io){
        LOGGER.error("Could not read from target: {}", io.toString());
        System.exit(-1);
    }

    String res = output;
    // Introduce additional state with a variable
    // that tracks whether the GLOBAL_STATE variable
    // has been written to.
    // It is only changed in the memory and does not
    // give output state, but one could assume that
    // the application uses an out-of-band-communication
    // mechanism to propagate the state.
    boolean written_state = false;

    int i = 0;
    while(i < 8) {
        try {
            output = reader.readLine();
            //LOGGER.info("GOT DBG: " + output);
            // This assumes that we always expect a
            // variable called like this
            if(output.contains("GLOBAL_STATE")) {
                // VARNAME=[r:w]
                String counts = output.split("=")[1];
                // [r:w] must get w (written)
                String num_str = counts.split(":")[1].split("]")[0];
                //LOGGER.info("Written " + num_str + " times");
                written_state = Integer.parseInt(num_str) > 0;
            }
            i++;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    res +=(written_state ? "y" : "n");

    return res;
	}
}
