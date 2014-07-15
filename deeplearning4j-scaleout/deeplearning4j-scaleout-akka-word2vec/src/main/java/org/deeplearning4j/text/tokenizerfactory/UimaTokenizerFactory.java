package org.deeplearning4j.text.tokenizerfactory;

import org.apache.uima.analysis_engine.AnalysisEngine;
import org.apache.uima.fit.factory.AnalysisEngineFactory;
import org.apache.uima.resource.ResourceInitializationException;
import org.apache.uima.util.CasPool;
import org.deeplearning4j.text.annotator.SentenceAnnotator;
import org.deeplearning4j.text.annotator.StemmerAnnotator;
import org.deeplearning4j.text.annotator.TokenizerAnnotator;
import org.deeplearning4j.text.tokenizer.UimaTokenizer;
import org.deeplearning4j.word2vec.tokenizer.Tokenizer;
import org.deeplearning4j.word2vec.tokenizer.TokenizerFactory;


/**
 * Uses a uima {@link AnalysisEngine} to
 * tokenize text.
 *
 *
 * @author Adam Gibson
 *
 */
public class UimaTokenizerFactory implements TokenizerFactory {

    private final AnalysisEngine tokenizer;
    private CasPool pool;
    private final boolean checkForLabel;


    public UimaTokenizerFactory() throws ResourceInitializationException {
        this(defaultAnalysisEngine(),true);
    }

    public UimaTokenizerFactory(String language) throws ResourceInitializationException {
        this(defaultAnalysisEngine(language),true);
    }

    public UimaTokenizerFactory(AnalysisEngine tokenizer) {
       this(tokenizer,true);
    }


    public UimaTokenizerFactory(boolean checkForLabel) throws ResourceInitializationException {
        this(defaultAnalysisEngine(),checkForLabel);
    }



    public UimaTokenizerFactory(AnalysisEngine tokenizer,boolean checkForLabel) {
        super();
        this.tokenizer = tokenizer;
        this.checkForLabel = checkForLabel;
        try {
            pool = new CasPool(Runtime.getRuntime().availableProcessors() * 10,tokenizer);

        }catch(Exception e) {
            throw new RuntimeException(e);
        }
    }



    @Override
    public  Tokenizer create(String toTokenize) {
        if(tokenizer == null || pool == null) {
            throw new IllegalStateException("Unable to proceed; tokenizer or pool is null");
        }
        if(toTokenize == null || toTokenize.isEmpty()) {
            throw new IllegalArgumentException("Unable to proceed; on sentence to tokenize");
        }
        return new UimaTokenizer(toTokenize,tokenizer,pool,checkForLabel);
    }


    /**
     * Creates a tokenization,/stemming pipeline
     * @return a tokenization/stemming pipeline
     */
    public static AnalysisEngine defaultAnalysisEngine()  {
        try {
            return AnalysisEngineFactory.createEngine(AnalysisEngineFactory.createEngineDescription(
                    SentenceAnnotator.getDescription(),
                    TokenizerAnnotator.getDescription(),
                    StemmerAnnotator.getDescription("English")));
        }catch(Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Creates a tokenization,/stemming pipeline
     * @param language
     * @return a tokenization/stemming pipeline
     */
    public static AnalysisEngine defaultAnalysisEngine(String language)  {
        try {
            return AnalysisEngineFactory.createEngine(AnalysisEngineFactory.createEngineDescription(
                    SentenceAnnotator.getDescription(),
                    TokenizerAnnotator.getDescription(),
                    StemmerAnnotator.getDescription(language)));
        }catch(Exception e) {
            throw new RuntimeException(e);
        }
    }
}
