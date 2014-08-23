package org.deeplearning4j.iterativereduce.tracker.statetracker.hazelcast;

import com.hazelcast.client.HazelcastClient;
import com.hazelcast.client.config.ClientConfig;
import com.hazelcast.config.Config;
import com.hazelcast.config.JoinConfig;
import com.hazelcast.config.ListConfig;
import com.hazelcast.config.MapConfig;
import com.hazelcast.core.*;
import io.dropwizard.Application;
import io.dropwizard.setup.Bootstrap;
import io.dropwizard.setup.Environment;
import org.apache.commons.compress.utils.IOUtils;

import org.deeplearning4j.iterativereduce.actor.core.Job;
import org.deeplearning4j.iterativereduce.actor.util.PortTaken;
import org.deeplearning4j.iterativereduce.tracker.statetracker.*;
import org.deeplearning4j.linalg.dataset.DataSet;
import org.deeplearning4j.nn.BaseMultiLayerNetwork;
import org.deeplearning4j.optimize.OutputLayerTrainingEvaluator;
import org.deeplearning4j.optimize.TrainingEvaluator;
import org.deeplearning4j.scaleout.iterativereduce.Updateable;
import org.deeplearning4j.scaleout.iterativereduce.multi.UpdateableImpl;
import org.deeplearning4j.util.SerializationUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.core.io.ClassPathResource;

import javax.ws.rs.*;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.InetAddress;
import java.util.*;

/**
 * Baseline hazelcast state tracker
 * @author Adam Gibson
 */

public abstract class BaseHazelCastStateTracker<E extends Updateable<?>>  implements StateTracker<E> {

    /**
     *
     */
    private static final long serialVersionUID = -7374372180080957334L;
    public final static String JOBS = "org.deeplearning4j.jobs";
    public final static String NUM_TIMES_PRETRAIN_RAN = "pretrainran";
    public final static String WORKERS = "org.deeplearning4j.workers";
    public final static String AVAILABLE_WORKERS = "AVAILABLE_WORKERS";
    public final static String NUM_TIMES_RUN_PRETRAIN = "PRETRAIN";
    public final static String TOPICS = "topics";
    public final static String RESULT = "RESULT";
    public final static String DONE = "done";
    public final static String UPDATES = "updates";
    public final static String REPLICATE_WEIGHTS = "replicate";
    public final static String HEART_BEAT = "heartbeat";
    public final static String WORKER_ENABLED = "workerenabled";
    public final static String INPUT_SPLIT = "inputsplit";
    public final static String IS_PRETRAIN = "ispretrain";
    public final static String BEST_LOSS = "bestloss";
    public final static String IMPROVEMENT_THRESHOLD = "improvementthreshold";
    public final static String VALIDATION_EPOCHS = "validationepochs";
    public final static String EARLY_STOP = "earlystop";
    public final static String PATIENCE = "patience";
    public final static String PATIENCE_INCREASE = "patienceincrease";
    public final static String BEGUN = "begun";
    public final static String NUM_BATCHES_SO_FAR_RAN = "numbatches";

    private volatile transient IAtomicReference<Object> master;
    private volatile transient IList<Job> jobs;
    private volatile transient IAtomicReference<Integer> numTimesPretrain;
    private volatile transient IAtomicReference<Integer> numTimesPretrainRan;
    private volatile transient IAtomicReference<Double> bestLoss;
    private volatile transient IAtomicReference<Double> improvementThreshold;
    private volatile transient IAtomicReference<Integer> numBatches;

    private volatile transient IAtomicReference<Boolean> earlyStop;

    private volatile transient IAtomicReference<Boolean> done;
    private volatile transient IList<String> replicate;
    private volatile transient IMap<String,Boolean> workerEnabled;
    private volatile transient IList<String> workers;
    private volatile  transient IList<String> topics;
    private volatile  transient IList<String> updates;
    private volatile IAtomicReference<Double> patience;
    private volatile IAtomicReference<Boolean> begunTraining;
    private volatile IAtomicReference<Double> patienceIncrease;
    private volatile IAtomicReference<Integer> validationEpochs;
    private volatile IAtomicReference<Integer> miniBatchSize;
    private WorkRetriever workRetriever = new LocalWorkRetriever();
    protected UpdateSaver<E> saver;
    private DataSetCache cache = new LocalDataSetCache();
    private volatile IAtomicReference<Boolean> isPretrain;
    private static Logger log = LoggerFactory.getLogger(HazelCastStateTracker.class);
    private transient Config config;
    public final static int DEFAULT_HAZELCAST_PORT = 2510;
    private transient HazelcastInstance h;
    private String type = "master";
    private int hazelCastPort = -1;
    private String connectionString;
    private Map<String,Long> heartbeat;
    private StateTrackerDropWizardResource resource;

