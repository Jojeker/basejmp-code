/* Copyright (C) 2013-2024 TU Dortmund University
 * This file is part of LearnLib, http://www.learnlib.de/.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package uelearner;

import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.Collection;
import java.util.Random;

import de.learnlib.acex.AcexAnalyzer;
import de.learnlib.acex.AcexAnalyzers;
import de.learnlib.algorithm.LearningAlgorithm;
import de.learnlib.algorithm.LearningAlgorithm.MealyLearner;
import de.learnlib.algorithm.lstar.mealy.ExtensibleLStarMealyBuilder;
import de.learnlib.algorithm.ttt.mealy.TTTLearnerMealy;
import de.learnlib.algorithm.ttt.mealy.TTTLearnerMealyBuilder;
import de.learnlib.counterexample.AcexLocalSuffixFinder;
import de.learnlib.filter.statistic.Counter;
import de.learnlib.filter.statistic.oracle.MealyCounterOracle;
import de.learnlib.oracle.AutomatonOracle;
import de.learnlib.oracle.EquivalenceOracle;
import de.learnlib.oracle.MembershipOracle.MealyMembershipOracle;
import de.learnlib.oracle.equivalence.MealyRandomWordsEQOracle;
import de.learnlib.oracle.equivalence.MealyWMethodEQOracle;
import de.learnlib.oracle.membership.SULOracle;
import de.learnlib.query.DefaultQuery;
import de.learnlib.query.Query;
import net.automatalib.alphabet.Alphabet;
import net.automatalib.alphabet.ArrayAlphabet;
import net.automatalib.automaton.transducer.MealyMachine;
import net.automatalib.serialization.dot.GraphDOT;
// import net.automatalib.serialization.dot.GraphDOT;
// import net.automatalib.visualization.Visualization;
import net.automatalib.visualization.dot.DOT;
import net.automatalib.word.Word;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class App {

    LearningAlgorithm<MealyMachine<?, String, ?, String>, String, Word<String>> LearningAlgorithm;

    private static final Logger LOGGER = LoggerFactory.getLogger(App.class);
    static final int MIN_LENGTH = 1;
    static final int MAX_LENGTH = 20;
    static final int NUM_QUERIES = 150;
    static final Random SEED = new Random(0x1337);
     
    // prevent instantiation
    private App() {
    }

    public static void main(String[] args) throws IOException {

      // Load Mealy machine and alphabet
      MealyMachineSUL sul = new MealyMachineSUL("./target");
      MealyMachineSULMapper mapper = new MealyMachineSULMapper();
      MealyMembershipOracle<String,String> oracle = new SULOracle<>(sul);
      // Choosing our own implementation does not seem to work, why?
      //new MealyMachineMembershipOracle(sul, mapper);
      Alphabet<String> inputAlphabet = new ArrayAlphabet<>("a", "b", "c", "d", "e", "f", "g", "h");

      // TTT Algorithm -- applied to mealy machine
      MealyLearner<String,String> learner = new TTTLearnerMealyBuilder<String,String>()
              .withAlphabet(inputAlphabet)
              .withOracle(oracle)
              .withAnalyzer(AcexAnalyzers.BINARY_SEARCH_FWD)
              .create();

      // This is the easy way for finding counter examples
//      EquivalenceOracle.MealyEquivalenceOracle<String,String> eqOracle
//        = new MealyRandomWordsEQOracle<String, String>(oracle, MIN_LENGTH, MAX_LENGTH, NUM_QUERIES, SEED);
//
      // This is way more precise but insanely slow compared to choosing random words
          EquivalenceOracle<MealyMachine<?,String,?,String>, String, Word<String>> eqOracle =
                  new MealyWMethodEQOracle<>(oracle, 2);

      boolean learning = true;
      Counter rounds = new Counter("Rounds","TTTLearner");
      LOGGER.info("Learning started");

      // Start learning step and get first model
      learner.startLearning();
      MealyMachine<?, String, ?, String> hypothesis = learner.getHypothesisModel();

      while(learning){
        // For every round, output the dot model
        String filename = "mealy_hyp_" + rounds.getCount();
        rounds.increment();
        try {
          produceOutput(hypothesis, inputAlphabet, filename);
        } catch (Exception e){
            LOGGER.error("Could not write dot: {}", e.toString());
          System.exit(-1);
        }

        DefaultQuery<String, Word<String>> counterExample = eqOracle
          .findCounterExample(hypothesis, inputAlphabet); 
         

        // Learning finished
        if(counterExample == null){
          learning = false;
          LOGGER.info("Learning complete!");

          filename = "mealy_final.dot";
          try {
            produceOutput(hypothesis, inputAlphabet, filename);
          } catch (Exception e){
              LOGGER.error("Could not write new dot: {}", e.toString());
            System.exit(-1);
          }

        } else {
          // Counter example has to be included
            LOGGER.info("Counter example found: {}", counterExample.toString());
          
          // Refine the hypothesis
          if (learner.refineHypothesis(counterExample)){
            LOGGER.info("Refinement successful!");
          }else {
            LOGGER.error("Refinement failed");
            System.exit(-1);
          }

          // Get a new hypothesis after refining it with the 
          // counter example
          hypothesis = learner.getHypothesisModel();
        }
      }
    }

    public static void produceOutput(MealyMachine<?, String, ?, String> model, Alphabet<String> alphabet, String id) {
        try (PrintWriter dotWriter = new PrintWriter("learned-model.dot")) {
            GraphDOT.write(model, alphabet, dotWriter);
            DOT.runDOT(new File("learned-model.dot"), "pdf", new File("learned-"+ id + ".pdf"));
        } catch (Exception e) {
            System.err.println("Warning: Install graphviz to convert dot-files to PDF");
            System.err.println(e.getMessage());
        }
    }
}
