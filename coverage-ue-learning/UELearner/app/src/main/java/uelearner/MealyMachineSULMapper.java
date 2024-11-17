package uelearner;

import de.learnlib.Mapper;

public class MealyMachineSULMapper implements Mapper<String,String,String,String>{

    @Override
    public String mapInput(String input) {
        // Example: Map the input letters to corresponding numbers
        return switch (input) {
            case "a" -> "a";
            case "b" -> "b";
            case "c" -> "c";
            case "d" -> "d";
            case "e" -> "e";
            case "f" -> "f";
            case "g" -> "g";
            case "h" -> "h";
            default -> throw new IllegalArgumentException("Unknown input symbol: " + input);
        };
    }

    @Override
    public String mapOutput(String output) {
        // if(output == null){
        //   return "????";
        // }

          // Example: Map the output numbers back to letters
//        return switch (output) {
//            case "1y" -> "STATE1y";
//            case "1n" -> "STATE1n";
//            case "2y" -> "STATE2y";
//            case "2n" -> "STATE2n";
//            case "3y" -> "STATE3y";
//            case "3n" -> "STATE3n";
//            case "4y" -> "STATE4y";
//            case "4n" -> "STATE4n";
//            case "5y" -> "STATE5y";
//            case "5n" -> "STATE5n";
//            case "6y" -> "STATE6y";
//            case "6n" -> "STATE6n";
//            case "7y" -> "STATE7y";
//            case "7n" -> "STATE7n";
//            case "8y" -> "STATE8y";
//            case "8n" -> "STATE8n";
//
//            case "Xy" -> "INITy";
//            case "Xn" -> "INITn";
//            default -> throw new IllegalArgumentException("Unknown output symbol: " + output);
        return output;
//        };
    }
}