    public BaseHazelCastStateTracker() throws Exception {
        this(DEFAULT_HAZELCAST_PORT);

    }

    /**
     * Number of batches ran so far
     *
     * @return the number of batches ran so far
     */
    @Override
    public int numBatchesRan() {
        return numBatches.get();
    }

    /**
     * Increments the number of batches ran.
     * This is purely a count and does not necessarily mean progress.
     *
     * @param numBatchesRan the number of batches ran to increment by
     */
    @Override
    public void incrementBatchesRan(int numBatchesRan) {
        numBatches.set(numBatchesRan + numBatches.get());
    }

    /**
     * Starts the rest api
     */
    @Override
    public void startRestApi() {
        resource = new StateTrackerDropWizardResource(this);
        try {
            InputStream is = new ClassPathResource("dropwizard.yml").getInputStream();
            File tmpConfig = new File("dropwizard.yml");
            BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(tmpConfig));
            IOUtils.copy(is, bos);
            bos.flush();
            resource.run(new String[]{"server",tmpConfig.getAbsolutePath()});
            tmpConfig.deleteOnExit();
        } catch(Exception e) {
            throw new RuntimeException(e);
        }

    }

    public abstract UpdateSaver<E> createUpdateSaver();

    /**
     * Current mini batch size
     *
     * @return
     */
    @Override
    public int miniBatchSize() {
        return miniBatchSize.get();
    }

    /**
     * Begin training
     */
    @Override
    public void beginTraining() {
        begunTraining.set(true);
    }

    /**
     * Whether the cluster has begun training
     *
     * @return whether the cluster  has begun training
     */
    @Override
    public boolean hasBegun() {
        return begunTraining.get();
    }

    /**
     * Removes the worker data
     *
     * @param worker the worker to remove
     */
    @Override
    public void removeWorkerData(String worker) {
        workRetriever.clear(worker);
    }

    /**
     * The collection of dat
     *
     * @return
     */
    @Override
    public Collection<String> workerData() {
        return workRetriever.workers();
    }

    /**
     * Sets the work retriever to use for storing data sets for workers
     *
     * @param workRetriever the work retreiver to use with this state tracker
     */
    @Override
    public void setWorkRetriever(WorkRetriever workRetriever) {
        this.workRetriever = workRetriever;
    }

    /**
     * Loads the data for a given worker
     *
     * @param workerId the worker id to load data for
     * @return the data applyTransformToDestination for a given worker
     */
    @Override
    public DataSet loadForWorker(String workerId) {
        return workRetriever.load(workerId);
    }

    /**
     * Saves the data for the given worker to work on
     *
     * @param workerId the worker to save
     * @param d        the data for the worker
     */
    @Override
    public void saveWorker(String workerId, DataSet d) {
        workRetriever.save(workerId,d);
    }

    /**
     * Creates a training evaluator using the given neural network
     *
     * @param network the neural network to use
     * @return a training evaluator based on the configuration of the state tracker
     * and the given network
     */
    @Override
    public TrainingEvaluator create(BaseMultiLayerNetwork network) {
        OutputLayerTrainingEvaluator eval = new OutputLayerTrainingEvaluator
                .Builder().bestLoss(bestLoss()).improvementThreshold(improvementThreshold())
                .patience(patience()).testSet(testSet())
                .withNetwork(network).validationEpochs(validationEpochs()).patienceIncrease(patienceIncrease.get())
                .build();
        return eval;
    }

    /**
     * Set the data applyTransformToDestination cache to use for fetching the test applyTransformToDestination
     *
     * @param cache the cache to use
     */
    @Override
    public void setDataSetCache(DataSetCache cache) {
        if(cache == null)
            throw new IllegalArgumentException("Cache must not be null");
        this.cache = cache;
    }


    /**
     * The patience improvement to use
     *
     * @param improvmentThreshold the patience improvement to applyTransformToDestination
     */
    @Override
    public void setImprovmentThreshold(double improvmentThreshold) {
        improvementThreshold.set(improvmentThreshold);
    }

    /**
     * Improvement threshold for early stopping, aka
     * the minimum
     *
     * @return
     */
    @Override
    public double improvementThreshold() {
        return improvementThreshold.get();
    }

    /**
     * Setter for patience
     *
     * @param patience
     */
    @Override
    public void setPatience(double patience) {
        this.patience.set(patience);
    }


    /**
     * Patience is what controls early stopping
     *
     * @return the patience for the trainer
     */
    @Override
    public double patience() {
        return patience.get();
    }

    /**
     * Improvement threshold for early stopping, aka
     * the minimum
     *
     * @return
     */
    @Override
    public double improvmentThreshold() {
        return improvementThreshold.get();
    }

    /**
     * The test applyTransformToDestination to use for validation
     *
     * @return the test to use for validation
     */
    @Override
    public DataSet testSet() {
        return cache.get();
    }

    /**
     * Sets the best loss
     *
     * @param bestLoss the best loss to use
     */
    @Override
    public void setBestLoss(double bestLoss) {
        this.bestLoss.set(bestLoss);
    }

    /**
     * The best validation loss so far
     *
     * @return the best validation loss so far
     */
    @Override
    public double bestLoss() {
        return bestLoss.get();
    }

    /**
     * The number of epochs to test on
     *
     * @return the number of epochs to test on
     */
    @Override
    public int validationEpochs() {
        return validationEpochs.get();
    }

    /**
     * Whether to validate against a held out test applyTransformToDestination and test for validation error.
     *
     * @return whether to validate against a held out test applyTransformToDestination and test for validation error.
     */
    @Override
    public boolean isEarlyStopTesting() {
        return earlyStop.get();
    }

    /**
     * A collection of worker updates.
     * This should be used to track
     * which workers have actually contributed an update for a given mini batch
     *
     * @return the worker updates
     */
    @Override
    public Collection<String> workerUpdates() {
        return updates;
    }

    /**
     * The update saver to use
     *
     * @param updateSaver the update saver to use
     */
    @Override
    public void setUpdateSaver(UpdateSaver<E> updateSaver) {
        this.saver = updateSaver;
    }

    /**
     * The update saver used with this state tracker
     *
     * @return the update saver used with this state tracker
     */
    @Override
    public UpdateSaver<E> updateSaver() {
        return saver;
    }

    /**
     * Sets the input split
     *
     * @param batchSize the input split to use
     */
    @Override
    public void setMiniBatchSize(int batchSize) {
        this.miniBatchSize.set(batchSize);
    }

    /**
     * The input split to use.
     * This means that each data applyTransformToDestination that is trained on
     * and loaded will be this batch size or lower
     * per worker
     *
     * @return the input split to use
     */
    @Override
    public int inputSplit() {
        return (miniBatchSize.get() * numWorkers()) / numWorkers();
    }

    /**
     * Returns the partition (optimal batch size)
     * given the available workers and the specified input split
     *
     * @return the optimal batch size
     */
    @Override
    public int partition() {
        return  inputSplit();
    }

    /**
     * Returns the status of whether the worker is enabled or not
     *
     * @param id the id of the worker to test
     * @return true if the worker is enabled, false otherwise
     */
    @Override
    public boolean workerEnabled(String id) {
        return workerEnabled.containsKey(id) && workerEnabled.get(id);
    }

    /**
     * Enables the worker with the given id,
     * allowing it to take jobs again
     *
     * @param id the id of the worker to enable
     */
    @Override
    public void enableWorker(String id) {
        workerEnabled.put(id,true);
    }

    /**
     * Disables the worker with the given id,
     * this means that it will not train
     * or take any new jobs until re enabled
     *
     * @param id the id of the worker to disable
     */
    @Override
    public void disableWorker(String id) {
        workerEnabled.put(id,false);
    }

    /**
     * Updates the status of the worker to not needing replication
     *
     * @param workerId the worker id to update
     */
    @Override
    public void doneReplicating(String workerId) {
        replicate.remove(workerId);
    }

    /**
     * Adds a worker to the list to be replicate d
     *
     * @param workerId the worker id to add
     */
    @Override
    public void addReplicate(String workerId) {
        if(!replicate.contains(workerId))
            replicate.add(workerId);
    }

    /**
     * Tracks worker ids that need state replication
     *
     * @param workerId the worker id to replicate
     * @return the list of worker ids that need state replication
     */
    @Override
    public boolean needsReplicate(String workerId) {
        return replicate.contains(workerId);
    }

    /**
     * Adds an update to the current mini batch
     * @param id the id of the worker who did the update
     * @param update the update to add
     */
    @Override
    public void addUpdate(String id,E update) {
        try {
            updateSaver().save(id,update);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        updates.add(id);

    }

    /**
     * Updates  for mini batches
     *
     * @return the current list of updates for mini batches
     */
    @Override
    public abstract IterateAndUpdate<E> updates();

    /**
     * Sets the connection string for connecting to the server
     *
     * @param connectionString the connection string to use
     */
    @Override
    public void setConnectionString(String connectionString) {
        this.connectionString = connectionString;
    }

    /**
     * Connection string for connecting to the server
     *
     * @return the connection string for connecting to the server
     */
    @Override
    public String connectionString() {
        return connectionString;
    }

    /**
     * Initializes the state tracker binding to the given port
     * @param stateTrackerPort the port to bind to
     * @throws Exception
     */
    public BaseHazelCastStateTracker(int stateTrackerPort) throws Exception {
        this("master","master",stateTrackerPort);

    }

    /**
     * Worker constructor
     * @param connectionString
     */
    public BaseHazelCastStateTracker(String connectionString) throws Exception {
        this(connectionString,"worker",DEFAULT_HAZELCAST_PORT);
    }

    public BaseHazelCastStateTracker(String connectionString,String type,int stateTrackerPort) throws Exception {
        log.info("Setting up hazelcast with type " + type + " connection string " + connectionString + " and port " + stateTrackerPort);


        if(type.equals("master") && !PortTaken.portTaken(stateTrackerPort)) {
            //sets up a proper connection string for reference wrt external actors needing a reference
            if(connectionString.equals("master")) {
                String host = InetAddress.getLocalHost().getHostName();
                this.connectionString = host + ":" + stateTrackerPort;
            }

            this.hazelCastPort = stateTrackerPort;
            config = hazelcast();


            h = Hazelcast.newHazelcastInstance(config);
            h.getCluster().addMembershipListener(new MembershipListener() {

                @Override
                public void memberAdded(MembershipEvent membershipEvent) {
                    log.info("Member added " + membershipEvent.toString());
                }

                @Override
                public void memberRemoved(MembershipEvent membershipEvent) {
                    log.info("Member removed " + membershipEvent.toString());

                }

                @Override
                public void memberAttributeChanged(
                        MemberAttributeEvent memberAttributeEvent) {
                    log.info("Member changed " + memberAttributeEvent.toString());

                }

            });
        }

        else if(type.equals("master") && PortTaken.portTaken(stateTrackerPort))
            throw new IllegalStateException("Specified type was master and the port specified was taken, please specify a different port");

        else {

            setConnectionString(connectionString);
            log.info("Connecting to hazelcast on " + connectionString);
            ClientConfig client = new ClientConfig();
            client.getNetworkConfig().addAddress(connectionString);
            h = HazelcastClient.newHazelcastClient(client);

        }

        this.type = type;

        jobs = h.getList(JOBS);
        workers = h.getList(WORKERS);

        //we can make the assumption workers isn't empty because
        //the master node by default comes with a applyTransformToDestination of workers
        if(!this.type.equals("master")) {
            while(workers.isEmpty()) {
                log.warn("Waiting for data sync...");
                Thread.sleep(1000);
            }

            log.info("Workers is " + workers.size());

        }

        begunTraining = h.getAtomicReference(BEGUN);
        miniBatchSize = h.getAtomicReference(INPUT_SPLIT);
        workerEnabled = h.getMap(WORKER_ENABLED);
        replicate = h.getList(REPLICATE_WEIGHTS);
        topics = h.getList(TOPICS);
        updates = h.getList(UPDATES);
        heartbeat = h.getMap(HEART_BEAT);
        master = h.getAtomicReference(RESULT);
        isPretrain = h.getAtomicReference(IS_PRETRAIN);
        numTimesPretrain = h.getAtomicReference(NUM_TIMES_RUN_PRETRAIN);
        numTimesPretrainRan = h.getAtomicReference(NUM_TIMES_PRETRAIN_RAN);
        done = h.getAtomicReference(DONE);
        validationEpochs = h.getAtomicReference(VALIDATION_EPOCHS);
        improvementThreshold = h.getAtomicReference(IMPROVEMENT_THRESHOLD);
        bestLoss = h.getAtomicReference(BEST_LOSS);
        earlyStop = h.getAtomicReference(EARLY_STOP);
        patience = h.getAtomicReference(PATIENCE);
        patienceIncrease = h.getAtomicReference(PATIENCE_INCREASE);
        numBatches = h.getAtomicReference(NUM_BATCHES_SO_FAR_RAN);

        //applyTransformToDestination defaults only when master, otherwise, overrides previous values
        if(type.equals("master")) {
            begunTraining.set(false);
            saver = createUpdateSaver();
            numTimesPretrainRan.set(0);
            numTimesPretrain.set(1);
            isPretrain.set(true);
            done.set(false);
            resource = new StateTrackerDropWizardResource(this);
            bestLoss.set(Double.POSITIVE_INFINITY);
            earlyStop.set(true);
            patience.set(40.0);
            patienceIncrease.set(2.0);
            improvementThreshold.set(0.995);
            validationEpochs.set((int) Math.min(10,patience() / 2));
            numBatches.set(0);
        }


    }

    private Config hazelcast() {
        Config conf = new Config();
        conf.getNetworkConfig().setPort(hazelCastPort);
        conf.getNetworkConfig().setPortAutoIncrement(false);



        conf.setProperty("hazelcast.initial.min.cluster.size","1");
        conf.setProperty("hazelcast.shutdownhook.enabled","false");


        JoinConfig join = conf.getNetworkConfig().getJoin();

        boolean isAws = System.getProperty("hazelcast.aws","false").equals("true");
        log.info("Setting up Joiner with this being "  + (isAws ? "AWS" : "Multicast"));

        join.getAwsConfig().setEnabled(isAws);
        if(isAws) {
            join.getAwsConfig().setAccessKey(System.getProperty("hazelcast.access-key"));
            join.getAwsConfig().setSecretKey(System.getProperty("hazelcast.access-secret"));
        }
        join.getMulticastConfig().setEnabled(!isAws);




        ListConfig jobConfig = new ListConfig();
        jobConfig.setName(JOBS);

        conf.addListConfig(jobConfig);

        ListConfig replicateConfig = new ListConfig();
        replicateConfig.setName(REPLICATE_WEIGHTS);

        conf.addListConfig(replicateConfig);


        ListConfig topicsConfig = new ListConfig();
        topicsConfig.setName(TOPICS);

        conf.addListConfig(topicsConfig);


        ListConfig updatesConfig = new ListConfig();
        updatesConfig.setName(UPDATES);

        conf.addListConfig(updatesConfig);


        ListConfig availableWorkersConfig = new ListConfig();
        availableWorkersConfig.setName(AVAILABLE_WORKERS);
        conf.addListConfig(availableWorkersConfig);


        MapConfig heartbeatConfig = new MapConfig();
        heartbeatConfig.setName(HEART_BEAT);
        conf.addMapConfig(heartbeatConfig);

        MapConfig workerEnabledConifg = new MapConfig();
        workerEnabledConifg.setName(WORKER_ENABLED);
        conf.addMapConfig(workerEnabledConifg);

        return conf;

    }




    @Override
    public boolean addJobToCurrent(Job j) throws Exception {

        IAtomicReference<Job> r = h.getAtomicReference("job-" + j.getWorkerId());


        if(r.get() != null || !r.isNull()) {
            boolean sent = false;
            while(!sent) {
                //always update
                for(String s : workers()) {
                    if(jobFor(s) == null) {
                        log.info("Redirecting worker " + j.getWorkerId() + " to " + s + " due to work already being allocated");
                        r = h.getAtomicReference("job-" + s);
                        j.setWorkerId(s);
                        sent = true;
                    }
                }

            }
        }

        r.set(j);

        //iterate over jobs without the work/data
        j.setWork(null);

        jobs.add(j);

        return true;

    }

    @Override
    public void setServerPort(int port) {
        this.hazelCastPort = port;
    }

    @Override
    public int getServerPort() {
        return hazelCastPort;
    }
    @Override
    public List<Job> currentJobs() throws Exception {
        return new ArrayList<>(jobs);
    }


    /**
     * Assuming a job already exists, updates the job
     *
     * @param j the job to update
     */
    @Override
    public void updateJob(Job j) {
        IAtomicReference<Job> jRef = h.getAtomicReference("job-" + j.getWorkerId());
        jRef.set(j);
    }

    @Override
    public void clearJob(String id) throws Exception {
        if(id == null) {
            log.warn("No job to clear; was null, returning");
            return;

        }

        IAtomicReference<Job> jRef = h.getAtomicReference("job-" + id);
        if(jRef.isNull())
            return;
        jRef.clear();
        log.info("Destroyed job ref " + id);
        Job remove = null;
        for(Job j : jobs) {
            if(j.getWorkerId().equals(id)) {
                remove = j;
                break;
            }
        }


        jobs.remove(remove);
    }

    @Override
    public void shutdown() {
        if(h != null)
            h.shutdown();

    }

    @Override
    public void addTopic(String topic) throws Exception {
        topics.add(topic);


    }

    @Override
    public List<String> topics() throws Exception {
        return topics;
    }

    @Override
    public  E getCurrent() throws Exception {
        E u =  (E) master.get();
        if(u == null)
            return null;
        return u;
    }

    @Override
    public  void setCurrent(E e) throws Exception {
        if(e == null || e.get() == null) {
            log.warn("Not setting a null update");
            return;
        }

        this.master.set(e);
    }


    @Override
    public boolean isPretrain() {
        return  isPretrain.get();
    }

    @Override
    public void moveToFinetune() {
        log.info("Moving to finetune");
        isPretrain.set(false);
        //new phase: resets the counter
        numBatches.set(0);
    }

    @Override
    public Job jobFor(String id) {
        IAtomicReference<Job> j = h.getAtomicReference("job-" + id);
        if(j.isNull() || isCurrentlyJob(id))
            return null;
        return j.get();
    }

    private boolean isCurrentlyJob(String id) {
        for(Job j : jobs)
            if(j.equals(id))
                return true;
        return false;
    }

    @Override
    public void availableForWork(String id) {
        if(!workers.contains(id))
            workers.add(id);
    }

    @Override
    public List<String> jobIds() {
        List<String> ret = new ArrayList<>();
        for(Job j : this.jobs)
            ret.add(j.getWorkerId());
        return ret;
    }

    @Override
    public void addWorker(String worker) {
        heartbeat.put(worker, System.currentTimeMillis());
        if(!workers.contains(worker)) {
            log.info("Adding worker " + worker);
            workers.add(worker);
            log.info("Number of workers is now " + workers.size());

        }
    }

    @Override
    public void removeWorker(String worker) {
        workers.remove(worker);
        if(jobFor(worker) != null) {
            try {
                clearJob(worker);

            }catch(Exception e) {
                log.warn("Unable to clear job for worker with id" + worker);
            }
        }
    }

    @Override
    public List<String> workers() {
        return workers;
    }

    @Override
    public int numWorkers() {
        int num = workers.size();
        return num;
    }

    public synchronized HazelcastInstance getH() {
        return h;
    }

    public synchronized void setH(HazelcastInstance h) {
        this.h = h;
    }

    @Override
    public Map<String, Long> getHeartBeats() {
        return heartbeat;
    }

    @Override
    public void runPreTrainIterations(int numTimes) {
        numTimesPretrain.set(numTimes);
    }

    @Override
    public int runPreTrainIterations() {
        return numTimesPretrain.get();
    }

    @Override
    public int numTimesPreTrainRun() {
        return numTimesPretrainRan.get();
    }

    @Override
    public void incrementNumTimesPreTrainRan() {
        numTimesPretrainRan.set(numTimesPreTrainRun() + 1);
    }

    @Override
    public boolean isDone() {
        //reason being that isDone() may getFromOrigin called and throw errors
        //this ensures a safe method call happens and just silently
        //returns true in case hazelcast is shutdown
        try {
            return done.get();
        }catch(Exception e) {
            log.warn("Hazelcast already shutdown...returning true on isDone()");
            return true;
        }
    }






    @Override
    public void finish() {
        //reason being that isDone() may getFromOrigin called and throw errors
        //this ensures a safe method call happens and just silently
        //returns true in case hazelcast is shutdown
        try {
            done.set(true);
            updateSaver().cleanup();
        }catch(Exception e) {
            log.warn("Hazelcast already shutdown...done() being applyTransformToDestination is pointless");
        }
    }

}
