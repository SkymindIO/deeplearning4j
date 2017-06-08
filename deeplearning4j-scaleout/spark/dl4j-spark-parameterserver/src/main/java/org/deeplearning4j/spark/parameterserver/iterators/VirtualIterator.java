package org.deeplearning4j.spark.parameterserver.iterators;

import javafx.beans.InvalidationListener;
import javafx.beans.Observable;
import lombok.NonNull;

import java.util.Iterator;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.LockSupport;
import java.util.function.Consumer;

/**
 * This class is thin wrapper, to provide block-until-depleted functionality in multi-threaded environment
 *
 * @author raver119@gmail.com
 */
public class VirtualIterator<E> implements Iterator<E> {
    // TODO: use AsyncIterator here?
    protected Iterator<E> iterator;
    protected AtomicBoolean state = new AtomicBoolean(true);

    public VirtualIterator(@NonNull Iterator<E> iterator) {
        this.iterator = iterator;
    }


    @Override
    public boolean hasNext() {
        boolean u = iterator.hasNext();
        state.compareAndSet(true, u);
        return u;
    }

    @Override
    public E next() {
        return iterator.next();
    }

    @Override
    public void remove() {
        // no-op, we don't need this call implemented
    }

    @Override
    public void forEachRemaining(Consumer<? super E> action) {
        iterator.forEachRemaining(action);
        state.compareAndSet(true, false);
    }

    /**
     * This method blocks until underlying Iterator is depleted
     */
    public void blockUntilDepleted() {
        // FIXME: implement Observer/Observable here, any blocking notification pattern will work here
        while (state.get())
            LockSupport.parkNanos(1000L);
    }
}
