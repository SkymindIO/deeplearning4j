package org.nd4j.autodiff.samediff.internal;

import org.nd4j.linalg.api.buffer.DataType;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.api.shape.LongShapeDescriptor;

import java.io.Closeable;

/**
 * SessionMemMgr - aka "Session Memory Manager" is responsible for allocating, managing, and deallocating memory used
 * during SameDiff execution.<br>
 * This interface allows different memory management strategies to be used, abstracted away from the actual graph
 * execution logic
 *
 * @author Alex Black
 */
public interface SessionMemMgr extends Closeable {

    /**
     * Allocate an array with the specified datatype and shape.<br>
     * NOTE: This array should be assumed to be uninitialized - i.e., contains random values.
     *
     * @param detached If true: the array is safe to return outside of the SameDiff session run (for example, the array
     *                 is one that may be returned to the user)
     * @param dataType Datatype of the returned array
     * @param shape    Array shape
     * @return The newly allocated (uninitialized) array
     */
    INDArray allocate(boolean detached, DataType dataType, long... shape);

    /**
     * As per {@link #allocate(boolean, DataType, long...)} but from a LongShapeDescriptor instead
     */
    INDArray allocate(boolean detached, LongShapeDescriptor descriptor);

    /**
     * Allocate an uninitialized array with the same datatype and shape as the specified array
     */
    INDArray ulike(INDArray arr);

    /**
     * Duplicate the specified array, to an array that is managed/allocated by the session memory manager
     */
    INDArray dup(INDArray arr);

    /**
     * Release the array. All arrays allocated via one of the allocate methods should be returned here once they are no
     * longer used, and all references to them should be cleared.
     * After calling release, anything could occur to the array - deallocated, workspace closed, reused, etc.
     *
     * @param array The array that can be released
     */
    void release(INDArray array);

    /**
     * Close the session memory manager and clean up any memory / resources, if any
     */
    void close();

}
