package org.deeplearning4j.models.word2vec.wordstore;

import lombok.Data;
import lombok.NonNull;

import java.io.Serializable;

/**
 * Simplified version of VocabWord.
 * Used only for w2v vocab building routines
 *
 * @author raver119@gmail.com
 */

@Data
public class VocabularyWord implements Serializable {
    @NonNull
    private String word;
    private int count = 1;

    // There's no reasons to save HuffmanNode data, it will be recalculated after deserialization
    private transient HuffmanNode huffmanNode;

    // empty constructor is required for proper deserialization
    public VocabularyWord() {

    }

    public VocabularyWord(@NonNull String word) {
        this.word = word;
    }

    /*
        since scavenging mechanics are targeting low-freq words, byte values is definitely enough.
        please note, array is initialized outside of this scope, since it's only holder, no logic involved inside this class
      */
    private transient byte[] frequencyShift;
    private transient byte retentionStep;

    // special mark means that this word should NOT be affected by minWordFrequency setting
    private boolean special = false;

    public void incrementCount() {
        this.count++;
    }

    public void incrementRetentionStep() {
        this.retentionStep += 1;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        VocabularyWord that = (VocabularyWord) o;

        if (count != that.count) return false;
        return word.equals(that.word);

    }

    @Override
    public int hashCode() {
        int result = word.hashCode();
        result = 31 * result + count;
        return result;
    }
}
