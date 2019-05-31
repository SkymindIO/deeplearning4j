package org.nd4j.autodiff.ui;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.nd4j.autodiff.listeners.impl.ScoreListener;
import org.nd4j.autodiff.listeners.impl.UIListener;
import org.nd4j.autodiff.samediff.SDVariable;
import org.nd4j.autodiff.samediff.SameDiff;
import org.nd4j.autodiff.samediff.TrainingConfig;
import org.nd4j.evaluation.classification.Evaluation;
import org.nd4j.linalg.api.buffer.DataType;
import org.nd4j.linalg.dataset.IrisDataSetIterator;
import org.nd4j.linalg.factory.Nd4j;
import org.nd4j.linalg.learning.config.Adam;

import java.io.File;

public class UIListenerTest {

    @Rule
    public TemporaryFolder testDir = new TemporaryFolder();

    @Test
    public void testUIListenerBasic() throws Exception {
        Nd4j.getRandom().setSeed(12345);

        IrisDataSetIterator iter = new IrisDataSetIterator(150, 150);

        SameDiff sd = SameDiff.create();
        SDVariable in = sd.placeHolder("in", DataType.FLOAT, -1, 4);
        SDVariable label = sd.placeHolder("label", DataType.FLOAT, -1, 3);
        SDVariable w = sd.var("W", Nd4j.rand(DataType.FLOAT, 4, 3));
        SDVariable b = sd.var("b", DataType.FLOAT, 1, 3);
        SDVariable mmul = in.mmul(w).add(b);
        SDVariable softmax = sd.nn.softmax("softmax", mmul);
        SDVariable loss = sd.loss().logLoss("loss", label, softmax);

        File dir = testDir.newFolder();
//        File dir = new File("C:/Temp/SameDiffUI/");
        File f = new File(dir, "logFile.bin");
        f.delete();
        UIListener l = UIListener.builder(f)
                .plotLosses(1)
                .trainEvaluationMetrics("softmax", 0, Evaluation.Metric.ACCURACY, Evaluation.Metric.F1)
                .updateRatios(1)
                .build();

        sd.setListeners(l, new ScoreListener(1));

        sd.setTrainingConfig(TrainingConfig.builder()
                .dataSetFeatureMapping("in")
                .dataSetLabelMapping("label")
                .updater(new Adam(1e-1))
                .weightDecay(1e-3, true)
                .build());

        sd.fit(iter, 30);


    }

}
