package uelearner;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Collection;

import de.learnlib.oracle.MembershipOracle.MealyMembershipOracle;
import de.learnlib.query.Query;
import de.learnlib.sul.SUL;
import net.automatalib.word.Word;
import net.automatalib.word.WordBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MealyMachineMembershipOracle implements MealyMembershipOracle<String,String> {

  private final MealyMachineSULMapper mapper;
  private final SUL<String, String> sul;
  private final Logger LOGGER = LoggerFactory.getLogger(MealyMachineMembershipOracle.class);

  public MealyMachineMembershipOracle(SUL<String, String> sul, MealyMachineSULMapper mapper){
    this.mapper = mapper;
    this.sul = sul;
  }


	@Override
	public void processQueries(Collection<? extends Query<String, Word<String>>> queries) {
  
    sul.pre();

//    ArrayList<Long> times = new ArrayList<>();
    for(Query<String, Word<String>> q : queries){
      final Word<String> input = q.getInput();
      WordBuilder<String> abstractOutputBuilder = new WordBuilder<>(input.size());

//      long start = System.nanoTime();

      for(String sym : input){
        String mappedSym = mapper.mapInput(sym);

        String output = sul.step(mappedSym);

        String mappedOutSym = mapper.mapOutput(output);
        abstractOutputBuilder.append(mappedOutSym);
      }

      // Reset the machine
      sul.post();


//      long stop = System.nanoTime();
//      times.add(stop - start);
//
      Word<String> outputWord = abstractOutputBuilder.toWord();
      LOGGER.info("I: " + input + ", O: " + outputWord);
      q.answer(outputWord);
    }
    //LOGGER.info(times.size() + "queries finished! A query took on average " + times.stream().reduce((long)0, Long::sum) / times.size() / 1000000);
  }

}
